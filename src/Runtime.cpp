#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <Windows.h>

#include "Runtime.h"
#include "Settings.h"

#include <atomic>
#include <cstddef>
#include <mutex>

namespace xp_control {
namespace {
constexpr auto kSettings = "Data/SKSE/Plugins/XPControl.ini";
Session session;
std::atomic_bool initialized{false};
QueuedWork applyWork;
QueuedWork snapshotWork;
std::mutex snapshotMutex;
Snapshot snapshot;
// Runtime state is accessed by game tasks and SKSE lifecycle messages.
Curve configured;
std::string status = "Waiting for game data.";
std::string deferredReason;

struct GameSettings { RE::Setting* base; RE::Setting* mult; };
GameSettings Settings() {
    auto* collection = RE::GameSettingCollection::GetSingleton();
    if (!collection) return {};
    auto* base = collection->GetSetting("fXPLevelUpBase");
    auto* mult = collection->GetSetting("fXPLevelUpMult");
    if (!base || !mult || base->GetType() != RE::Setting::Type::kFloat ||
        mult->GetType() != RE::Setting::Type::kFloat) return {};
    return {base, mult};
}

bool StableGameplay() {
    if (session.Current() != Phase::playing) return false;
    auto* ui = RE::UI::GetSingleton();
    return ui && !ui->IsMenuOpen(RE::MainMenu::MENU_NAME) &&
        !ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) &&
        !ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME) &&
        !ui->IsMenuOpen(RE::StatsMenu::MENU_NAME);
}

RE::PlayerCharacter::PlayerSkills::Data* PlayerData(unsigned& level) {
    if (!StableGameplay()) return nullptr;
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->GetParentCell()) return nullptr;
    // CommonLib selects each runtime's player-info layout.
    const RE::PlayerCharacter::PlayerSkills* skills = nullptr;
    if (REL::Module::IsVR()) {
        const auto* info = player->GetVRInfoRuntimeData();
        if (info) skills = info->skills;
    } else {
        skills = player->GetInfoRuntimeData().skills;
    }
    if (!skills || !skills->data) return nullptr;
    level = player->GetLevel();
    return skills->data;
}

bool CanApply() {
    if (!initialized.load() || session.Current() == Phase::loading) return false;
    return session.Current() == Phase::menu || StableGameplay();
}

void Publish() {
    Snapshot next;
    next.initialized = initialized.load();
    next.configured = configured;
    next.busy = applyWork.Busy();
    next.canApply = CanApply();
    next.experience = GetModuleHandleW(L"Experience.dll") != nullptr;
    next.status = status;
    if (!deferredReason.empty()) next.status += " " + deferredReason;
    if (next.initialized) {
        const auto gmst = Settings();
        if (gmst.base) {
            next.engine = {gmst.base->data.f, gmst.mult->data.f};
            next.engineValid = Valid(next.engine);
        }
        if (const auto* data = PlayerData(next.level)) {
            next.hasPlayer = true;
            next.xp = data->xp;
            next.threshold = data->levelThreshold;
        }
    }
    std::scoped_lock lock(snapshotMutex);
    snapshot = std::move(next);
}

bool ApplyNow(const char* reason) {
    if (!CanApply()) return false;
    const auto gmst = Settings();
    if (!gmst.base) {
        status = "XP game settings are unavailable.";
        SKSE::log::error("{}", status);
        return false;
    }
    unsigned level = 0;
    auto* data = PlayerData(level);
    const float xpBefore = data ? data->xp : 0.0F;
    const float thresholdBefore = data ? data->levelThreshold : 0.0F;
    const auto plan = data ? PlanThreshold(configured, level, xpBefore, thresholdBefore) : ThresholdPlan{};

    // Apply the configured curve and refresh an eligible cached threshold.
    gmst.base->data.f = configured.base;
    gmst.mult->data.f = configured.mult;
    deferredReason.clear();
    if (data && plan.action == Refresh::update) data->levelThreshold = plan.threshold;
    if (data && (plan.action == Refresh::pendingLevel || plan.action == Refresh::wouldCreateLevel)) {
        deferredReason = "The current XP threshold remains active until the next level-up.";
    } else if (data && plan.action == Refresh::invalid) {
        deferredReason = "Threshold refresh is waiting for valid player XP data.";
    }
    SKSE::log::info("{}: curve={}/{} level={} XP={} threshold={} -> {} refresh={}",
        reason, configured.base, configured.mult, level, xpBefore, thresholdBefore,
        data ? data->levelThreshold : 0.0F, static_cast<int>(plan.action));
    return true;
}

void QueueSessionApply() {
    const auto epoch = session.Epoch();
    SKSE::GetTaskInterface()->AddTask([epoch] {
        if (!session.Accepts(epoch) || !initialized.load()) return;
        if (ApplyNow("Session ready")) status = "Saved curve applied.";
        Publish();
    });
}

class MenuEvents final : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
        if (!event) return RE::BSEventNotifyControl::kContinue;
        if (event->menuName == RE::MainMenu::MENU_NAME && event->opening) {
            ResetSession(Phase::menu);
        } else if (!event->opening && session.Current() == Phase::playing &&
            (event->menuName == RE::RaceSexMenu::MENU_NAME ||
             event->menuName == RE::LoadingMenu::MENU_NAME ||
             event->menuName == RE::StatsMenu::MENU_NAME)) {
            QueueSessionApply();
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
MenuEvents menuEvents;
}

static_assert(offsetof(RE::PlayerCharacter::PlayerSkills::Data, xp) == 0);
static_assert(offsetof(RE::PlayerCharacter::PlayerSkills::Data, levelThreshold) == 4);

void InitializeRuntime() {
    const auto settings = ReadSettings(kSettings);
    configured = settings.curve;
    if (!settings.ok) SKSE::log::warn("{}", settings.message);
    initialized = true;
    status = settings.ok ? "Saved curve loaded." : settings.message;
    ApplyNow("Game data loaded");
    if (auto* ui = RE::UI::GetSingleton()) ui->AddEventSink(&menuEvents);
    Publish();
}

void ResetSession(Phase phase) {
    session.Reset(phase);
    applyWork.Cancel();
    snapshotWork.Cancel();
    // Revert invalidates the session and clears the displayed player data.
    std::scoped_lock lock(snapshotMutex);
    snapshot.hasPlayer = false;
    snapshot.canApply = false;
    snapshot.level = 0;
    snapshot.xp = snapshot.threshold = 0.0F;
}

void StartSession() {
    ResetSession(Phase::playing);
    QueueSessionApply();
}

Snapshot GetSnapshot() {
    std::scoped_lock lock(snapshotMutex);
    auto copy = snapshot;
    copy.busy = applyWork.Busy();
    return copy;
}

void RequestSnapshot() {
    if (!initialized.load()) return;
    const auto epoch = session.Epoch();
    if (!snapshotWork.Begin(epoch)) return;
    SKSE::GetTaskInterface()->AddTask([epoch] {
        snapshotWork.Complete(epoch);
        if (session.Accepts(epoch)) Publish();
    });
}

void RequestApply(Curve curve) {
    if (!Valid(curve)) return;
    const auto epoch = session.Epoch();
    if (!applyWork.Begin(epoch)) return;
    SKSE::GetTaskInterface()->AddTask([epoch, curve] {
        if (!session.Accepts(epoch) || !CanApply()) {
            applyWork.Complete(epoch);
            // Discard tasks whose session has ended.
            return;
        }
        const auto gmst = Settings();
        std::string error;
        if (!gmst.base) {
            status = "XP settings are unavailable.";
        } else if (!WriteSettings(kSettings, curve, error)) {
            status = error;
            SKSE::log::error("{}", status);
        } else {
            configured = curve;
            if (ApplyNow("Apply and save")) status = "Curve applied and settings saved.";
        }
        applyWork.Complete(epoch);
        Publish();
    });
}
}
