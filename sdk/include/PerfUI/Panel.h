#pragma once

#include "UIElement.h"
#include "Types.h"

namespace PerfUI {

class PERFUI_API Panel : public UIElement {
public:
    explicit Panel(std::string name = "Panel");
    ~Panel() override = default;

    Color backgroundColor() const { return m_backgroundColor; }
    Panel& backgroundColor(Color color) { m_backgroundColor = color; return *this; }

    Color borderColor() const { return m_borderColor; }
    Panel& borderColor(Color color) { m_borderColor = color; return *this; }

    float borderWidth() const { return m_borderWidth; }
    Panel& borderWidth(float width) { m_borderWidth = width; return *this; }

    float cornerRadius() const { return m_cornerRadius; }
    Panel& cornerRadius(float radius) { m_cornerRadius = radius; return *this; }

    bool hasShadow() const { return m_hasShadow; }
    Panel& shadow(bool enable, Color color = Color(0, 0, 0, 160), float blur = 16.0f, Point offset = { 0.0f, 6.0f }) {
        m_hasShadow = enable;
        m_shadowColor = color;
        m_shadowBlur = blur;
        m_shadowOffset = offset;
        return *this;
    }

    void render(UIRenderBackend& backend) override;

private:
    Color m_backgroundColor{ Color::BackgroundElevated() };
    Color m_borderColor{ Color::BorderSubtle() };
    float m_borderWidth{ 1.0f };
    float m_cornerRadius{ 8.0f };

    bool m_hasShadow{ false };
    Color m_shadowColor{ 0, 0, 0, 160 };
    float m_shadowBlur{ 16.0f };
    Point m_shadowOffset{ 0.0f, 6.0f };
};

} // namespace PerfUI
