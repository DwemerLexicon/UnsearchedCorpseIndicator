#pragma once

// actor and ash pile point at each other with ExtraAshPileRef, the loot stays on the actor
namespace UCI::Corpse {
    inline RE::NiPointer<RE::Actor> Get(RE::TESObjectREFR* a_ref) {
        if (!a_ref) return nullptr;

        if (auto* actor = a_ref->As<RE::Actor>())
            return RE::NiPointer<RE::Actor>(actor);

        if (!a_ref->extraList.HasType(RE::ExtraDataType::kAshPileRef)) return nullptr;

        const auto owner = a_ref->extraList.GetAshPileRef().get();
        return RE::NiPointer<RE::Actor>(owner ? owner->As<RE::Actor>() : nullptr);
    }

    inline bool HasAshPile(const RE::Actor* a_actor) {
        return a_actor && a_actor->extraList.HasType(RE::ExtraDataType::kAshPileRef);
    }
}
