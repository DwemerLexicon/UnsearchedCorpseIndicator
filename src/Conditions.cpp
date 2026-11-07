#include "Conditions.h"

#include <algorithm>

#include "Corpse.h"
#include "PCH.h"
#include "Settings.h"
#include "State.h"

namespace UCI {
    namespace {
        constexpr float kAshPileDelay = 1.0f;    // let the pile form first
        constexpr float kBodyDelay = 1.0f;       // stops the icon popping up the instant something dies
        constexpr float kAshPileTimeout = 5.0f;  // ash spawn piles show up late, give up waiting after this

        bool IsTurningToAsh(const RE::Actor* a_actor) {
            const auto stage = a_actor->GetActorRuntimeData().criticalStage.get();
            return stage == RE::ACTOR_CRITICAL_STAGE::kDisintegrateStart ||
                   stage == RE::ACTOR_CRITICAL_STAGE::kDisintegrateEnd;
        }

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

        // GetInventory includes base container items so default gear counts too
        bool HasLootableItems(RE::TESObjectREFR* a_ref) {
            const auto inventory = a_ref->GetInventory([](RE::TESBoundObject& a_object) {
                return a_object.GetPlayable() && !a_object.Is(RE::FormType::LeveledItem);
            });

            return std::ranges::any_of(inventory, [](const auto& a_item) { return a_item.second.first > 0; });
        }
    }
}

namespace UCI::Conditions {
    bool ShowIcon(RE::TESObjectREFR* a_ref) {
        const auto corpse = Corpse::Get(a_ref);
        if (!corpse || !corpse->IsDead()) return false;

        const auto formID = corpse->GetFormID();
        if (corpse.get() == a_ref) {
            if (Corpse::HasAshPile(corpse.get()) || IsTurningToAsh(corpse.get())) return false;

            // no death time if it died before this session, just show it
            if (const auto sinceDeath = State::SecondsSinceDeath(formID)) {
                if (*sinceDeath < kBodyDelay) return false;
                if (*sinceDeath < kAshPileTimeout && State::WillTurnToAsh(formID)) return false;
            }
        } else if (State::SecondsSinceFirstSeen(a_ref->GetFormID()) < kAshPileDelay) {
            return false;
        }

        return !State::IsSearched(formID) && MatchesCorpseScope(formID) && HasLootableItems(corpse.get());
    }
}
