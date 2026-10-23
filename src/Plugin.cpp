#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "Runtime.h"

namespace {
void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (!message) return;
    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        xp_control::InitializeRuntime();
        if (!xp_control::RegisterMenu()) SKSE::log::warn("Menu Framework unavailable; INI settings remain active.");
        break;
    case SKSE::MessagingInterface::kPreLoadGame:
        xp_control::ResetSession(xp_control::Phase::loading);
        break;
    case SKSE::MessagingInterface::kPostLoadGame:
        if (message->data) xp_control::StartSession();
        else xp_control::ResetSession(xp_control::Phase::menu);
        break;
    case SKSE::MessagingInterface::kNewGame:
        xp_control::StartSession();
        break;
    default: break;
    }
}
}

extern "C" __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept {
    SKSE::PluginVersionData data{};
    data.PluginName("XPControl");
    data.AuthorName("XP Control contributors");
    data.PluginVersion(REL::Version(XPCONTROL_VERSION_MAJOR, XPCONTROL_VERSION_MINOR, XPCONTROL_VERSION_PATCH, 0));
    data.UsesAddressLibrary();
    // Layout differences are handled by CommonLib runtime accessors.
    data.UsesNoStructs();
    return data;
}();

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* skse) {
    if (!skse || skse->IsEditor()) return false;
    auto directory = SKSE::log::log_directory();
    if (!directory) return false;
    try {
        *directory /= "XPControl.log";
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(directory->string(), true);
        auto log = std::make_shared<spdlog::logger>("global", std::move(sink));
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        SKSE::InitInfo init;
        init.log = false;
        SKSE::Init(skse, init);
        SKSE::log::info("XP Control {}; Skyrim {}; CommonLibSSE-NG 13.1.0", XPCONTROL_VERSION, skse->RuntimeVersion().string());
        const auto* messaging = SKSE::GetMessagingInterface();
        const auto* serialization = SKSE::GetSerializationInterface();
        if (!messaging || !serialization || !SKSE::GetTaskInterface()) return false;
        // Cancel queued work when the current session ends.
        serialization->SetUniqueID(0x58504354); // XPCT
        serialization->SetRevertCallback([](auto*) { xp_control::ResetSession(xp_control::Phase::loading); });
        return messaging->RegisterListener(OnMessage);
    } catch (const std::exception&) {
        return false;
    }
}

// SE and VR use Query; AE uses SKSEPlugin_Version. Both entry points are
// required for a universal DLL. Address Library supplies the runtime IDs.
extern "C" __declspec(dllexport) bool SKSEPlugin_Query(
    const SKSE::QueryInterface* skse, SKSE::PluginInfo* info) {
    if (!skse || !info || skse->IsEditor()) return false;
    info->infoVersion = SKSE::PluginInfo::kVersion;
    info->name = SKSEPlugin_Version.pluginName;
    info->version = SKSEPlugin_Version.pluginVersion;
    return true;
}
