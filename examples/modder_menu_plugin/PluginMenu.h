#pragma once

#include "PerfUI/UIWindow.h"
#include "PerfUI/Button.h"
#include "PerfUI/Checkbox.h"
#include "PerfUI/Slider.h"
#include "PerfUI/Text.h"
#include "PerfUI/UIContext.h"
#include <functional>

namespace PerfUI::Examples {

/**
 * @brief Production-grade Custom Menu Window for Skyrim SKSE Modders.
 *
 * Demonstrates:
 *  - Modal / overlay UIWindow with title and close button
 *  - Interactive controls: Sliders, Checkboxes, Action Buttons
 *  - Reactive callbacks triggering in-game mod actions
 *  - Clean layout hierarchy with Flexbox styling
 */
class PluginMenu : public UIWindow {
public:
    explicit PluginMenu(std::string title = "Mod Configuration")
        : UIWindow(std::move(title))
    {
        // Window sizing & position
        layout()
            .width(PerfUI::DimensionConstraint::Fixed(460.0f))
            .height(PerfUI::DimensionConstraint::Fixed(380.0f))
            .padding(16.0f)
            .gap(12.0f);

        // Subtitle / description
        auto* desc = add<Text>("Customize your mod settings with smooth retained UI.");
        desc->color(Color(180, 190, 205, 230)).fontSize(12.0f);

        // Option 1: Feature toggle checkbox
        auto* enableHud = add<Checkbox>("Enable Dynamic Status Bars", true);
        enableHud->onToggle([this](bool enabled) {
            if (m_onToggleFeature) m_onToggleFeature(enabled);
            if (context()) {
                context()->playSound(enabled ? "UIMenuFocus" : "UIMenuCancel");
            }
        });

        // Option 2: HUD Scale Slider
        auto* scaleLabel = add<Text>("HUD Scale: 100%");
        scaleLabel->color(Color(212, 175, 55, 255)).fontSize(12.0f);

        auto* scaleSlider = add<Slider>(100.0f, 50.0f, 150.0f);
        scaleSlider->layout().height(22.0f);
        scaleSlider->onValueChanged([scaleLabel, this](float val) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "HUD Scale: %.0f%%", val);
            scaleLabel->text(buf);
            if (m_onScaleChanged) m_onScaleChanged(val / 100.0f);
        });

        // Option 3: Action Button
        auto* saveBtn = add<Button>("Save & Apply Settings");
        saveBtn->layout().height(36.0f).margin(0.0f, 12.0f, 0.0f, 0.0f);
        saveBtn->onClick([this]() {
            if (context()) {
                context()->playSound("UIMenuOK");
                context()->showToast("Mod Settings Saved", "Configuration applied successfully.", ToastType::Success, 3.0f);
            }
            close();
        });
    }

    void setOnToggleFeature(std::function<void(bool)> cb) { m_onToggleFeature = std::move(cb); }
    void setOnScaleChanged(std::function<void(float)> cb) { m_onScaleChanged = std::move(cb); }

private:
    std::function<void(bool)> m_onToggleFeature;
    std::function<void(float)> m_onScaleChanged;
};

} // namespace PerfUI::Examples
