#pragma once

#include "UIElement.h"
#include "Types.h"
#include "Animation.h"
#include "UIRenderBackend.h"
#include <string>

namespace PerfUI {

class PERFUI_API ProgressBar : public UIElement {
public:
    explicit ProgressBar(float progress = 0.0f, std::string name = "ProgressBar");
    ~ProgressBar() override = default;

    float progress() const { return m_progress; }
    ProgressBar& progress(float p);

    float value() const { return m_progress; }
    ProgressBar& value(float v) { return progress(v); }

    bool showLabel() const { return m_showLabel; }
    ProgressBar& showLabel(bool show) { m_showLabel = show; return *this; }

    ProgressBar& fillColor(Color color) { m_fillColor = color; return *this; }
    ProgressBar& trackColor(Color color) { m_trackColor = color; return *this; }
    ProgressBar& height(float h) { m_barHeight = h; markLayoutDirty(); return *this; }
    ProgressBar& cornerRadius(float r) { m_cornerRadius = r; return *this; }

    void update(float deltaTime) override;
    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

private:
    float m_progress{ 0.0f }; // 0.0f to 1.0f
    AnimatedFloat m_animProgress{ 0.0f, 7.0f };
    bool m_showLabel{ true };
    float m_barHeight{ 14.0f };
    float m_cornerRadius{ 5.0f };

    Color m_trackColor{ 20, 24, 32, 220 };
    Color m_trackBorder{ Color::BorderSubtle() };
    Color m_fillColor{ Color::NordicGold() };
    TextStyle m_textStyle{ Color::White(), 10.0f, true, false };
};

} // namespace PerfUI
