#pragma once

#include "Types.h"
#include "UIContext.h"
#include "Toast.h"
#include "Overlay.h"
#include <string>
#include <cstdint>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace PerfUI::API {

constexpr unsigned long InterfaceVersion_1 = 1;

struct ClientABIInfo {
    unsigned long structSize{ sizeof(ClientABIInfo) };
    unsigned long mscVer{
#if defined(_MSC_VER)
        _MSC_VER
#else
        0
#endif
    };
    int iteratorDebugLevel{
#if defined(_ITERATOR_DEBUG_LEVEL)
        _ITERATOR_DEBUG_LEVEL
#else
        0
#endif
    };
};

struct IPerfUI_v1 {
    unsigned long version{ InterfaceVersion_1 };

    // Core Context Access (allows adding custom windows/elements to root)
    UIContext* (*GetContext)();

    // Helper functions
    void (*ShowToast)(const char* title, const char* message, int toastType, float duration);
    void (*PlaySound)(const char* soundEditorId);
    void (*SetUIVisible)(bool visible);
    bool (*IsUIVisible)();
    void (*ToggleUI)();

    // Overlay System (Phase 1 Clean Client)
    OverlayId (*RegisterOverlay)(const char* name, OverlayCallback cb, int zOrder, bool alwaysVisible);
    void (*UnregisterOverlay)(OverlayId id);
    void (*SetOverlayVisible)(OverlayId id, bool visible);

    // Texture System (Phase 2 Clean Client)
    TextureId (*LoadTexture)(const char* filePath);
    TextureId (*CreateDynamicTexture)(uint32_t width, uint32_t height, const uint8_t* rgbaPixels);
    bool (*UpdateDynamicTexture)(TextureId id, uint32_t width, uint32_t height, const uint8_t* rgbaPixels);
    void (*DestroyTexture)(TextureId id);
    Dimensions (*GetTextureSize)(TextureId id);

    // Input & Hotkeys System (Phase 3 Clean Client)
    void (*CaptureInput)(bool capture);
    bool (*RegisterHotkey)(const char* id, uint32_t defaultKey, std::function<void()> cb);
    void (*UnregisterHotkey)(const char* id);
};

} // namespace PerfUI::API

namespace PerfUI::Client {

// Function for third-party SKSE modders to obtain the PerfUI API pointer
inline API::IPerfUI_v1* GetApi() {
    static API::IPerfUI_v1* s_api = nullptr;
    if (!s_api) {
        auto hModule = ::GetModuleHandleW(L"PerfUI.dll");
        if (hModule) {
            typedef void* (*RequestPluginAPIExFn)(unsigned long, const API::ClientABIInfo*);
            auto fnEx = reinterpret_cast<RequestPluginAPIExFn>(::GetProcAddress(hModule, "RequestPluginAPIEx"));
            if (fnEx) {
                API::ClientABIInfo abi;
                s_api = static_cast<API::IPerfUI_v1*>(fnEx(API::InterfaceVersion_1, &abi));
            } else {
                typedef void* (*RequestPluginAPIFn)(unsigned long);
                auto fn = reinterpret_cast<RequestPluginAPIFn>(::GetProcAddress(hModule, "RequestPluginAPI"));
                if (fn) {
                    s_api = static_cast<API::IPerfUI_v1*>(fn(API::InterfaceVersion_1));
                }
            }
        }
    }
    return s_api;
}

inline UIContext* GetContext() {
    auto* api = GetApi();
    return api ? api->GetContext() : nullptr;
}

inline void ShowToast(const std::string& title, const std::string& message, ToastType type = ToastType::Info, float duration = 3.2f) {
    auto* api = GetApi();
    if (api && api->ShowToast) {
        api->ShowToast(title.c_str(), message.c_str(), static_cast<int>(type), duration);
    }
}

inline void PlaySound(const std::string& soundEditorId) {
    auto* api = GetApi();
    if (api && api->PlaySound) {
        api->PlaySound(soundEditorId.c_str());
    }
}

inline void SetUIVisible(bool visible) {
    auto* api = GetApi();
    if (api && api->SetUIVisible) {
        api->SetUIVisible(visible);
    }
}

inline bool IsUIVisible() {
    auto* api = GetApi();
    return api && api->IsUIVisible ? api->IsUIVisible() : false;
}

inline void ToggleUI() {
    auto* api = GetApi();
    if (api && api->ToggleUI) {
        api->ToggleUI();
    }
}

inline void RunOnUIThread(std::function<void()> task) {
    auto* ctx = GetContext();
    if (ctx) {
        ctx->runOnUIThread(std::move(task));
    }
}

inline OverlayId RegisterOverlay(const char* name, OverlayCallback cb, int zOrder = 0, bool alwaysVisible = true) {
    auto* api = GetApi();
    if (api && api->RegisterOverlay) {
        return api->RegisterOverlay(name, std::move(cb), zOrder, alwaysVisible);
    }
    auto* ctx = GetContext();
    return ctx ? ctx->registerOverlay(name, std::move(cb), zOrder, alwaysVisible) : 0;
}

inline void UnregisterOverlay(OverlayId id) {
    auto* api = GetApi();
    if (api && api->UnregisterOverlay) {
        api->UnregisterOverlay(id);
        return;
    }
    auto* ctx = GetContext();
    if (ctx) {
        ctx->unregisterOverlay(id);
    }
}

inline void SetOverlayVisible(OverlayId id, bool visible) {
    auto* api = GetApi();
    if (api && api->SetOverlayVisible) {
        api->SetOverlayVisible(id, visible);
        return;
    }
    auto* ctx = GetContext();
    if (ctx) {
        ctx->setOverlayVisible(id, visible);
    }
}

// Texture System (Phase 2 Clean Client)
inline TextureId LoadTexture(const char* filePath) {
    auto* api = GetApi();
    if (api && api->LoadTexture) {
        return api->LoadTexture(filePath);
    }
    auto* ctx = GetContext();
    return ctx ? ctx->loadTexture(filePath) : 0;
}

inline TextureId CreateDynamicTexture(uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
    auto* api = GetApi();
    if (api && api->CreateDynamicTexture) {
        return api->CreateDynamicTexture(width, height, rgbaPixels);
    }
    auto* ctx = GetContext();
    return ctx ? ctx->createDynamicTexture(width, height, rgbaPixels) : 0;
}

inline bool UpdateDynamicTexture(TextureId id, uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
    auto* api = GetApi();
    if (api && api->UpdateDynamicTexture) {
        return api->UpdateDynamicTexture(id, width, height, rgbaPixels);
    }
    auto* ctx = GetContext();
    return ctx ? ctx->updateDynamicTexture(id, width, height, rgbaPixels) : false;
}

inline void DestroyTexture(TextureId id) {
    auto* api = GetApi();
    if (api && api->DestroyTexture) {
        api->DestroyTexture(id);
        return;
    }
    auto* ctx = GetContext();
    if (ctx) {
        ctx->destroyTexture(id);
    }
}

inline Dimensions GetTextureSize(TextureId id) {
    auto* api = GetApi();
    if (api && api->GetTextureSize) {
        return api->GetTextureSize(id);
    }
    auto* ctx = GetContext();
    return ctx ? ctx->getTextureSize(id) : Dimensions{ 0.0f, 0.0f };
}

// Input & Hotkeys System (Phase 3 Clean Client)
inline void CaptureInput(bool capture) {
    auto* api = GetApi();
    if (api && api->CaptureInput) {
        api->CaptureInput(capture);
        return;
    }
}

inline bool RegisterHotkey(const char* id, uint32_t defaultKey, std::function<void()> cb) {
    auto* api = GetApi();
    if (api && api->RegisterHotkey) {
        return api->RegisterHotkey(id, defaultKey, std::move(cb));
    }
    auto* ctx = GetContext();
    if (ctx && id && cb) {
        return ctx->registerHotkey(id, defaultKey, std::move(cb));
    }
    return false;
}

inline void UnregisterHotkey(const char* id) {
    auto* api = GetApi();
    if (api && api->UnregisterHotkey) {
        api->UnregisterHotkey(id);
        return;
    }
    auto* ctx = GetContext();
    if (ctx && id) {
        ctx->unregisterHotkey(id);
    }
}

} // namespace PerfUI::Client
