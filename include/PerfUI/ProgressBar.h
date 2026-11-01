#pragma once

#include "UIElement.h"
#include "Types.h"
#include "Animation.h"
#include "UIRenderBackend.h"
#include <string>

namespace PerfUI {

class ProgressBar;

class PERFUI_API ProgressBarAnimator : public ElementAnimator {
public:
    explicit ProgressBarAnimator(ProgressBar* bar);
    ProgressBarAnimator& fill(float target, float duration, Easing easing = Easing::EaseOutCubic);
    ProgressBarAnimator& fillColor(Color target, float duration, Easing easing = Easing::Linear);

    void update(float dt);
    bool isFinished() const;

private:
    ProgressBar* m_bar{ nullptr };
    bool m_animFill{ false };
    float m_fromFill{ 0.0f };
    float m_toFill{ 0.0f };
    float m_fillDuration{ 0.3f };
    float m_fillElapsed{ 0.0f };
    Easing m_fillEasing{ Easing::EaseOutCubic };

    bool m_animFillColor{ false };
    Color m_fromFillColor{};
    Color m_toFillColor{};
    float m_colorDuration{ 0.3f };
    float m_colorElapsed{ 0.0f };
    Easing m_colorEasing{ Easing::Linear };
};

class PERFUI_API ProgressBar : public UIElement {
    friend class ProgressBarAnimator;
public:
    explicit ProgressBar(float progress = 0.0f, std::string name = "ProgressBar");
    ~ProgressBar() override = default;

    float progress() const { return m_progress; }
    ProgressBar& progress(float p);

    ProgressBarAnimator& animate();
    ProgressBar& animateFill(float target, float duration, Easing easing = Easing::EaseOutCubic);

    float value() const { return m_progress; }
    ProgressBar& value(float v) { return progress(v); }

    bool showLabel() const { return m_showLabel; }
    ProgressBar& showLabel(bool show) { m_showLabel = show; return *this; }

    ProgressBar& fillColor(Color color) { m_fillColor = color; return *this; }
    ProgressBar& trackColor(Color color) { m_trackColor = color; return *this; }
    ProgressBar& height(float h) { m_barHeight = h; markLayoutDirty(); return *this; }
    ProgressBar& cornerRadius(float r) { m_cornerRadius = r; return *this; }

    const std::string& text() const { return m_text; }
    ProgressBar& text(std::string t) { m_text = std::move(t); return *this; }
    ProgressBar& setText(std::string t) { return text(std::move(t)); }

    const std::string& secondaryText() const { return m_secondaryText; }
    ProgressBar& secondaryText(std::string t) { m_secondaryText = std::move(t); return *this; }
    ProgressBar& setSecondaryText(std::string t) { return secondaryText(std::move(t)); }

    bool isGlossy() const { return m_glossy; }
    ProgressBar& glossy(bool g) { m_glossy = g; return *this; }

    bool hasDropShadow() const { return m_dropShadow; }
    ProgressBar& dropShadow(bool s) { m_dropShadow = s; return *this; }

    Color borderColor() const { return m_borderColor; }
    ProgressBar& borderColor(Color c) { m_borderColor = c; return *this; }

    float borderWidth() const { return m_borderWidth; }
    ProgressBar& borderWidth(float w) { m_borderWidth = w; return *this; }

    ProgressBar& fontSize(float s) {
        m_textStyle.fontSize = s;
        m_secondaryTextStyle.fontSize = (std::max)(8.0f, s - 1.5f);
        return *this;
    }

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

private:
    float m_progress{ 0.0f }; // 0.0f to 1.0f
    AnimatedFloat m_animProgress{ 0.0f, 7.0f };
    std::unique_ptr<ProgressBarAnimator> m_barAnimator;
    bool m_showLabel{ true };
    float m_barHeight{ 14.0f };
    float m_cornerRadius{ 5.0f };

    std::string m_text;
    std::string m_secondaryText;
    bool m_glossy{ true };
    bool m_dropShadow{ true };
    Color m_borderColor{ 45, 52, 65, 200 };
    float m_borderWidth{ 1.0f };

    Color m_trackColor{ 20, 24, 32, 220 };
    Color m_trackBorder{ Color::BorderSubtle() };
    Color m_fillColor{ Color::NordicGold() };
    TextStyle m_textStyle{ Color::White(), 11.0f, true, false };
    TextStyle m_secondaryTextStyle{ Color(180, 235, 255, 245), 9.5f, true, false };
};

} // namespace PerfUI
