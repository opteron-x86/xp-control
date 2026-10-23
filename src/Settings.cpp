#include "Settings.h"

#include <charconv>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <locale>
#include <sstream>
#include <system_error>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace xp_control {
namespace {
std::string_view Trim(std::string_view value) {
    constexpr std::string_view whitespace = " \t\r\n";
    const auto first = value.find_first_not_of(whitespace);
    if (first == std::string_view::npos) return {};
    return value.substr(first, value.find_last_not_of(whitespace) - first + 1);
}
bool Number(std::string_view value, float& output) {
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), output);
    return error == std::errc{} && end == value.data() + value.size() && std::isfinite(output);
}
}

SettingsResult ParseSettings(std::string_view text) {
    if (text.starts_with("\xEF\xBB\xBF")) text.remove_prefix(3);
    Curve curve;
    bool inCurve = false, haveBase = false, haveMult = false;
    while (!text.empty()) {
        const auto newline = text.find('\n');
        auto line = Trim(text.substr(0, newline));
        text = newline == std::string_view::npos ? std::string_view{} : text.substr(newline + 1);
        line = Trim(line.substr(0, line.find_first_of(";#")));
        if (line.empty()) continue;
        if (line.front() == '[' && line.back() == ']') {
            inCurve = line == "[Curve]";
            continue;
        }
        if (!inCurve) continue;
        const auto equals = line.find('=');
        if (equals == std::string_view::npos) return {{}, false, "Expected key=value in [Curve]."};
        const auto key = Trim(line.substr(0, equals));
        const auto value = Trim(line.substr(equals + 1));
        if (key == "BaseXP") {
            if (haveBase || !Number(value, curve.base)) return {{}, false, "Invalid or repeated BaseXP."};
            haveBase = true;
        } else if (key == "XPPerLevel") {
            if (haveMult || !Number(value, curve.mult)) return {{}, false, "Invalid or repeated XPPerLevel."};
            haveMult = true;
        } else {
            return {{}, false, "Unknown setting in [Curve]: " + std::string(key)};
        }
    }
    if (!haveBase || !haveMult) return {{}, false, "Both BaseXP and XPPerLevel are required."};
    if (!Valid(curve)) return {{}, false, "BaseXP must be 1..100000; XPPerLevel must be 0..10000."};
    return {curve, true, {}};
}

std::string FormatSettings(Curve curve) {
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << "; XP Control\n"
        "; XP required = BaseXP + (current level * XPPerLevel)\n"
        "; These settings apply to every character using this mod profile.\n"
        "[Curve]\n" << std::setprecision(9) << "BaseXP=" << curve.base <<
        "\nXPPerLevel=" << curve.mult << '\n';
    return output.str();
}

SettingsResult ReadSettings(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return {{}, false, "Settings unavailable. Using defaults."};
    std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) return {{}, false, "Settings file read failed."};
    return ParseSettings(text);
}

bool WriteSettings(const std::filesystem::path& path, Curve curve, std::string& error) {
    if (!Valid(curve)) { error = "Invalid XP curve."; return false; }
    auto temporary = path;
    temporary += ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) { error = "Settings file creation failed."; return false; }
        output << FormatSettings(curve);
        output.flush();
        if (!output) { error = "Settings file write failed."; return false; }
        output.close();
        if (!output) { error = "Settings file close failed."; return false; }
    }
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "Settings file update failed (Windows error " + std::to_string(GetLastError()) + ").";
        return false;
    }
#else
    std::error_code ec;
    std::filesystem::rename(temporary, path, ec);
    if (ec) { error = "Settings file update failed: " + ec.message(); return false; }
#endif
    return true;
}
}
