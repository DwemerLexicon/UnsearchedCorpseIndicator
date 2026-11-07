#include "Settings.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>

#include "PCH.h"

namespace UCI::Settings {
    float delay{0.0f};
    bool allCorpses{true};
    std::uint32_t toggleKey{38};
    std::uint32_t toggleModifier{0};

    float fadeStartDistance{1000.0f};
    float fadeMaxDistance{1500.0f};
    std::uint32_t maxIcons{48};
    float scale{1.0f};
    float opacity{100.0f};
    bool fixedSize{false};
    std::optional<std::array<float, 3>> color;

    namespace {
        constexpr std::uint32_t kMostIcons = 100;

        std::string Trim(std::string value) {
            const auto first = value.find_first_not_of(" \t");
            if (first == std::string::npos) {
                return {};
            }

            const auto last = value.find_last_not_of(" \t\r\n");
            return value.substr(first, last - first + 1);
        }

        bool HasOnlyTrailingWhitespace(const std::string& value, std::size_t offset) {
            return value.find_first_not_of(" \t\r\n", offset) == std::string::npos;
        }

        std::optional<float> TryParseFloat(const std::string& value) {
            try {
                std::size_t parsedChars = 0;
                const auto parsed = std::stof(value, &parsedChars);
                if (!HasOnlyTrailingWhitespace(value, parsedChars)) {
                    return std::nullopt;
                }

                return parsed;
            } catch (...) {
                return std::nullopt;
            }
        }

        std::optional<std::uint32_t> TryParseUInt32(const std::string& value) {
            if (value.empty() || value.front() == '-') {
                return std::nullopt;
            }

            try {
                std::size_t parsedChars = 0;
                const auto parsed = std::stoul(value, &parsedChars);
                if (!HasOnlyTrailingWhitespace(value, parsedChars) ||
                    parsed > std::numeric_limits<std::uint32_t>::max()) {
                    return std::nullopt;
                }

                return static_cast<std::uint32_t>(parsed);
            } catch (...) {
                return std::nullopt;
            }
        }

        bool ParseBool(const std::string& value) { return value == "1" || value == "true"; }

        // A hex colour such as FF4444 or #FF4444. Blank means no tint.
        std::optional<std::array<float, 3>> TryParseColor(std::string value) {
            if (!value.empty() && value.front() == '#') {
                value.erase(0, 1);
            }
            if (value.size() != 6 || !std::ranges::all_of(value, [](unsigned char c) { return std::isxdigit(c); })) {
                return std::nullopt;
            }

            std::array<float, 3> rgb{};
            for (std::size_t i = 0; i < rgb.size(); ++i) {
                rgb[i] = static_cast<float>(std::stoul(value.substr(i * 2, 2), nullptr, 16)) / 255.0f;
            }
            return rgb;
        }
    }

    void Load() {
        const auto path = std::filesystem::path{"Data/SKSE/Plugins/UnsearchedCorpsesIndicator.ini"};
        if (!std::filesystem::exists(path)) {
            SKSE::log::warn("{} not found, using defaults", path.string());
            return;
        }

        std::ifstream file(path);
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;

            auto eq = line.find('=');
            if (eq == std::string::npos) continue;

            auto key = line.substr(0, eq);
            auto val = line.substr(eq + 1);

            key = Trim(key);
            val = Trim(val);

            if (key == "fDelay") {
                if (auto parsed = TryParseFloat(val)) {
                    delay = std::max(0.0f, *parsed);
                }
            } else if (key == "bAllCorpses") {
                allCorpses = ParseBool(val);
            } else if (key == "iToggleKey") {
                if (auto parsed = TryParseUInt32(val)) {
                    toggleKey = *parsed;
                }
            } else if (key == "iToggleModifier") {
                if (auto parsed = TryParseUInt32(val)) {
                    toggleModifier = *parsed;
                }
            } else if (key == "fFadeStartDistance") {
                if (auto parsed = TryParseFloat(val)) {
                    fadeStartDistance = std::max(0.0f, *parsed);
                }
            } else if (key == "fFadeMaxDistance") {
                if (auto parsed = TryParseFloat(val)) {
                    fadeMaxDistance = std::max(0.0f, *parsed);
                }
            } else if (key == "iMaxIcons") {
                if (auto parsed = TryParseUInt32(val)) {
                    maxIcons = std::clamp(*parsed, 1u, kMostIcons);
                }
            } else if (key == "fScale") {
                if (auto parsed = TryParseFloat(val)) {
                    scale = std::max(0.0f, *parsed);
                }
            } else if (key == "iOpacity") {
                if (auto parsed = TryParseUInt32(val)) {
                    opacity = static_cast<float>(std::min(*parsed, 100u));
                }
            } else if (key == "bFixedSize") {
                fixedSize = ParseBool(val);
            } else if (key == "sColor") {
                color = TryParseColor(val);
                if (!color && !val.empty()) {
                    SKSE::log::warn("sColor = {} isn't a colour like FF4444, so the icon isn't tinted", val);
                }
            }
        }

        fadeStartDistance = std::min(fadeStartDistance, fadeMaxDistance);

        SKSE::log::info(
            "Settings: fDelay={}, bAllCorpses={}, iToggleKey={}, iToggleModifier={}, fFadeStartDistance={}, "
            "fFadeMaxDistance={}, iMaxIcons={}, fScale={}, iOpacity={}, bFixedSize={}, sColor={}",
            delay, allCorpses, toggleKey, toggleModifier, fadeStartDistance, fadeMaxDistance, maxIcons, scale, opacity,
            fixedSize, color ? "set" : "none");
    }
}
