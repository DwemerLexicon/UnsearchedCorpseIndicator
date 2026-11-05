#pragma once

#include <optional>
#include <vector>

// Per-corpse tracking shared by the event sinks, the SIF conditions and the cosave.
// Every function is thread-safe.
namespace UCI::State {
    // Seconds since the plugin loaded; used to time fDelay.
    float GetRealTime();

    // a_dying is true for the first death event (the actor started dying), false for the second
    // (fully dead). The first one starts a fresh corpse: any searched mark left on a reused form ID
    // or on a resurrected actor is cleared.
    void RecordDeath(RE::FormID a_formID, bool a_killedByPlayer, bool a_dying);
    void MarkSearched(RE::FormID a_formID);
    void Forget(RE::FormID a_formID);

    [[nodiscard]] bool IsSearched(RE::FormID a_formID);
    [[nodiscard]] bool WasKilledByPlayer(RE::FormID a_formID);
    // Empty when the death happened before this session (for example in a loaded save).
    [[nodiscard]] std::optional<float> SecondsSinceDeath(RE::FormID a_formID);
    // Seconds since this was first called for the reference this session; the first call returns 0.
    [[nodiscard]] float SecondsSinceFirstSeen(RE::FormID a_formID);

    [[nodiscard]] bool IconsEnabled();
    void ToggleIcons();

    [[nodiscard]] std::vector<RE::FormID> GetSearched();
    [[nodiscard]] std::vector<RE::FormID> GetKilledByPlayer();
    void Restore(std::vector<RE::FormID> a_searched, std::vector<RE::FormID> a_killedByPlayer);
    void Reset();
}
