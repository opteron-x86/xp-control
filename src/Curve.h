#pragma once

#include <cmath>
#include <cstdint>
#include <string_view>

namespace xp_control {
struct Curve {
    float base{375.0F};
    float mult{15.0F};
    bool operator==(const Curve&) const = default;
};

inline constexpr Curve kVanilla{75.0F, 25.0F};
inline constexpr float kMaximumBase = 100000.0F;
inline constexpr float kMaximumMult = 10000.0F;
inline constexpr std::uint32_t kMaximumLevel = 65535;

[[nodiscard]] inline bool Valid(Curve curve) {
    return std::isfinite(curve.base) && std::isfinite(curve.mult) &&
        curve.base >= 1.0F && curve.base <= kMaximumBase &&
        curve.mult >= 0.0F && curve.mult <= kMaximumMult;
}

[[nodiscard]] inline float Requirement(Curve curve, std::uint32_t level) {
    return curve.base + curve.mult * static_cast<float>(level);
}

// Total XP spent to arrive at targetLevel, beginning at level 1.
[[nodiscard]] inline double TotalToLevel(Curve curve, std::uint32_t targetLevel) {
    if (targetLevel <= 1) return 0.0;
    const auto n = static_cast<double>(targetLevel - 1);
    return n * curve.base + curve.mult * n * (n + 1.0) / 2.0;
}

enum class Refresh {
    invalid,
    unchanged,
    update,
    pendingLevel,
    wouldCreateLevel
};

struct ThresholdPlan {
    Refresh action{Refresh::invalid};
    float threshold{};
};

// Calculate a threshold refresh while preserving pending-level eligibility.
// Deferred thresholds remain active until a level-up completes.
[[nodiscard]] inline ThresholdPlan PlanThreshold(
    Curve curve, std::uint32_t level, float xp, float currentThreshold) {
    if (!Valid(curve) || level < 1 || level > kMaximumLevel ||
        !std::isfinite(xp) || xp < 0.0F ||
        !std::isfinite(currentThreshold) || currentThreshold <= 0.0F) {
        return {};
    }
    const auto next = Requirement(curve, level);
    if (next == currentThreshold) return {Refresh::unchanged, currentThreshold};
    if (xp >= currentThreshold) return {Refresh::pendingLevel, currentThreshold};
    if (xp >= next) return {Refresh::wouldCreateLevel, currentThreshold};
    return {Refresh::update, next};
}

struct Preset { std::string_view name; Curve curve; };
inline constexpr Preset kPresets[]{
    {"Standard", {375.0F, 15.0F}},
    {"Vanilla", kVanilla},
    {"Moderate", {225.0F, 15.0F}},
    {"Balanced", {300.0F, 10.0F}},
    {"Gentle", {375.0F, 5.0F}},
    {"Flat", {450.0F, 0.0F}},
    {"Slower flat", {700.0F, 0.0F}}
};
}
