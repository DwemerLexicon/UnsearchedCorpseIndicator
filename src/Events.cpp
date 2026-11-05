#include "PCH.h"
#include "Events.h"
#include "Settings.h"
#include "State.h"

namespace UCI {
    namespace {
        // The player, a follower, or something the player commands (summons, reanimated thralls).
        bool IsPlayerOrAlly(RE::TESObjectREFR* a_killer) {
            if (!a_killer) return false;
            if (a_killer->IsPlayerRef()) return true;

            const auto* actor = a_killer->As<RE::Actor>();
            if (!actor) return false;
            if (actor->IsPlayerTeammate()) return true;

            const auto commander = actor->GetCommandingActor();
            return commander && commander->IsPlayerRef();
        }

        // Typing in the console or a menu shouldn't toggle the icons.
        bool IsInMenu() {
            auto* ui = RE::UI::GetSingleton();
            return !ui || ui->GameIsPaused() || ui->IsMenuOpen(RE::Console::MENU_NAME);
        }
    }

    class InputSink : public RE::BSTEventSink<RE::InputEvent*> {
    public:
        static InputSink* GetSingleton() {
            static InputSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* events,
            RE::BSTEventSource<RE::InputEvent*>*) override
        {
            if (!events || Settings::toggleKey == RE::BSKeyboardDevice::Keys::kNone)
                return RE::BSEventNotifyControl::kContinue;

            for (auto* event = *events; event; event = event->next) {
                if (event->GetDevice() != RE::INPUT_DEVICE::kKeyboard)
                    continue;

                const auto* button = event->AsButtonEvent();
                if (!button)
                    continue;

                const auto key = button->GetIDCode();
                if (Settings::toggleModifier != 0 && key == Settings::toggleModifier) {
                    _modifierHeld = button->IsPressed();
                    continue;
                }

                if (key != Settings::toggleKey || !button->IsDown())
                    continue;
                if (Settings::toggleModifier != 0 && !_modifierHeld)
                    continue;
                if (IsInMenu())
                    continue;

                State::ToggleIcons();
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        bool _modifierHeld{ false };
    };

    class DeathSink : public RE::BSTEventSink<RE::TESDeathEvent> {
    public:
        static DeathSink* GetSingleton() {
            static DeathSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::TESDeathEvent* event,
            RE::BSTEventSource<RE::TESDeathEvent>*) override
        {
            if (!event || !event->actorDying)
                return RE::BSEventNotifyControl::kContinue;

            State::RecordDeath(
                event->actorDying->GetFormID(),
                IsPlayerOrAlly(event->actorKiller.get()),
                !event->dead);
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    class ActivateSink : public RE::BSTEventSink<RE::TESActivateEvent> {
    public:
        static ActivateSink* GetSingleton() {
            static ActivateSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::TESActivateEvent* event,
            RE::BSTEventSource<RE::TESActivateEvent>*) override
        {
            if (!event || !event->objectActivated || !event->actionRef)
                return RE::BSEventNotifyControl::kContinue;

            if (!event->actionRef->IsPlayerRef())
                return RE::BSEventNotifyControl::kContinue;

            const auto* actor = event->objectActivated->As<RE::Actor>();
            if (actor && actor->IsDead())
                State::MarkSearched(actor->GetFormID());

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    // Covers looting without activating the corpse, such as with QuickLoot.
    class ContainerChangedSink : public RE::BSTEventSink<RE::TESContainerChangedEvent> {
    public:
        static ContainerChangedSink* GetSingleton() {
            static ContainerChangedSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::TESContainerChangedEvent* event,
            RE::BSTEventSource<RE::TESContainerChangedEvent>*) override
        {
            if (!event || event->itemCount <= 0)
                return RE::BSEventNotifyControl::kContinue;

            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player || event->newContainer != player->GetFormID())
                return RE::BSEventNotifyControl::kContinue;

            const auto* actor = RE::TESForm::LookupByID<RE::Actor>(event->oldContainer);
            if (actor && actor->IsDead())
                State::MarkSearched(actor->GetFormID());

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    // References created at runtime (FF form IDs) get deleted and their IDs reused, so drop what we
    // know about one once it is gone. References from plugins keep their IDs, so they're left alone
    // in case the event also fires when their cell is unloaded.
    class FormDeleteSink : public RE::BSTEventSink<RE::TESFormDeleteEvent> {
    public:
        static FormDeleteSink* GetSingleton() {
            static FormDeleteSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::TESFormDeleteEvent* event,
            RE::BSTEventSource<RE::TESFormDeleteEvent>*) override
        {
            if (event && (event->formID >> 24) == 0xFF)
                State::Forget(event->formID);
            return RE::BSEventNotifyControl::kContinue;
        }
    };

    // A cell reset puts its references back as the plugin placed them. Corpses that start dead
    // refill their inventory but never die again, so their searched mark has to be cleared here.
    class ResetSink : public RE::BSTEventSink<RE::TESResetEvent> {
    public:
        static ResetSink* GetSingleton() {
            static ResetSink instance;
            return &instance;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::TESResetEvent* event,
            RE::BSTEventSource<RE::TESResetEvent>*) override
        {
            if (event && event->object)
                State::Forget(event->object->GetFormID());
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

        auto* input = RE::BSInputDeviceManager::GetSingleton();
        if (input)
            input->AddEventSink(InputSink::GetSingleton());

        REX::INFO("Event sinks registered (input: {})", input ? "yes" : "no");
    }
}
