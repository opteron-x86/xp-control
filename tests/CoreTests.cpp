#include "Curve.h"
#include "Session.h"
#include "Settings.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

namespace {
unsigned checks = 0;
void Check(bool condition, const char* description) {
    ++checks;
    if (!condition) throw std::runtime_error(description);
}
void Run() {
    using namespace xp_control;
    Check(Requirement(kVanilla, 1) == 100.0F, "Vanilla first level costs 100");
    Check(Requirement(Curve{}, 1) == 390.0F, "Standard curve first level costs 390");
    Check(Requirement(Curve{}, 50) == 1125.0F, "Standard curve at level 50");
    Check(TotalToLevel(kVanilla, 1) == 0.0, "Cumulative XP starts at zero");
    Check(TotalToLevel(kVanilla, 3) == 225.0, "Cumulative sums levels one and two");
    Check(TotalToLevel(Curve{}, 10) == 4050.0, "Cumulative standard curve");
    Check(!Valid({0, 0}) && !Valid({10, -1}), "Reject nonpositive thresholds");
    Check(!Valid({std::numeric_limits<float>::quiet_NaN(), 15}), "Reject NaN");
    Check(!Valid({375, std::numeric_limits<float>::infinity()}), "Reject infinity");

    Check(PlanThreshold({375, 15}, 1, 20, 100).action == Refresh::update, "Safe live increase");
    Check(PlanThreshold({75, 25}, 1, 20, 390).threshold == 100, "Safe live decrease");
    Check(PlanThreshold({375, 15}, 1, 100, 100).action == Refresh::pendingLevel, "Preserve exact pending boundary");
    Check(PlanThreshold({375, 15}, 1, 100000, 100).threshold == 100, "Preserve banked XP threshold");
    Check(PlanThreshold({75, 25}, 1, 100, 390).action == Refresh::wouldCreateLevel, "Preserve pending-level eligibility at equality");
    Check(PlanThreshold({75, 25}, 1, 200, 390).threshold == 390, "Preserve pending-level eligibility above boundary");
    Check(PlanThreshold({375, 15}, 1, 20, 390).action == Refresh::unchanged, "Idempotent refresh");
    Check(PlanThreshold({375, 15}, 0, 0, 100).action == Refresh::invalid, "Reject uninitialized level");
    Check(PlanThreshold({375, 15}, 1, -1, 100).action == Refresh::invalid, "Reject invalid XP");
    Check(PlanThreshold({375, 15}, 1, 0, 0).action == Refresh::invalid, "Reject uninitialized threshold");
    Check(PlanThreshold({375, 15}, 1, std::numeric_limits<float>::infinity(), 100).action == Refresh::invalid, "Reject nonfinite XP");

    std::mt19937 random(12345);
    std::uniform_real_distribution<float> base(1, kMaximumBase), mult(0, kMaximumMult), fraction(0, 2);
    std::uniform_int_distribution<unsigned> level(1, kMaximumLevel);
    for (unsigned i = 0; i < 10000; ++i) {
        const Curve currentCurve{base(random), mult(random)}, candidateCurve{base(random), mult(random)};
        const auto lv = level(random);
        const auto currentThreshold = Requirement(currentCurve, lv);
        const auto xp = currentThreshold * fraction(random);
        const auto plan = PlanThreshold(candidateCurve, lv, xp, currentThreshold);
        Check(std::isfinite(plan.threshold) && plan.threshold > 0, "All valid curves yield finite positive thresholds");
        if (xp >= currentThreshold) Check(plan.threshold == currentThreshold, "Preserve pending thresholds");
        if (plan.action == Refresh::update) Check(xp < plan.threshold && xp < currentThreshold, "Preserve pending-level eligibility");
        const auto roundtrip = ParseSettings(FormatSettings(candidateCurve));
        Check(roundtrip.ok && roundtrip.curve == candidateCurve, "Float settings round-trip exactly");
    }

    Check(ParseSettings("\xEF\xBB\xBF[Curve]\r\nBaseXP=375 ; custom\r\nXPPerLevel=15\r\n").ok, "BOM, CRLF, comments supported");
    for (const auto* text : {"", "[Curve]\nBaseXP=375", "[Curve]\nBaseXP=NaN\nXPPerLevel=15",
        "[Curve]\nBaseXP=375oops\nXPPerLevel=15", "[Curve]\nBaseXP=375\nBaseXP=400\nXPPerLevel=15",
        "[Curve]\nBaseXP=375\nXPPerLevell=15", "[Curve]\nBaseXP=375\nXPPerLevel=-1"}) {
        Check(!ParseSettings(text).ok, "Malformed config is rejected as a whole");
    }
    Session session;
    const auto initialEpoch = session.Epoch();
    session.Reset(Phase::loading);
    Check(!session.Accepts(initialEpoch) && !session.Accepts(session.Epoch()), "Loading invalidates and blocks tasks");
    const auto loaded = session.Reset(Phase::playing);
    Check(session.Accepts(loaded) && !session.Accepts(initialEpoch), "Only current save accepts work");
    session.Reset(Phase::menu);
    const auto newGame = session.Reset(Phase::playing);
    Check(session.Accepts(newGame) && !session.Accepts(loaded), "Session change invalidates character tasks");
    QueuedWork work;
    Check(work.Begin(loaded) && !work.Begin(loaded), "Coalesce duplicate tasks");
    work.Cancel();
    Check(work.Begin(newGame), "Revert releases queued work even if the engine discards its callback");
    work.Complete(loaded);
    Check(work.Busy(), "Stale callback preserves current session work");
    work.Complete(newGame);
    Check(!work.Busy(), "Current callback completes normally");

    const auto directory = std::filesystem::temp_directory_path() / ("xp_control-tests-" + std::to_string(random()));
    std::filesystem::create_directories(directory);
    const auto file = directory / "settings.ini";
    std::string error;
    Check(WriteSettings(file, Curve{}, error), "Write settings");
    Check(ReadSettings(file).curve == Curve{}, "Read written settings");
    Check(WriteSettings(file, kVanilla, error), "Replace existing settings");
    Check(ReadSettings(file).curve == kVanilla, "Saved configuration contains both values");
    Check(!WriteSettings(file, {0, 0}, error), "Invalid write rejected");
    Check(ReadSettings(file).curve == kVanilla, "Rejected write preserves saved settings");
    Check(!WriteSettings(directory / "missing" / "settings.ini", Curve{}, error), "Write failure is reported");
    std::filesystem::remove_all(directory);
}
}

int main() {
    try { Run(); std::cout << checks << " checks passed\n"; return EXIT_SUCCESS; }
    catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return EXIT_FAILURE; }
}
