#pragma once

// When a body turns to ash (Disintegrate, sun damage, ghosts), an ash pile takes its place, and
// the engine links the two with ExtraAshPileRef: on the actor it points to the pile, on the pile it
// points back to the actor. Searching the pile opens the actor's inventory, so the actor is the
// corpse whichever of the two we're given.
namespace UCI::Corpse {
    // The actor itself, the dead actor an ash pile belongs to, or null for anything else.
    inline RE::NiPointer<RE::Actor> Get(RE::TESObjectREFR* a_ref) {
        if (!a_ref) return nullptr;

        if (auto* actor = a_ref->As<RE::Actor>())
            return RE::NiPointer<RE::Actor>(actor);

        // HasType only reads a bitfield, so levers, shrines and other activators SIF asks about
        // every frame are turned away before any lookup.
        if (!a_ref->extraList.HasType(RE::ExtraDataType::kAshPileRef)) return nullptr;

        const auto owner = a_ref->extraList.GetAshPileRef().get();
        return RE::NiPointer<RE::Actor>(owner ? owner->As<RE::Actor>() : nullptr);
    }

    // The body has turned to ash, so the icon belongs on the pile.
    inline bool HasAshPile(const RE::Actor* a_actor) {
        return a_actor && a_actor->extraList.HasType(RE::ExtraDataType::kAshPileRef);
    }
}
