#pragma once

#include "UIElement.h"
#include "Types.h"

namespace PerfUI {

class PERFUI_API Panel : public UIElement {
public:
    explicit Panel(std::string name = "Panel");
    ~Panel() override = default;

    Color backgroundColor() const { return m_backgroundColor.value_or(Color::Transparent()); }
    Panel& backgroundColor(Color color) { m_backgroundColor = color; return *this; }

    Color borderColor() const { return m_borderColor.value_or(Color::Transparent()); }
    Panel& borderColor(Color color) { m_borderColor = color; return *this; }

    float borderWidth() const { return m_borderWidth.value_or(0.0f); }
    Panel& borderWidth(float width) { m_borderWidth = width; return *this; }

    float cornerRadius() const { return m_cornerRadius.value_or(0.0f); }
    Panel& cornerRadius(float radius) { m_cornerRadius = radius; return *this; }

    bool hasShadow() const { return m_hasShadow; }
    Panel& shadow(bool enable, Color color = Color(0, 0, 0, 160), float blur = 16.0f, Point offset = { 0.0f, 6.0f }) {
        m_hasShadow = enable;
        m_shadowColor = color;
        m_shadowBlur = blur;
        m_shadowOffset = offset;
        return *this;
    }

    bool clipChildren() const { return m_clipChildren; }
    Panel& clipChildren(bool clip) { m_clipChildren = clip; return *this; }

    using UIElement::style;
    Panel& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void render(UIRenderBackend& backend) override;

private:
    std::optional<Color> m_backgroundColor;
    std::optional<Color> m_borderColor;
    std::optional<float> m_borderWidth;
    std::optional<float> m_cornerRadius;

    bool m_hasShadow{ false };
    Color m_shadowColor{ 0, 0, 0, 160 };
    float m_shadowBlur{ 16.0f };
    Point m_shadowOffset{ 0.0f, 6.0f };

    bool m_clipChildren{ false };
};

} // namespace PerfUI
