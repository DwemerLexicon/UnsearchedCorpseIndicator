#include "PCH.h"
#include "State.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace UCI::State {
    namespace {
        std::mutex lock;
        std::unordered_map<RE::FormID, float> deathTimes;
        std::unordered_set<RE::FormID> killedByPlayer;
        std::unordered_set<RE::FormID> searched;
        std::unordered_map<RE::FormID, float> firstSeen;
        std::unordered_set<RE::FormID> willTurnToAsh;
        std::atomic<bool> iconsEnabled{ true };
    }

    float GetRealTime() {
        using clock = std::chrono::steady_clock;
        static const auto start = clock::now();
        return std::chrono::duration<float>(clock::now() - start).count();
    }

    void RecordDeath(RE::FormID a_formID, bool a_killedByPlayer, bool a_dying) {
        const auto now = GetRealTime();
        std::scoped_lock guard{ lock };
        if (a_dying) {
            searched.erase(a_formID);
            killedByPlayer.erase(a_formID);
            willTurnToAsh.erase(a_formID);
            deathTimes.insert_or_assign(a_formID, now);
        } else {
            deathTimes.try_emplace(a_formID, now);
        }

        if (a_killedByPlayer) {
            killedByPlayer.insert(a_formID);
        }
    }

    void MarkSearched(RE::FormID a_formID) {
        std::scoped_lock guard{ lock };
        searched.insert(a_formID);
        deathTimes.erase(a_formID);
        killedByPlayer.erase(a_formID);
    }

    void MarkWillTurnToAsh(RE::FormID a_formID) {
        std::scoped_lock guard{ lock };
        willTurnToAsh.insert(a_formID);
    }

    void Forget(RE::FormID a_formID) {
        std::scoped_lock guard{ lock };
        searched.erase(a_formID);
        deathTimes.erase(a_formID);
        killedByPlayer.erase(a_formID);
        firstSeen.erase(a_formID);
        willTurnToAsh.erase(a_formID);
    }

    bool IsSearched(RE::FormID a_formID) {
        std::scoped_lock guard{ lock };
        return searched.contains(a_formID);
    }

    bool WasKilledByPlayer(RE::FormID a_formID) {
        std::scoped_lock guard{ lock };
        return killedByPlayer.contains(a_formID);
    }

    bool WillTurnToAsh(RE::FormID a_formID) {
        std::scoped_lock guard{ lock };
        return willTurnToAsh.contains(a_formID);
    }

    std::optional<float> SecondsSinceDeath(RE::FormID a_formID) {
        std::scoped_lock guard{ lock };
        const auto it = deathTimes.find(a_formID);
        if (it == deathTimes.end()) {
            return std::nullopt;
        }
        return GetRealTime() - it->second;
    }

    float SecondsSinceFirstSeen(RE::FormID a_formID) {
        const auto now = GetRealTime();
        std::scoped_lock guard{ lock };
        const auto [it, inserted] = firstSeen.try_emplace(a_formID, now);
        return now - it->second;
    }

    bool IconsEnabled() {
        return iconsEnabled.load(std::memory_order_relaxed);
    }

    void ToggleIcons() {
        iconsEnabled.store(!iconsEnabled.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    std::vector<RE::FormID> GetSearched() {
        std::scoped_lock guard{ lock };
        return { searched.begin(), searched.end() };
    }

    std::vector<RE::FormID> GetKilledByPlayer() {
        std::scoped_lock guard{ lock };
        return { killedByPlayer.begin(), killedByPlayer.end() };
    }

    void Restore(std::vector<RE::FormID> a_searched, std::vector<RE::FormID> a_killedByPlayer) {
        std::scoped_lock guard{ lock };
        searched.insert(a_searched.begin(), a_searched.end());
        killedByPlayer.insert(a_killedByPlayer.begin(), a_killedByPlayer.end());
    }

    void Reset() {
        {
            std::scoped_lock guard{ lock };
            deathTimes.clear();
            killedByPlayer.clear();
            searched.clear();
            firstSeen.clear();
            willTurnToAsh.clear();
        }
        iconsEnabled.store(true, std::memory_order_relaxed);
    }
}
