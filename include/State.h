#pragma once

#include <optional>
#include <vector>

// all thread-safe
namespace UCI::State {
    float GetRealTime();

    // death event fires twice, dying then dead. Dying clears old marks (reused IDs, resurrected actors)
    void RecordDeath(RE::FormID a_formID, bool a_killedByPlayer, bool a_dying);
    void MarkSearched(RE::FormID a_formID);
    void MarkWillTurnToAsh(RE::FormID a_formID);
    void Forget(RE::FormID a_formID);

    [[nodiscard]] bool IsSearched(RE::FormID a_formID);
    [[nodiscard]] bool WasKilledByPlayer(RE::FormID a_formID);
    [[nodiscard]] bool WillTurnToAsh(RE::FormID a_formID);
    // empty if it died before this session
    [[nodiscard]] std::optional<float> SecondsSinceDeath(RE::FormID a_formID);
    // first call returns 0
    [[nodiscard]] float SecondsSinceFirstSeen(RE::FormID a_formID);

    [[nodiscard]] bool IconsEnabled();
    void ToggleIcons();

    [[nodiscard]] std::vector<RE::FormID> GetSearched();
    [[nodiscard]] std::vector<RE::FormID> GetKilledByPlayer();
    void Restore(std::vector<RE::FormID> a_searched, std::vector<RE::FormID> a_killedByPlayer);
    void Reset();
}
