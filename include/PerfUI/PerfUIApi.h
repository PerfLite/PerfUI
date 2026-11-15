#pragma once

#include "Types.h"
#include "UIContext.h"
#include "Toast.h"
#include "Overlay.h"
#include <string>
#include <cstdint>
#include <atomic>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace PerfUI {
class ScriptHost;
}

namespace PerfUI::API {

constexpr unsigned long InterfaceVersion_1 = 1;
constexpr unsigned long InterfaceVersion_2 = 2;
constexpr unsigned long InterfaceVersion_3 = 3;
constexpr unsigned long InterfaceVersion_4 = 4;
constexpr unsigned long InterfaceVersion_5 = 5;

constexpr unsigned long LuaApiVersion_1 = 1;
constexpr unsigned long PERFUI_LUA_API_VERSION = LuaApiVersion_1;

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
    TextureId (*LoadSvg)(const char* filePath, float targetWidth, float targetHeight);
    TextureId (*LoadSvgFromMemory)(const char* svgContent, float targetWidth, float targetHeight);

    // Input & Hotkeys System (Phase 3 Clean Client)
    void (*CaptureInput)(bool capture);
    bool (*RegisterHotkey)(const char* id, uint32_t defaultKey, std::function<void()> cb);
    void (*UnregisterHotkey)(const char* id);

    // Must be set before SetUIVisible. Modal (default) swallows the whole input; overlay keeps
    // the keyboard and the game controls alive so the world stays playable under the panel.
    void (*SetOverlayMode)(bool overlay);
};

// v2 appends to v1 without changing the v1 layout, so mods built against v1 keep working.
struct IPerfUI_v2 {
    IPerfUI_v1 v1;

    // Blocks gameplay keyboard (pause menu, movement) while keeping PerfUI hotkeys alive.
    // Pair this with CaptureInput(true) for modal-style overlay interactions such as editors.
    void (*SetCaptureKeyboard)(bool capture);
};

// v3: adds ScriptHost lifecycle for modders to execute sandboxed Lua 5.4 scripts,
// external texture registration, and WorldAnchor target tracking.
struct IPerfUI_v3 {
    IPerfUI_v2 v2;

    ScriptHost* (*CreateScriptHost)(const char* modName, unsigned long luaApiVersion);
    void        (*DestroyScriptHost)(ScriptHost* host);

    // External Texture Support (Render Targets / 3D item preview)
    TextureId   (*RegisterExternalTexture)(void* srv, uint32_t width, uint32_t height);
    void        (*UnregisterExternalTexture)(TextureId id);

    // WorldAnchor Tracking API
    uint32_t    (*TrackPosition)(float x, float y, float z, bool checkLoS);
    uint32_t    (*TrackActor)(uint32_t formId, float offsetX, float offsetY, float offsetZ, bool checkLoS);
    void        (*Untrack)(uint32_t targetId);
};

struct SettingsPageBuilder;

struct SettingsPageBuilder {
    void (*AddHeader)(SettingsPageBuilder* self, const char* title);
    void (*AddText)(SettingsPageBuilder* self, const char* text, const char* tooltip);
    void (*AddSeparator)(SettingsPageBuilder* self);
    void (*AddEmpty)(SettingsPageBuilder* self, float height);
    void (*AddToggle)(SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, bool defaultValue);
    void (*AddSlider)(SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, float minVal, float maxVal, float stepVal, float defaultValue, const char* formatString);
    void (*AddStepper)(SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, int32_t minVal, int32_t maxVal, int32_t stepVal, int32_t defaultValue);
    void (*AddChoice)(SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, const char** optionIds, const char** optionLabels, size_t optionCount, const char* defaultId);
    void (*AddColor)(SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, uint32_t defaultRgba);
    void (*AddKeybind)(SettingsPageBuilder* self, const char* label, const char* key, const char* tooltip, uint32_t defaultKey);
    void (*AddTextInput)(SettingsPageBuilder* self, const char* label, const char* key, const char* placeholder, const char* tooltip, const char* defaultText);
    void (*SetRowVisibleIf)(SettingsPageBuilder* self, const char* key, const char* conditionKey, bool invert);
    void (*SetRowEnabledIf)(SettingsPageBuilder* self, const char* key, const char* conditionKey, bool invert);
    void* internalData;
};

// v4: adds Mod Settings Layer (SettingsStore access, declarative/C++ mod settings registration, menu opening)
struct IPerfUI_v4 {
    IPerfUI_v3 v3;

    // Mod Settings Registration (ABI-safe: no std:: containers)
    bool (*RegisterModSettingsJson)(const char* modId, const char* displayName, const char* jsonPath);
    bool (*RegisterModSettingsPage)(const char* modId, const char* displayName, void (*build)(void* userData, SettingsPageBuilder* builder), void* userData);

    // Settings Store Access
    bool        (*GetSettingBool)(const char* modId, const char* key, bool defaultVal);
    void        (*SetSettingBool)(const char* modId, const char* key, bool val);
    int32_t     (*GetSettingInt)(const char* modId, const char* key, int32_t defaultVal);
    void        (*SetSettingInt)(const char* modId, const char* key, int32_t val);
    float       (*GetSettingFloat)(const char* modId, const char* key, float defaultVal);
    void        (*SetSettingFloat)(const char* modId, const char* key, float val);
    const char* (*GetSettingString)(const char* modId, const char* key, const char* defaultVal);
    void        (*SetSettingString)(const char* modId, const char* key, const char* val);
    uint32_t    (*GetSettingColor)(const char* modId, const char* key, uint32_t defaultRgba);
    void        (*SetSettingColor)(const char* modId, const char* key, uint32_t rgba);
    uint32_t    (*GetSettingKey)(const char* modId, const char* key, uint32_t defaultKey);
    void        (*SetSettingKey)(const char* modId, const char* key, uint32_t keycode);

    // Subscriptions
    uint64_t    (*SubscribeSettingChanged)(const char* modId, const char* key, void (*cb)(void* userData, const char* modId, const char* key), void* userData);
    void        (*UnsubscribeSettingChanged)(uint64_t subscriptionId);

    // Menu Navigation
    void        (*OpenSettings)(const char* modId);
    void        (*CloseSettings)();
};

/**
 * @brief PerfUI Interface Version 5.
 * Extends v4 with thread-safe caller-buffered string access.
 */
struct IPerfUI_v5 {
    IPerfUI_v4 v4;

    /**
     * @brief Thread-safe string copying into a caller-provided buffer.
     * @param modId Target mod identifier.
     * @param key Setting key.
     * @param outBuf Pointer to user buffer where the string will be written.
     * @param outBufSize Size of outBuf in bytes.
     * @param defaultVal Fallback string if key does not exist.
     * @return Number of characters copied (excluding null-terminator), or total required buffer size (including null-terminator) if outBuf is nullptr or outBufSize is 0.
     */
    size_t      (*GetSettingStringCopy)(const char* modId, const char* key, char* outBuf, size_t outBufSize, const char* defaultVal);
};

#if defined(_MSC_VER) && defined(_M_X64)
static_assert(sizeof(IPerfUI_v1) == 168, "IPerfUI_v1 ABI size mismatch");
static_assert(sizeof(IPerfUI_v2) == 176, "IPerfUI_v2 ABI size mismatch");
static_assert(sizeof(IPerfUI_v3) == 232, "IPerfUI_v3 ABI size mismatch");
static_assert(sizeof(IPerfUI_v4) == 376, "IPerfUI_v4 ABI size mismatch");
static_assert(sizeof(IPerfUI_v5) == 384, "IPerfUI_v5 ABI size mismatch");
static_assert(offsetof(IPerfUI_v2, v1) == 0, "IPerfUI_v2::v1 offset mismatch");
static_assert(offsetof(IPerfUI_v3, v2) == 0, "IPerfUI_v3::v2 offset mismatch");
static_assert(offsetof(IPerfUI_v4, v3) == 0, "IPerfUI_v4::v3 offset mismatch");
static_assert(offsetof(IPerfUI_v5, v4) == 0, "IPerfUI_v5::v4 offset mismatch");
#endif

struct ClientABIInfo {
    unsigned long structSize{ sizeof(ClientABIInfo) };
    unsigned long apiStructSize{ sizeof(IPerfUI_v1) };
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

} // namespace PerfUI::API

namespace PerfUI::Client {

namespace Detail {
using RequestApiFn = void* (*)(unsigned long version, const API::ClientABIInfo* abi);
inline std::atomic<RequestApiFn> s_customRequestFn{ nullptr };
inline std::atomic<bool> s_customHostLoaded{ false };

inline size_t ClampUtf8ToBoundary(const char* s, size_t maxBytes) {
    if (!s || maxBytes == 0) return 0;
    size_t i = maxBytes;
    while (i > 0 && (static_cast<unsigned char>(s[i - 1]) & 0xC0) == 0x80) {
        --i;
    }
    if (i == 0) return 0;
    unsigned char lead = static_cast<unsigned char>(s[i - 1]);
    if ((lead & 0x80) == 0) return maxBytes;
    size_t expectedLen = 0;
    if ((lead & 0xE0) == 0xC0) expectedLen = 2;
    else if ((lead & 0xF0) == 0xE0) expectedLen = 3;
    else if ((lead & 0xF8) == 0xF0) expectedLen = 4;
    else return i - 1;

    if (maxBytes - (i - 1) >= expectedLen) {
        return maxBytes;
    } else {
        return i - 1;
    }
}

inline void* QueryHostApi(unsigned long version, size_t structSize, bool* outHostLoaded = nullptr) {
    if (outHostLoaded) *outHostLoaded = false;
    auto customFn = s_customRequestFn.load(std::memory_order_relaxed);
    if (customFn) {
        if (outHostLoaded) *outHostLoaded = s_customHostLoaded.load(std::memory_order_relaxed);
        API::ClientABIInfo abi;
        abi.apiStructSize = static_cast<unsigned long>(structSize);
        return customFn(version, &abi);
    }
    auto hModule = ::GetModuleHandleW(L"PerfUI.dll");
    if (!hModule) return nullptr;
    if (outHostLoaded) *outHostLoaded = true;

    typedef void* (*RequestPluginAPIExFn)(unsigned long, const API::ClientABIInfo*);
    auto fnEx = reinterpret_cast<RequestPluginAPIExFn>(::GetProcAddress(hModule, "RequestPluginAPIEx"));
    if (fnEx) {
        API::ClientABIInfo abi;
        abi.apiStructSize = static_cast<unsigned long>(structSize);
        return fnEx(version, &abi);
    }
    typedef void* (*RequestPluginAPIFn)(unsigned long);
    auto fn = reinterpret_cast<RequestPluginAPIFn>(::GetProcAddress(hModule, "RequestPluginAPI"));
    if (fn) {
        return fn(version);
    }
    return nullptr;
}

inline std::atomic<API::IPerfUI_v1*> s_cachedV1{ nullptr };
inline std::atomic<API::IPerfUI_v2*> s_cachedV2{ nullptr };
inline std::atomic<API::IPerfUI_v3*> s_cachedV3{ nullptr };
inline std::atomic<API::IPerfUI_v4*> s_cachedV4{ nullptr };
inline std::atomic<API::IPerfUI_v5*> s_cachedV5{ nullptr };

inline std::atomic<bool> s_unsupportedV1{ false };
inline std::atomic<bool> s_unsupportedV2{ false };
inline std::atomic<bool> s_unsupportedV3{ false };
inline std::atomic<bool> s_unsupportedV4{ false };
inline std::atomic<bool> s_unsupportedV5{ false };

inline void ResetCachedApisForTesting() {
    s_cachedV1.store(nullptr, std::memory_order_relaxed);
    s_cachedV2.store(nullptr, std::memory_order_relaxed);
    s_cachedV3.store(nullptr, std::memory_order_relaxed);
    s_cachedV4.store(nullptr, std::memory_order_relaxed);
    s_cachedV5.store(nullptr, std::memory_order_relaxed);
    s_unsupportedV1.store(false, std::memory_order_relaxed);
    s_unsupportedV2.store(false, std::memory_order_relaxed);
    s_unsupportedV3.store(false, std::memory_order_relaxed);
    s_unsupportedV4.store(false, std::memory_order_relaxed);
    s_unsupportedV5.store(false, std::memory_order_relaxed);
    s_customHostLoaded.store(false, std::memory_order_relaxed);
}

} // namespace Detail

// Function for third-party SKSE modders to obtain the PerfUI API pointer.
// Retries on subsequent calls if PerfUI.dll was not yet loaded during the first attempt.
// Caches permanent refusal if the loaded PerfUI.dll predates the requested version.
inline API::IPerfUI_v1* GetApi() {
    if (Detail::s_unsupportedV1.load(std::memory_order_relaxed)) return nullptr;
    auto* ptr = Detail::s_cachedV1.load(std::memory_order_acquire);
    if (ptr) return ptr;

    bool hostLoaded = false;
    ptr = static_cast<API::IPerfUI_v1*>(Detail::QueryHostApi(API::InterfaceVersion_1, sizeof(API::IPerfUI_v1), &hostLoaded));
    if (ptr) {
        Detail::s_cachedV1.store(ptr, std::memory_order_release);
    } else if (hostLoaded) {
        Detail::s_unsupportedV1.store(true, std::memory_order_release);
    }
    return ptr;
}

inline UIContext* GetContext() {
    auto* api = GetApi();
    return api ? api->GetContext() : nullptr;
}

// Returns nullptr when the loaded PerfUI.dll predates interface v2.
inline API::IPerfUI_v2* GetApiV2() {
    if (Detail::s_unsupportedV2.load(std::memory_order_relaxed)) return nullptr;
    auto* ptr = Detail::s_cachedV2.load(std::memory_order_acquire);
    if (ptr) return ptr;

    bool hostLoaded = false;
    ptr = static_cast<API::IPerfUI_v2*>(Detail::QueryHostApi(API::InterfaceVersion_2, sizeof(API::IPerfUI_v2), &hostLoaded));
    if (ptr) {
        Detail::s_cachedV2.store(ptr, std::memory_order_release);
    } else if (hostLoaded) {
        Detail::s_unsupportedV2.store(true, std::memory_order_release);
    }
    return ptr;
}

// Returns nullptr when the loaded PerfUI.dll predates interface v3 or was built without Lua.
inline API::IPerfUI_v3* GetApiV3() {
    if (Detail::s_unsupportedV3.load(std::memory_order_relaxed)) return nullptr;
    auto* ptr = Detail::s_cachedV3.load(std::memory_order_acquire);
    if (ptr) return ptr;

    bool hostLoaded = false;
    ptr = static_cast<API::IPerfUI_v3*>(Detail::QueryHostApi(API::InterfaceVersion_3, sizeof(API::IPerfUI_v3), &hostLoaded));
    if (ptr) {
        Detail::s_cachedV3.store(ptr, std::memory_order_release);
    } else if (hostLoaded) {
        Detail::s_unsupportedV3.store(true, std::memory_order_release);
    }
    return ptr;
}

// Returns nullptr when the loaded PerfUI.dll predates interface v4.
inline API::IPerfUI_v4* GetApiV4() {
    if (Detail::s_unsupportedV4.load(std::memory_order_relaxed)) return nullptr;
    auto* ptr = Detail::s_cachedV4.load(std::memory_order_acquire);
    if (ptr) return ptr;

    bool hostLoaded = false;
    ptr = static_cast<API::IPerfUI_v4*>(Detail::QueryHostApi(API::InterfaceVersion_4, sizeof(API::IPerfUI_v4), &hostLoaded));
    if (ptr) {
        Detail::s_cachedV4.store(ptr, std::memory_order_release);
    } else if (hostLoaded) {
        Detail::s_unsupportedV4.store(true, std::memory_order_release);
    }
    return ptr;
}

// Returns nullptr when the loaded PerfUI.dll predates interface v5.
inline API::IPerfUI_v5* GetApiV5() {
    if (Detail::s_unsupportedV5.load(std::memory_order_relaxed)) return nullptr;
    auto* ptr = Detail::s_cachedV5.load(std::memory_order_acquire);
    if (ptr) return ptr;

    bool hostLoaded = false;
    ptr = static_cast<API::IPerfUI_v5*>(Detail::QueryHostApi(API::InterfaceVersion_5, sizeof(API::IPerfUI_v5), &hostLoaded));
    if (ptr) {
        Detail::s_cachedV5.store(ptr, std::memory_order_release);
    } else if (hostLoaded) {
        Detail::s_unsupportedV5.store(true, std::memory_order_release);
    }
    return ptr;
}

/**
 * @brief Check whether PerfUI.dll is loaded and the requested interface version is ready.
 * @param minVersion Minimum required interface version (default InterfaceVersion_1).
 * Recommended registration point for mod settings is after SKSE DataLoaded event (kDataLoaded).
 */
inline bool IsApiReady(unsigned long minVersion = API::InterfaceVersion_1) {
    switch (minVersion) {
    case API::InterfaceVersion_1: return GetApi() != nullptr;
    case API::InterfaceVersion_2: return GetApiV2() != nullptr;
    case API::InterfaceVersion_3: return GetApiV3() != nullptr;
    case API::InterfaceVersion_4: return GetApiV4() != nullptr;
    case API::InterfaceVersion_5: return GetApiV5() != nullptr;
    default: return false;
    }
}

inline bool RegisterModSettingsJson(const char* modId, const char* displayName, const char* jsonPath) {
    auto* api = GetApiV4();
    return api && api->RegisterModSettingsJson ? api->RegisterModSettingsJson(modId, displayName, jsonPath) : false;
}

inline bool RegisterModSettingsPage(const char* modId, const char* displayName, void (*build)(void*, API::SettingsPageBuilder*), void* userData) {
    auto* api = GetApiV4();
    return api && api->RegisterModSettingsPage ? api->RegisterModSettingsPage(modId, displayName, build, userData) : false;
}

inline void OpenSettings(const char* modId = nullptr) {
    auto* api = GetApiV4();
    if (api && api->OpenSettings) {
        api->OpenSettings(modId);
    }
}

inline void CloseSettings() {
    auto* api = GetApiV4();
    if (api && api->CloseSettings) {
        api->CloseSettings();
    }
}

inline bool GetSettingBool(const char* modId, const char* key, bool defaultVal = false) {
    auto* api = GetApiV4();
    return api && api->GetSettingBool ? api->GetSettingBool(modId, key, defaultVal) : defaultVal;
}

inline void SetSettingBool(const char* modId, const char* key, bool val) {
    auto* api = GetApiV4();
    if (api && api->SetSettingBool) api->SetSettingBool(modId, key, val);
}

inline int32_t GetSettingInt(const char* modId, const char* key, int32_t defaultVal = 0) {
    auto* api = GetApiV4();
    return api && api->GetSettingInt ? api->GetSettingInt(modId, key, defaultVal) : defaultVal;
}

inline void SetSettingInt(const char* modId, const char* key, int32_t val) {
    auto* api = GetApiV4();
    if (api && api->SetSettingInt) api->SetSettingInt(modId, key, val);
}

inline float GetSettingFloat(const char* modId, const char* key, float defaultVal = 0.0f) {
    auto* api = GetApiV4();
    return api && api->GetSettingFloat ? api->GetSettingFloat(modId, key, defaultVal) : defaultVal;
}

inline void SetSettingFloat(const char* modId, const char* key, float val) {
    auto* api = GetApiV4();
    if (api && api->SetSettingFloat) api->SetSettingFloat(modId, key, val);
}

inline size_t GetSettingStringCopy(const char* modId, const char* key, char* outBuf, size_t outBufSize, const char* defaultVal = "") {
    auto* api5 = GetApiV5();
    if (api5 && api5->GetSettingStringCopy) {
        return api5->GetSettingStringCopy(modId, key, outBuf, outBufSize, defaultVal);
    }
    auto* api4 = GetApiV4();
    if (api4 && api4->GetSettingString) {
        const char* s = api4->GetSettingString(modId, key, defaultVal);
        if (!s) return 0;
        size_t len = std::strlen(s);
        if (!outBuf || outBufSize == 0) return len + 1;
        size_t toCopy = (std::min)(len, outBufSize - 1);
        if (toCopy < len) {
            toCopy = Detail::ClampUtf8ToBoundary(s, toCopy);
        }
        std::memcpy(outBuf, s, toCopy);
        outBuf[toCopy] = '\0';
        return toCopy;
    }
    if (outBuf && outBufSize > 0) outBuf[0] = '\0';
    return 0;
}

inline std::string GetSettingString(const char* modId, const char* key, const std::string& defaultVal = "") {
    auto* api5 = GetApiV5();
    if (api5 && api5->GetSettingStringCopy) {
        char stackBuf[256];
        size_t copied = api5->GetSettingStringCopy(modId, key, stackBuf, sizeof(stackBuf), defaultVal.c_str());
        // Fast path: if copied < sizeof(stackBuf) - 4, the string was not truncated
        if (copied < sizeof(stackBuf) - 4) {
            return std::string(stackBuf, copied);
        }

        // Accurate dynamic path for longer strings with up to 3 retries against concurrent modification
        for (int retry = 0; retry < 3; ++retry) {
            size_t needed = api5->GetSettingStringCopy(modId, key, nullptr, 0, defaultVal.c_str());
            if (needed <= 1) return "";
            std::string dynBuf;
            dynBuf.resize(needed);
            size_t actualCopied = api5->GetSettingStringCopy(modId, key, dynBuf.data(), dynBuf.size(), defaultVal.c_str());
            if (actualCopied + 1 <= needed) {
                dynBuf.resize(actualCopied);
                return dynBuf;
            }
        }
        size_t needed = api5->GetSettingStringCopy(modId, key, nullptr, 0, defaultVal.c_str());
        std::string dynBuf(needed, '\0');
        size_t actualCopied = api5->GetSettingStringCopy(modId, key, dynBuf.data(), dynBuf.size(), defaultVal.c_str());
        dynBuf.resize(actualCopied);
        return dynBuf;
    }
    auto* api4 = GetApiV4();
    if (api4 && api4->GetSettingString) {
        const char* s = api4->GetSettingString(modId, key, defaultVal.c_str());
        return s ? std::string(s) : defaultVal;
    }
    return defaultVal;
}

inline void SetSettingString(const char* modId, const char* key, const char* val) {
    auto* api = GetApiV4();
    if (api && api->SetSettingString) api->SetSettingString(modId, key, val);
}

inline uint32_t GetSettingColor(const char* modId, const char* key, uint32_t defaultRgba = 0xFFFFFFFF) {
    auto* api = GetApiV4();
    return api && api->GetSettingColor ? api->GetSettingColor(modId, key, defaultRgba) : defaultRgba;
}

inline void SetSettingColor(const char* modId, const char* key, uint32_t rgba) {
    auto* api = GetApiV4();
    if (api && api->SetSettingColor) api->SetSettingColor(modId, key, rgba);
}

inline uint32_t GetSettingKey(const char* modId, const char* key, uint32_t defaultKey = 0) {
    auto* api = GetApiV4();
    return api && api->GetSettingKey ? api->GetSettingKey(modId, key, defaultKey) : defaultKey;
}

inline void SetSettingKey(const char* modId, const char* key, uint32_t keycode) {
    auto* api = GetApiV4();
    if (api && api->SetSettingKey) api->SetSettingKey(modId, key, keycode);
}

inline uint64_t SubscribeSettingChanged(const char* modId, const char* key, void (*cb)(void* userData, const char* modId, const char* key), void* userData) {
    auto* api = GetApiV4();
    return api && api->SubscribeSettingChanged ? api->SubscribeSettingChanged(modId, key, cb, userData) : 0;
}

inline void UnsubscribeSettingChanged(uint64_t subscriptionId) {
    auto* api = GetApiV4();
    if (api && api->UnsubscribeSettingChanged) api->UnsubscribeSettingChanged(subscriptionId);
}

inline ScriptHost* CreateScriptHost(const char* modName) {
    auto* api = GetApiV3();
    if (api && api->CreateScriptHost) {
        return api->CreateScriptHost(modName, API::PERFUI_LUA_API_VERSION);
    }
    return nullptr;
}

inline void DestroyScriptHost(ScriptHost* host) {
    auto* api = GetApiV3();
    if (api && api->DestroyScriptHost && host) {
        api->DestroyScriptHost(host);
    }
}

inline TextureId RegisterExternalTexture(void* srv, uint32_t width, uint32_t height) {
    auto* api = GetApiV3();
    if (api && api->RegisterExternalTexture) {
        return api->RegisterExternalTexture(srv, width, height);
    }
    auto* ctx = GetContext();
    return ctx ? ctx->registerExternalTexture(srv, width, height) : 0;
}

inline void UnregisterExternalTexture(TextureId id) {
    auto* api = GetApiV3();
    if (api && api->UnregisterExternalTexture) {
        api->UnregisterExternalTexture(id);
        return;
    }
    auto* ctx = GetContext();
    if (ctx) {
        ctx->unregisterExternalTexture(id);
    }
}

inline uint32_t TrackPosition(float x, float y, float z, bool checkLoS = false) {
    auto* api = GetApiV3();
    if (api && api->TrackPosition) {
        return api->TrackPosition(x, y, z, checkLoS);
    }
    return 0;
}

inline uint32_t TrackActor(uint32_t formId, float offsetX = 0.0f, float offsetY = 0.0f, float offsetZ = 120.0f, bool checkLoS = false) {
    auto* api = GetApiV3();
    if (api && api->TrackActor) {
        return api->TrackActor(formId, offsetX, offsetY, offsetZ, checkLoS);
    }
    return 0;
}

inline void Untrack(uint32_t targetId) {
    auto* api = GetApiV3();
    if (api && api->Untrack) {
        api->Untrack(targetId);
    }
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

inline TextureId LoadSvg(const char* filePath, float targetWidth = 0.0f, float targetHeight = 0.0f) {
    auto* api = GetApi();
    if (api && api->LoadSvg) {
        return api->LoadSvg(filePath, targetWidth, targetHeight);
    }
    auto* ctx = GetContext();
    return ctx ? ctx->loadSvg(filePath, targetWidth, targetHeight) : 0;
}

inline TextureId LoadSvgFromMemory(const char* svgContent, float targetWidth = 0.0f, float targetHeight = 0.0f) {
    auto* api = GetApi();
    if (api && api->LoadSvgFromMemory) {
        return api->LoadSvgFromMemory(svgContent, targetWidth, targetHeight);
    }
    auto* ctx = GetContext();
    return ctx ? ctx->loadSvgFromMemory(svgContent, targetWidth, targetHeight) : 0;
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

// v2: stop gameplay keyboard input while an overlay drives a modal interaction.
// Returns false when the host does not implement v2 (older PerfUI.dll).
inline bool SetCaptureKeyboard(bool capture) {
    auto* api = GetApiV2();
    if (api && api->SetCaptureKeyboard) {
        api->SetCaptureKeyboard(capture);
        return true;
    }
    return false;
}

// Call before SetUIVisible(true) if the window is an overlay on top of the live game world.
inline void SetOverlayMode(bool overlay) {
    auto* api = GetApi();
    if (api && api->SetOverlayMode) {
        api->SetOverlayMode(overlay);
    }
}

} // namespace PerfUI::Client
