#include "Pch.h"
#include "D3D11Hook.h"
#include "InputHook.h"
#include <spdlog/sinks/basic_file_sink.h>

namespace {

void InitializeLogging() {
    auto path = SKSE::log::log_directory();
    if (!path) {
        return;
    }

    *path /= "PerfUI.log";
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

    log->set_level(spdlog::level::info);
    log->flush_on(spdlog::level::info);

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
}

void OnMessage(SKSE::MessagingInterface::Message* a_msg) {
    if (!a_msg) return;

    switch (a_msg->type) {
    case SKSE::MessagingInterface::kDataLoaded: {
        SKSE::log::info("Skyrim kDataLoaded received, installing D3D11 and Input hooks...");

        if (PerfUI::Skyrim::D3D11Hook::GetSingleton().Install()) {
            auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
            if (renderer && renderer->data.renderWindows[0].hWnd) {
                HWND hWnd = reinterpret_cast<HWND>(renderer->data.renderWindows[0].hWnd);
                PerfUI::Skyrim::InputHook::GetSingleton().Install(hWnd);
            }
            SKSE::log::info("PerfUI initialized successfully! Press F11 in game to toggle menu.");

            auto* dataHandler = RE::TESDataHandler::GetSingleton();
            if (dataHandler) {
                const auto* lal = dataHandler->LookupLoadedModByName("Alternate Start - Live Another Life.esp");
                if (lal) {
                    SKSE::log::info(">>> Alternate Start - Live Another Life.esp IS LOADED! Index: {}", lal->compileIndex);
                } else {
                    SKSE::log::warn(">>> Alternate Start - Live Another Life.esp is NOT LOADED by Skyrim!");
                }
            }
        } else {
            SKSE::log::error("Failed to install D3D11 hook");
        }
        break;
    }
    default:
        if (a_msg->type > 8) { // Game shutdown
            SKSE::log::info("Skyrim shutdown signal received, cleaning up hooks...");
            PerfUI::Skyrim::InputHook::GetSingleton().Uninstall();
            PerfUI::Skyrim::D3D11Hook::GetSingleton().Uninstall();
        }
        break;
    }
}

} // namespace

SKSEPluginInfo(
    .Version = REL::Version{ 0, 1, 0, 0 },
    .Name = "PerfUI",
    .Author = "Alik"
)

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    InitializeLogging();
    SKSE::log::info("PerfUI v0.1.0 loading...");

    auto* messaging = static_cast<SKSE::MessagingInterface*>(
        a_skse->QueryInterface(SKSE::LoadInterface::kMessaging));

    if (!messaging) {
        SKSE::log::critical("Failed to query SKSE messaging interface");
        return false;
    }

    SKSE::Init(a_skse);
    messaging->RegisterListener("SKSE", OnMessage);

    SKSE::log::info("PerfUI loaded and messaging listener registered");
    return true;
}
