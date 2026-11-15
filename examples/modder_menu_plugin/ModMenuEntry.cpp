/**
 * @file ModMenuEntry.cpp
 * @brief SKSE Mod Settings Layer Example for PerfUI (MCM Modern Alternative).
 *
 * Demonstrates:
 *  1. Registering a declarative mod settings page via JSON schema (recommended).
 *  2. Registering an interactive mod settings page via C-API Page Builder.
 *  3. Reading settings at runtime with zero overhead.
 *  4. Subscribing to setting changes reactively without polling.
 *  5. Binding a persistent hotkey to open the Settings Window.
 */

#include "PerfUI/PerfUIApi.h"
#include <iostream>

// Unique mod identifier
static constexpr const char* MOD_ID = "MyAwesomeMod";

// Cached runtime values for high-frequency game loop checks
static bool g_hudEnabled = true;
static float g_hudScale = 100.0f;
static uint32_t g_accentColor = 0xFFFFFFFF;
static uint64_t g_scaleSubId = 0;

/**
 * @brief Example C++ Page Builder callback for programmatic menu construction.
 */
static void BuildCustomSettingsPage(void* userData, PerfUI::API::SettingsPageBuilder* builder) {
    if (!builder) return;

    builder->AddHeader(builder, "C++ Mod Configuration");
    builder->AddText(builder, "This page was constructed programmatically via IPerfUI_v4 API.");
    
    builder->AddToggle(builder, "Enable Advanced Telemetry", "enableTelemetry", 
                       "Sends anonymous performance metrics to the game log.", false);
    
    builder->AddSlider(builder, "Logging Verbosity", "logVerbosity", 
                       "Sets diagnostic log level (0=Off, 1=Info, 2=Debug).", 
                       0.0f, 2.0f, 1.0f, 1.0f, "Level %.0f");
    
    builder->AddKeybind(builder, "Telemetry Toggle Key", "telemetryKey", 
                        "Hotkey to dump telemetry data to console.", 0);
}

/**
 * @brief Initialize mod settings during SKSE plugin load.
 */
void InitializeModSettings() {
    // 1. Declarative JSON Schema Registration
    // Automatically parses perfui.settings.json, registers definitions, and mounts page to SettingsWindow.
    bool jsonOk = PerfUI::Client::RegisterModSettingsJson(
        MOD_ID,
        "My Awesome Mod",
        "Data/SKSE/Plugins/MyAwesomeMod/perfui.settings.json"
    );

    if (jsonOk) {
        std::cout << "[MyAwesomeMod] Registered JSON settings page." << std::endl;
    }

    // 2. Programmatic C++ Page Registration
    PerfUI::Client::RegisterModSettingsPage(
        "MyAwesomeMod_CPP",
        "My Awesome Mod (C++ Page)",
        BuildCustomSettingsPage,
        nullptr
    );

    // 3. Read initial settings from persistent store (with fallback defaults)
    g_hudEnabled = PerfUI::Client::GetSettingBool(MOD_ID, "enableDynamicHud", true);
    g_hudScale = PerfUI::Client::GetSettingFloat(MOD_ID, "hudScale", 100.0f);
    g_accentColor = PerfUI::Client::GetSettingColor(MOD_ID, "hudAccentColor", 0x3399FFCC);

    // 4. Reactive subscription to settings changes
    // When the user adjusts a slider or toggle in the UI, this callback is fired automatically.
    g_scaleSubId = PerfUI::Client::SubscribeSettingChanged(MOD_ID, "hudScale", [](void* /*userData*/, const char* modId, const char* key) {
        g_hudScale = PerfUI::Client::GetSettingFloat(modId, key, 100.0f);
        std::cout << "[MyAwesomeMod] HUD Scale updated in real-time to: " << g_hudScale << "%" << std::endl;
    }, nullptr);

    // 5. Register persistent hotkey to open settings directly to our mod's page
    auto* ctx = PerfUI::Client::GetContext();
    if (ctx) {
        // Hotkey code 67 = 'F9' or custom key. Hotkey is automatically saved and rebindable!
        ctx->registerHotkey("MyAwesomeMod:openMenu", 67, []() {
            PerfUI::Client::OpenSettings(MOD_ID);
        });
    }
}

/**
 * @brief Clean up subscriptions on plugin unload.
 */
void ShutdownModSettings() {
    if (g_scaleSubId != 0) {
        PerfUI::Client::UnsubscribeSettingChanged(g_scaleSubId);
        g_scaleSubId = 0;
    }
}
