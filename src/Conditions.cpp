#include "PCH.h"
#include "Conditions.h"
#include "Corpse.h"
#include "SIF_API.h"
#include "Settings.h"
#include "State.h"

#include <algorithm>

namespace UCI {
    namespace {
        // Seconds an ash pile has to exist before its icon shows, so the icon doesn't appear
        // before the pile has finished forming on the ground.
        constexpr float kAshPileDelay = 1.0f;

        bool MatchesCorpseScope(RE::FormID a_formID) {
            if (!Settings::allCorpses && !State::WasKilledByPlayer(a_formID)) {
                return false;
            }

            if (Settings::delay > 0.0f) {
                const auto elapsed = State::SecondsSinceDeath(a_formID);
                if (elapsed && *elapsed < Settings::delay) {
                    return false;
                }
            }

            return true;
        }

        // Anything the player would see when searching the corpse. GetInventory() adds base
        // container items to what was added or removed since, so items the actor always carries
        // count too, and items already taken don't.
        bool HasLootableItems(RE::TESObjectREFR* a_ref) {
            const auto inventory = a_ref->GetInventory([](RE::TESBoundObject& a_object) {
                return a_object.GetPlayable() && !a_object.Is(RE::FormType::LeveledItem);
            });

            return std::ranges::any_of(inventory, [](const auto& a_item) {
                return a_item.second.first > 0;
            });
        }
    }

    // The rule has no formType, so SIF asks about every actor, door, container and activator it
    // tracks. Both conditions judge the corpse behind a body or an ash pile and reject the rest.
    // SIF checks hasLoot first, so it turns everything else away cheaply.
    class HasLootCondition : public SIF::ICondition {
    public:
        bool Match(RE::TESObjectREFR* ref) const override {
            if (!ref || !State::IconsEnabled()) return false;

            const auto corpse = Corpse::Get(ref);
            if (!corpse || !corpse->IsDead()) return false;

            if (corpse.get() == ref) {
                // Once the body has turned to ash, only the pile gets the icon.
                if (Corpse::HasAshPile(corpse.get())) return false;
            } else if (State::SecondsSinceFirstSeen(ref->GetFormID()) < kAshPileDelay) {
                return false;
            }

            return MatchesCorpseScope(corpse->GetFormID()) && HasLootableItems(corpse.get());
        }
    };

    class NotSearchedCondition : public SIF::ICondition {
    public:
        bool Match(RE::TESObjectREFR* ref) const override {
            const auto corpse = Corpse::Get(ref);
            return corpse && !State::IsSearched(corpse->GetFormID());
        }
    };
}

namespace UCI::Conditions {
    void Register() {
        const bool listening = SIF::ListenForRegistration([](SKSE::MessagingInterface::Message* msg) {
            if (msg->type != SIF::kMessage_GetAPI) return;

            auto* api = static_cast<SIF::IAPI*>(msg->data);
            if (!api) {
                REX::ERROR("SIF sent an empty API pointer");
                return;
            }

            const bool hasLoot = api->RegisterCondition("hasLoot", [](const Json::Value&, RE::FormType) {
                return std::make_unique<HasLootCondition>();
            });
            const bool notSearched = api->RegisterCondition("notSearched", [](const Json::Value&, RE::FormType) {
                return std::make_unique<NotSearchedCondition>();
            });
            REX::INFO("SIF API v{}: hasLoot {}, notSearched {}", api->GetVersion(),
                hasLoot ? "registered" : "FAILED", notSearched ? "registered" : "FAILED");
        });

        if (!listening) {
            REX::ERROR("Status Indicator Framework is not loaded, so no corpse icons will show");
        }
    }
}
