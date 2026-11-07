#include "Events.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <vector>

#include "Corpse.h"
#include "IconMenu.h"
#include "PCH.h"
#include "Plugin.h"
#include "Settings.h"
#include "State.h"

// QuickLoot IE API header (MIT) from github.com/MissCorruption/QuickLootIE, needs Windows.h
#include <Windows.h>

#include "QuickLootAPI.h"

namespace UCI {
    namespace {
        bool IsPlayerOrAlly(RE::TESObjectREFR* a_killer) {
            if (!a_killer) return false;
            if (a_killer->IsPlayerRef()) return true;

            const auto* actor = a_killer->As<RE::Actor>();
            if (!actor) return false;
            if (actor->IsPlayerTeammate()) return true;

            const auto commander = actor->GetCommandingActor();
            return commander && commander->IsPlayerRef();
        }

        bool IsInMenu() {
            auto* ui = RE::UI::GetSingleton();
            return !ui || ui->GameIsPaused() || ui->IsMenuOpen(RE::Console::MENU_NAME);
        }

        bool ContainsNoCase(std::string_view a_text, std::string_view a_word) {
            const auto lower = [](unsigned char a_char) { return std::tolower(a_char); };
            return !std::ranges::search(a_text, a_word, {}, lower, lower).empty();
        }

        // e.g. magicAttachAshPileOnDeath, DLC2AshSpawnDisintegrateDeathFX
        bool HasAshDeathScript(RE::Actor* a_actor) {
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            auto* target = a_actor ? a_actor->GetMagicTarget() : nullptr;
            auto* effects = target ? target->GetActiveEffectList() : nullptr;
            if (!vm || !effects) return false;

            std::vector<RE::VMHandle> handles;
            for (auto* effect : *effects) {
                if (effect)
                    handles.push_back(
                        vm->GetObjectHandlePolicy()->GetHandleForObject(RE::ActiveEffect::VMTYPEID, effect));
            }

            RE::BSSpinLockGuard guard(vm->attachedScriptsLock);
            for (const auto handle : handles) {
                const auto it = vm->attachedScripts.find(handle);
                if (it == vm->attachedScripts.end()) continue;
                for (auto& script : it->second) {
                    const auto object = script.get();
                    const auto type = object ? object->GetTypeInfo() : nullptr;
                    if (!type) continue;
                    const std::string_view name = type->GetName();
                    if (ContainsNoCase(name, "AshPile") || ContainsNoCase(name, "Disintegrate")) return true;
                }
            }
            return false;
        }
    }

    class InputSink : public RE::BSTEventSink<RE::InputEvent*> {
    public:
        static InputSink* GetSingleton() {
            static InputSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* events,
                                              RE::BSTEventSource<RE::InputEvent*>*) override {
            if (!events || Settings::toggleKey == RE::BSKeyboardDevice::Keys::kNone)
                return RE::BSEventNotifyControl::kContinue;

            for (auto* event = *events; event; event = event->next) {
                if (event->GetDevice() != RE::INPUT_DEVICE::kKeyboard) continue;

                const auto* button = event->AsButtonEvent();
                if (!button) continue;

                const auto key = button->GetIDCode();
                if (Settings::toggleModifier != 0 && key == Settings::toggleModifier) {
                    _modifierHeld = button->IsPressed();
                    continue;
                }

                if (key != Settings::toggleKey || !button->IsDown()) continue;
                if (Settings::toggleModifier != 0 && !_modifierHeld) continue;
                if (IsInMenu()) continue;

                State::ToggleIcons();
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        bool _modifierHeld{false};
    };

    class DeathSink : public RE::BSTEventSink<RE::TESDeathEvent> {
    public:
        static DeathSink* GetSingleton() {
            static DeathSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* event,
                                              RE::BSTEventSource<RE::TESDeathEvent>*) override {
            if (!event || !event->actorDying) return RE::BSEventNotifyControl::kContinue;

            const auto formID = event->actorDying->GetFormID();
            const bool dying = !event->dead;
            State::RecordDeath(formID, IsPlayerOrAlly(event->actorKiller.get()), dying);

            // effects are still on the actor while it's dying, gone once it's dead
            if (dying && HasAshDeathScript(event->actorDying->As<RE::Actor>())) State::MarkWillTurnToAsh(formID);
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    class ActivateSink : public RE::BSTEventSink<RE::TESActivateEvent> {
    public:
        static ActivateSink* GetSingleton() {
            static ActivateSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* event,
                                              RE::BSTEventSource<RE::TESActivateEvent>*) override {
            if (!event || !event->objectActivated || !event->actionRef) return RE::BSEventNotifyControl::kContinue;

            if (!event->actionRef->IsPlayerRef()) return RE::BSEventNotifyControl::kContinue;

            const auto corpse = Corpse::Get(event->objectActivated.get());
            if (corpse && corpse->IsDead()) State::MarkSearched(corpse->GetFormID());

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    // looting without activating, e.g. QuickLoot
    class ContainerChangedSink : public RE::BSTEventSink<RE::TESContainerChangedEvent> {
    public:
        static ContainerChangedSink* GetSingleton() {
            static ContainerChangedSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* event,
                                              RE::BSTEventSource<RE::TESContainerChangedEvent>*) override {
            if (!event || event->itemCount <= 0) return RE::BSEventNotifyControl::kContinue;

            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player || event->newContainer != player->GetFormID()) return RE::BSEventNotifyControl::kContinue;

            const auto corpse = Corpse::Get(RE::TESForm::LookupByID<RE::TESObjectREFR>(event->oldContainer));
            if (corpse && corpse->IsDead()) State::MarkSearched(corpse->GetFormID());

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    void OnQuickLootTakeItem(QuickLoot::API::TakeItemEvent* a_event) {
        if (!a_event || !a_event->actor || !a_event->actor->IsPlayerRef()) return;

        const auto corpse = Corpse::Get(a_event->container.get().get());
        if (corpse && corpse->IsDead()) State::MarkSearched(corpse->GetFormID());
    }

    // FF form IDs get reused after deletion. Plugin refs are left alone in case this fires on cell unload.
    class FormDeleteSink : public RE::BSTEventSink<RE::TESFormDeleteEvent> {
    public:
        static FormDeleteSink* GetSingleton() {
            static FormDeleteSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESFormDeleteEvent* event,
                                              RE::BSTEventSource<RE::TESFormDeleteEvent>*) override {
            if (event && (event->formID >> 24) == 0xFF) State::Forget(event->formID);
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    // corpses that start dead refill on cell reset but never die again
    class ResetSink : public RE::BSTEventSink<RE::TESResetEvent> {
    public:
        static ResetSink* GetSingleton() {
            static ResetSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::TESResetEvent* event,
                                              RE::BSTEventSource<RE::TESResetEvent>*) override {
            if (event && event->object) State::Forget(event->object->GetFormID());
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    // kNewGame/kPostLoadGame aren't sent for coc from the main menu, the loading screen always closes
    class MenuSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static MenuSink* GetSingleton() {
            static MenuSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
                                              RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
            if (event && !event->opening && event->menuName == RE::LoadingMenu::MENU_NAME) IconMenu::Show();
            return RE::BSEventNotifyControl::kContinue;
        }
    };
}

namespace UCI::Events {
    void Register() {
        auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
        holder->AddEventSink<RE::TESDeathEvent>(DeathSink::GetSingleton());
        holder->AddEventSink<RE::TESActivateEvent>(ActivateSink::GetSingleton());
        holder->AddEventSink<RE::TESContainerChangedEvent>(ContainerChangedSink::GetSingleton());
        holder->AddEventSink<RE::TESFormDeleteEvent>(FormDeleteSink::GetSingleton());
        holder->AddEventSink<RE::TESResetEvent>(ResetSink::GetSingleton());

        RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(MenuSink::GetSingleton());

        auto* input = RE::BSInputDeviceManager::GetSingleton();
        if (input) input->AddEventSink(InputSink::GetSingleton());

        const bool quickLoot =
            QuickLoot::API::QuickLootAPI::Init(Plugin::NAME.data(), QuickLoot::API::ApiVersion::kV20);
        if (quickLoot) QuickLoot::API::QuickLootAPI::RegisterTakeItemHandler(OnQuickLootTakeItem);

        SKSE::log::info("Event sinks registered (input: {}, QuickLoot IE: {})", input ? "yes" : "no",
                        quickLoot ? "yes" : "no");
    }
}
