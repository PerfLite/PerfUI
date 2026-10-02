#pragma once

#include "Types.h"
#include "UIContext.h"
#include "Toast.h"
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace PerfUI::API {

constexpr unsigned long InterfaceVersion_1 = 1;

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
};

} // namespace PerfUI::API

namespace PerfUI::Client {

// Function for third-party SKSE modders to obtain the PerfUI API pointer
inline API::IPerfUI_v1* GetApi() {
    static API::IPerfUI_v1* s_api = nullptr;
    if (!s_api) {
        auto hModule = ::GetModuleHandleW(L"PerfUI.dll");
        if (hModule) {
            typedef void* (*RequestPluginAPIFn)(unsigned long);
            auto fn = reinterpret_cast<RequestPluginAPIFn>(::GetProcAddress(hModule, "RequestPluginAPI"));
            if (fn) {
                s_api = static_cast<API::IPerfUI_v1*>(fn(API::InterfaceVersion_1));
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

} // namespace PerfUI::Client
