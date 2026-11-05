#include "PCH.h"
#include "Conditions.h"
#include "SIF_API.h"
#include "Settings.h"
#include "State.h"

#include <algorithm>

namespace UCI {
    namespace {
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

    class HasLootCondition : public SIF::ICondition {
    public:
        bool Match(RE::TESObjectREFR* ref) const override {
            if (!ref || !State::IconsEnabled()) return false;

            // SIF checks hasLoot before isDead, so skip the inventory scan for the living.
            const auto* actor = ref->As<RE::Actor>();
            if (!actor || !actor->IsDead()) return false;

            return MatchesCorpseScope(ref->GetFormID()) && HasLootableItems(ref);
        }
    };

    class NotSearchedCondition : public SIF::ICondition {
    public:
        bool Match(RE::TESObjectREFR* ref) const override {
            return ref && !State::IsSearched(ref->GetFormID());
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
