#pragma once
#include "Curve.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace xp_control {
struct SettingsResult {
    Curve curve{};
    bool ok{};
    std::string message;
};

[[nodiscard]] SettingsResult ParseSettings(std::string_view text);
[[nodiscard]] std::string FormatSettings(Curve curve);
[[nodiscard]] SettingsResult ReadSettings(const std::filesystem::path& path);
[[nodiscard]] bool WriteSettings(const std::filesystem::path& path, Curve curve, std::string& error);
}
