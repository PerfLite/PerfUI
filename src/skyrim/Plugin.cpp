#include "Pch.h"
#include "D3D11Hook.h"
#include "InputHook.h"
#include "SkyrimSoundService.h"
#include "PerfUI/PerfUIApi.h"
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
    .Author = "PerfLite"
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

// -------------------------------------------------------------
// PerfUI Official Plugin API Export (for third-party modders)
// -------------------------------------------------------------
static PerfUI::UIContext* API_GetContext() {
    return PerfUI::Skyrim::D3D11Hook::GetSingleton().GetContext();
}

static void API_ShowToast(const char* title, const char* message, int toastType, float duration) {
    auto* ctx = API_GetContext();
    if (ctx && title && message) {
        ctx->showToast(title, message, static_cast<PerfUI::ToastType>(toastType), duration);
    }
}

static void API_PlaySound(const char* soundEditorId) {
    if (soundEditorId) {
        PerfUI::Skyrim::SkyrimSoundService::PlayUISound(soundEditorId);
    }
}

static void API_SetUIVisible(bool visible) {
    PerfUI::Skyrim::D3D11Hook::GetSingleton().SetUIVisible(visible);
}

static bool API_IsUIVisible() {
    return PerfUI::Skyrim::D3D11Hook::GetSingleton().IsUIVisible();
}

static void API_ToggleUI() {
    PerfUI::Skyrim::D3D11Hook::GetSingleton().ToggleUI();
}

static PerfUI::OverlayId API_RegisterOverlay(const char* name, PerfUI::OverlayCallback cb, int zOrder, bool alwaysVisible) {
    auto* ctx = API_GetContext();
    return ctx ? ctx->registerOverlay(name, std::move(cb), zOrder, alwaysVisible) : 0;
}

static void API_UnregisterOverlay(PerfUI::OverlayId id) {
    auto* ctx = API_GetContext();
    if (ctx) {
        ctx->unregisterOverlay(id);
    }
}

static void API_SetOverlayVisible(PerfUI::OverlayId id, bool visible) {
    auto* ctx = API_GetContext();
    if (ctx) {
        ctx->setOverlayVisible(id, visible);
    }
}

static PerfUI::TextureId API_LoadTexture(const char* filePath) {
    if (!filePath) return 0;
    auto* ctx = API_GetContext();
    return ctx ? ctx->loadTexture(filePath) : 0;
}

static PerfUI::TextureId API_CreateDynamicTexture(uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
    auto* ctx = API_GetContext();
    return ctx ? ctx->createDynamicTexture(width, height, rgbaPixels) : 0;
}

static bool API_UpdateDynamicTexture(PerfUI::TextureId id, uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
    auto* ctx = API_GetContext();
    return ctx ? ctx->updateDynamicTexture(id, width, height, rgbaPixels) : false;
}

static void API_DestroyTexture(PerfUI::TextureId id) {
    auto* ctx = API_GetContext();
    if (ctx) {
        ctx->destroyTexture(id);
    }
}

static PerfUI::Dimensions API_GetTextureSize(PerfUI::TextureId id) {
    auto* ctx = API_GetContext();
    return ctx ? ctx->getTextureSize(id) : PerfUI::Dimensions{ 0.0f, 0.0f };
}

static PerfUI::API::IPerfUI_v1 g_perfUI_API_v1{
    .version = PerfUI::API::InterfaceVersion_1,
    .GetContext = API_GetContext,
    .ShowToast = API_ShowToast,
    .PlaySound = API_PlaySound,
    .SetUIVisible = API_SetUIVisible,
    .IsUIVisible = API_IsUIVisible,
    .ToggleUI = API_ToggleUI,
    .RegisterOverlay = API_RegisterOverlay,
    .UnregisterOverlay = API_UnregisterOverlay,
    .SetOverlayVisible = API_SetOverlayVisible,
    .LoadTexture = API_LoadTexture,
    .CreateDynamicTexture = API_CreateDynamicTexture,
    .UpdateDynamicTexture = API_UpdateDynamicTexture,
    .DestroyTexture = API_DestroyTexture,
    .GetTextureSize = API_GetTextureSize
};

extern "C" __declspec(dllexport) void* RequestPluginAPIEx(unsigned long a_interfaceVersion, const PerfUI::API::ClientABIInfo* a_clientAbi) {
    if (a_interfaceVersion != PerfUI::API::InterfaceVersion_1) {
        SKSE::log::warn("RequestPluginAPIEx called with unsupported version: {}", a_interfaceVersion);
        return nullptr;
    }

    if (a_clientAbi) {
        constexpr unsigned long hostMscVer =
#if defined(_MSC_VER)
            _MSC_VER;
#else
            0;
#endif
        constexpr int hostIdl =
#if defined(_ITERATOR_DEBUG_LEVEL)
            _ITERATOR_DEBUG_LEVEL;
#else
            0;
#endif

        if (a_clientAbi->iteratorDebugLevel != hostIdl) {
            SKSE::log::error("PerfUI API Rejected: _ITERATOR_DEBUG_LEVEL mismatch! Host={}, Client={}. Mod must be built in Release mode with /MD.",
                hostIdl, a_clientAbi->iteratorDebugLevel);
            return nullptr;
        }

        if (a_clientAbi->mscVer > 0 && (a_clientAbi->mscVer / 100 != hostMscVer / 100 || a_clientAbi->mscVer < 1930)) {
            SKSE::log::error("PerfUI API Rejected: MSVC toolchain version mismatch! Host requires MSVC v143 (_MSC_VER >= 1930), client provided _MSC_VER={}.",
                a_clientAbi->mscVer);
            return nullptr;
        }
    }

    SKSE::log::info("RequestPluginAPIEx(v1) called by external mod - ABI verified, granting API pointer!");
    return &g_perfUI_API_v1;
}

extern "C" __declspec(dllexport) void* RequestPluginAPI(unsigned long a_interfaceVersion) {
    return RequestPluginAPIEx(a_interfaceVersion, nullptr);
}
