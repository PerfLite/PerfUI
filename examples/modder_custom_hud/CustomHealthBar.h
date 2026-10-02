#pragma once

#include "PerfUI/Panel.h"
#include "PerfUI/ProgressBar.h"
#include "PerfUI/Text.h"
#include "PerfUI/Animation.h"
#include "PerfUI/Theme.h"
#include "PerfUI/UIContext.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace PerfUI::Examples {

/**
 * @brief Production-grade Custom Health Bar HUD Widget for Skyrim Modders.
 * 
 * Demonstrates:
 *  - Retained hierarchy: Panel -> [Label, ProgressBar, ValueText]
 *  - Flexbox automatic alignment and padding
 *  - Dynamic low-health pulse animation
 *  - Audio feedback and toast integration via PerfUI
 */
class CustomHealthBar : public Panel {
public:
    explicit CustomHealthBar(std::string name = "CustomHealthBar")
        : Panel(std::move(name))
    {
        // 1. Panel Visual Styling: Elevated Nordic Dark Glass with subtle gold/red border
        backgroundColor(Color(14, 18, 26, 215));
        borderColor(Color::BorderSubtle());
        borderWidth(1.2f);
        cornerRadius(8.0f);
        shadow(true, Color(0, 0, 0, 180), 16.0f, { 0.0f, 4.0f });

        // 2. Horizontal Flexbox Layout
        layout().direction(LayoutDirection::Horizontal)
                .alignment(Alignment::Center)
                .padding(12.0f, 6.0f)
                .gap(10.0f);

        // 3. Heart / HP Label
        m_label = add<Text>("HP");
        m_label->color(Color(255, 80, 80, 255)).fontSize(12.0f).bold(true);

        // 4. Smooth Animated Progress Bar
        m_bar = add<ProgressBar>(1.0f);
        m_bar->layout().width(180.0f).height(12.0f);
        m_bar->fillColor(Color(210, 40, 40, 255));
        m_bar->trackColor(Color(30, 16, 16, 220));
        m_bar->cornerRadius(4.0f);

        // 5. Numeric Text Display
        m_valText = add<Text>("100 / 100");
        m_valText->color(Color(220, 225, 235, 240)).fontSize(11.5f);

        // Default screen position (Bottom-left HUD overlay)
        m_screenPos = Point{ 40.0f, 640.0f };
    }

    /**
     * @brief Update health values and visual states.
     * @param current Current HP
     * @param max Maximum HP
     */
    void setHealth(float current, float max) {
        m_currentHealth = (std::max)(0.0f, current);
        m_maxHealth = (std::max)(1.0f, max);

        float ratio = (std::clamp)(m_currentHealth / m_maxHealth, 0.0f, 1.0f);
        m_bar->progress(ratio);

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.0f / %.0f", m_currentHealth, m_maxHealth);
        m_valText->text(buf);

        // Critical HP state (< 25%)
        bool wasCritical = m_isCritical;
        m_isCritical = (ratio <= 0.25f);

        if (m_isCritical && !wasCritical && context()) {
            context()->playSound("UIMenuCancel");
            context()->showToast("Low Health Warning", "Health dropped below 25%!", ToastType::Warning, 2.0f);
        }
    }

    void setScreenPosition(Point pos) {
        m_screenPos = pos;
        markLayoutDirty();
    }

    void update(float deltaTime) override {
        Panel::update(deltaTime);

        // Low health pulsing heartbeat effect
        if (m_isCritical) {
            m_pulseTimer += deltaTime * 5.0f;
            float pulse = (std::sin(m_pulseTimer) + 1.0f) * 0.5f; // 0.0 to 1.0
            
            Color glowBorder = Color::Lerp(Color(255, 40, 40, 160), Color(255, 100, 100, 255), pulse);
            borderColor(glowBorder);
            m_bar->fillColor(Color::Lerp(Color(180, 20, 20, 255), Color(255, 50, 50, 255), pulse));
        } else {
            borderColor(Color::BorderSubtle());
            m_bar->fillColor(Color(210, 40, 40, 255));
            m_pulseTimer = 0.0f;
        }
    }

    void measure(Dimensions availableSize) override {
        Panel::measure(availableSize);
    }

    void arrange(const Rect& finalRect) override {
        // Place widget at custom HUD screen coordinates
        float w = desiredSize().width > 0.0f ? desiredSize().width : 280.0f;
        float h = desiredSize().height > 0.0f ? desiredSize().height : 32.0f;

        float x = m_screenPos.x;
        float y = m_screenPos.y;

        // Auto-clamp to screen bounds
        if (finalRect.width > 0.0f) {
            x = (std::clamp)(x, 8.0f, (std::max)(8.0f, finalRect.width - w - 8.0f));
            y = (std::clamp)(y, 8.0f, (std::max)(8.0f, finalRect.height - h - 8.0f));
        }

        Panel::arrange(Rect{ x, y, w, h });
    }

private:
    Text* m_label{ nullptr };
    ProgressBar* m_bar{ nullptr };
    Text* m_valText{ nullptr };

    float m_currentHealth{ 100.0f };
    float m_maxHealth{ 100.0f };
    bool m_isCritical{ false };
    float m_pulseTimer{ 0.0f };
    Point m_screenPos{ 40.0f, 640.0f };
};

} // namespace PerfUI::Examples
