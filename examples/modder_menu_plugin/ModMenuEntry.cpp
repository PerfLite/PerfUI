/**
 * @file ModMenuEntry.cpp
 * @brief Minimal example showing how a third-party SKSE mod opens a PerfUI menu window on hotkey or event.
 */

#include "PluginMenu.h"
#include "PerfUI/PerfUIApi.h"

// Stored pointer to active mod menu
static PerfUI::Examples::PluginMenu* g_modMenu = nullptr;

/**
 * @brief Toggles the mod configuration menu.
 * Can be bound to a hotkey (e.g. F11, Insert, or MCM hotkey).
 */
void ToggleModConfigurationMenu() {
    auto* ctx = PerfUI::Client::GetContext();
    if (!ctx || !ctx->root()) return;

    if (g_modMenu && g_modMenu->isVisible()) {
        g_modMenu->close();
        return;
    }

    if (!g_modMenu) {
        g_modMenu = ctx->root()->add<PerfUI::Examples::PluginMenu>("MyModSettings");
        
        // Center the window on screen
        auto vp = ctx->viewportSize();
        g_modMenu->layout().width(460.0f).height(380.0f);
        g_modMenu->setBounds(PerfUI::Rect{
            (vp.width - 460.0f) * 0.5f,
            (vp.height - 380.0f) * 0.5f,
            460.0f,
            380.0f
        });

        // Register custom callbacks
        g_modMenu->setOnToggleFeature([](bool enabled) {
            // Apply setting to your SKSE mod internals
            (void)enabled;
        });

        g_modMenu->setOnScaleChanged([](float scale) {
            // Apply scale to HUD/game elements
            (void)scale;
        });
    }

    g_modMenu->show();
    ctx->setFocus(g_modMenu);
}
