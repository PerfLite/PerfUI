/**
 * @file ModEntry.cpp
 * @brief Minimal example showing how a third-party SKSE modder registers a custom HUD with PerfUI.
 * 
 * Total code required: ~25 lines!
 */

#include "CustomHealthBar.h"
#include "PerfUI/PerfUIApi.h"

// Stored pointer to our custom widget
static PerfUI::Examples::CustomHealthBar* g_customHud = nullptr;

/**
 * @brief Called once by SKSE or during mod initialization.
 */
void InitializeCustomModUI() {
    // 1. Get the global PerfUI context via client API
    auto* ctx = PerfUI::Client::GetContext();
    if (!ctx || !ctx->root()) {
        return;
    }

    // 2. Add our custom Health Bar directly to the root element
    g_customHud = ctx->root()->add<PerfUI::Examples::CustomHealthBar>();

    // 3. Set initial position (e.g. pinned at X=30, Y=620)
    g_customHud->setScreenPosition(PerfUI::Point{ 30.0f, 620.0f });

    // 4. (Optional) Show a friendly toast confirmation
    PerfUI::Client::ShowToast("Custom HUD Loaded", "Modder Health Bar active!", PerfUI::ToastType::Success, 3.0f);
}

/**
 * @brief Called whenever the player takes damage or heals in Skyrim.
 */
void OnPlayerHealthChanged(float currentHealth, float maxHealth) {
    if (g_customHud) {
        g_customHud->setHealth(currentHealth, maxHealth);
    }
}
