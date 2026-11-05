#include "PCH.h"
#include "Settings.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>

namespace UCI::Settings {
    float delay{ 0.0f };
    bool allCorpses{ true };
    std::uint32_t toggleKey{ 38 };

    namespace {
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
    }

    void Load() {
        const auto path = std::filesystem::path{ "Data/SKSE/Plugins/UnsearchedCorpsesIndicator.ini" };
        if (!std::filesystem::exists(path)) {
            REX::WARN("{} not found, using defaults", path.string());
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
                allCorpses = (val == "1" || val == "true");
            } else if (key == "iToggleKey") {
                if (auto parsed = TryParseUInt32(val)) {
                    toggleKey = *parsed;
                }
            }
        }

        REX::INFO("Settings: fDelay={}, bAllCorpses={}, iToggleKey={}", delay, allCorpses, toggleKey);
    }
}
