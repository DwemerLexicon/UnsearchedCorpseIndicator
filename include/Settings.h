#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace UCI::Settings {
    extern float delay;
    extern bool allCorpses;
    extern std::uint32_t toggleKey;
    extern std::uint32_t toggleModifier;  // 0 = no modifier

    // Icon appearance
    extern float fadeStartDistance;
    extern float fadeMaxDistance;
    extern std::uint32_t maxIcons;
    extern float scale;    // 1.0 = normal size
    extern float opacity;  // 0 to 100
    extern bool fixedSize;
    extern std::optional<std::array<float, 3>> color;  // red, green and blue from 0 to 1; empty = no tint

    void Load();
}
