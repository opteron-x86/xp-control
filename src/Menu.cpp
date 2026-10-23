#include "Runtime.h"

#include <atomic>
#include <cfloat>
#include <cstdarg>
#include <filesystem>
#include <chrono>
#include <array>
#include <iterator>
#include <string>
namespace RE { class InputEvent; }
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#pragma clang diagnostic ignored "-Wreturn-type"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4099 4715)
#endif
#include "SKSEMenuFramework.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace xp_control {
namespace {
namespace UI = ImGuiMCP;
Curve draft;
bool draftInitialized = false;
int selectedPreset = 0;
auto lastRefresh = std::chrono::steady_clock::time_point{};

void __stdcall Render() {
    const auto now = std::chrono::steady_clock::now();
    if (now - lastRefresh >= std::chrono::milliseconds(500)) {
        RequestSnapshot();
        lastRefresh = now;
    }
    const auto state = GetSnapshot();
    if (state.initialized && !draftInitialized) {
        draft = state.configured;
        draftInitialized = true;
    }
    UI::TextUnformatted("XP Control");
    UI::TextWrapped("Set how much character XP each level costs. Experience and other XP sources continue to award XP normally.");
    UI::Separator();
    if (state.hasPlayer) {
        UI::Text("Level %u  |  XP %.1f / %.1f", state.level, state.xp, state.threshold);
    } else {
        UI::TextUnformatted("Character XP is available after loading and finishing character creation.");
    }
    UI::Text("Saved curve: %.1f base + %.1f per level", state.configured.base, state.configured.mult);
    if (state.engineValid && state.engine != state.configured) {
        UI::TextWrapped("Active game settings: %.1f base / %.1f per level.", state.engine.base, state.engine.mult);
    }
    if (state.experience) UI::TextUnformatted("Experience detected.");
    UI::TextWrapped("%s", state.status.c_str());
    UI::Separator();

    UI::BeginDisabled(!state.initialized || state.busy);
    const char* presets[]{"Standard", "Vanilla", "Moderate", "Balanced", "Gentle", "Flat", "Slower flat"};
    if (UI::Combo("Preset", &selectedPreset, presets, static_cast<int>(std::size(presets)))) {
        draft = kPresets[selectedPreset].curve;
    }
    UI::InputFloat("Base XP", &draft.base, 25.0F, 100.0F, "%.1f");
    UI::InputFloat("Extra XP per level", &draft.mult, 1.0F, 5.0F, "%.1f");
    UI::TextWrapped("XP required = base XP + (current level x extra XP per level)");
    const bool valid = Valid(draft);
    if (!valid) UI::TextUnformatted("Enter a base from 1 to 100,000 and extra XP from 0 to 10,000.");
    UI::BeginDisabled(!valid || !state.canApply);
    if (UI::Button(state.busy ? "Applying..." : "Apply and save")) RequestApply(draft);
    UI::EndDisabled();
    UI::SameLine();
    if (UI::Button("Revert edits")) draft = state.configured;
    UI::EndDisabled();
    UI::TextWrapped("Settings apply to every character using this configuration. Pending level-ups retain the current threshold until a level-up completes.");

    if (valid) {
        UI::Separator();
        UI::TextUnformatted("Curve preview");
        std::array<float, 80> values{};
        for (unsigned i = 0; i < values.size(); ++i) values[i] = Requirement(draft, i + 1);
        UI::PlotLines("##curve", values.data(), static_cast<int>(values.size()), 0,
            "XP per level, levels 1-80", 0.0F, values.back() * 1.1F, {0.0F, 140.0F});
        if (UI::BeginTable("XP preview", 4, UI::ImGuiTableFlags_RowBg | UI::ImGuiTableFlags_BordersInnerV)) {
            UI::TableSetupColumn("Current level");
            UI::TableSetupColumn("Next level XP");
            UI::TableSetupColumn("Vanilla XP");
            UI::TableSetupColumn("Total XP to reach level");
            UI::TableHeadersRow();
            for (const unsigned level : {1U, 5U, 10U, 25U, 50U, 75U, 100U}) {
                UI::TableNextRow();
                UI::TableSetColumnIndex(0); UI::Text("%u", level);
                UI::TableSetColumnIndex(1); UI::Text("%.1f", Requirement(draft, level));
                UI::TableSetColumnIndex(2); UI::Text("%.1f", Requirement(kVanilla, level));
                UI::TableSetColumnIndex(3); UI::Text("%.1f", TotalToLevel(draft, level));
            }
            UI::EndTable();
        }
    }
}
}

bool RegisterMenu() {
    const auto module = GetModuleHandleW(L"SKSEMenuFramework.dll");
    if (!module) return false;
    // Validate the framework exports used by this page.
    for (const auto* name : {"AddSectionItem", "igTextV", "igTextUnformatted", "igTextWrappedV", "igSeparator",
        "igBeginDisabled", "igEndDisabled", "igCombo_Str_arr", "igInputFloat", "igButton", "igSameLine",
        "igPlotLines_FloatPtr", "igBeginTable", "igTableSetupColumn", "igTableHeadersRow", "igTableNextRow",
        "igTableSetColumnIndex", "igEndTable"}) {
        if (!GetProcAddress(module, name)) return false;
    }
    SKSEMenuFramework::SetSection("XP Control");
    SKSEMenuFramework::AddSectionItem("XP curve", Render);
    return true;
}
}
