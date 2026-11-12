#pragma once

#include "UIElement.h"
#include "Types.h"

namespace PerfUI {

class PERFUI_API Panel : public UIElement {
public:
    explicit Panel(std::string name = "Panel");
    ~Panel() override;

    Color backgroundColor() const { return m_backgroundColor.value_or(Color::Transparent()); }
    Panel& backgroundColor(Color color) { m_backgroundColor = color; return *this; }

    Color borderColor() const { return m_borderColor.value_or(Color::Transparent()); }
    Panel& borderColor(Color color) { m_borderColor = color; return *this; }

    float borderWidth() const { return m_borderWidth.value_or(0.0f); }
    Panel& borderWidth(float width) { m_borderWidth = width; return *this; }

    float cornerRadius() const { return m_cornerRadius.value_or(0.0f); }
    Panel& cornerRadius(float radius) { m_cornerRadius = radius; return *this; }

    Color outlineColor() const { return m_outlineColor.value_or(UIElement::outlineColor().value_or(Color::Transparent())); }
    Panel& outlineColor(Color color) { m_outlineColor = color; return *this; }

    float outlineWidth() const { return m_outlineWidth.value_or(UIElement::outlineWidth().value_or(0.0f)); }
    Panel& outlineWidth(float width) { m_outlineWidth = width; return *this; }

    float outlineOffset() const { return m_outlineOffset.value_or(UIElement::outlineOffset().value_or(0.0f)); }
    Panel& outlineOffset(float offset) { m_outlineOffset = offset; return *this; }

    Panel& outline(Color color, float width, float offset = 0.0f) {
        m_outlineColor = color;
        m_outlineWidth = width;
        m_outlineOffset = offset;
        return *this;
    }

    const std::optional<LinearGradientDef>& backgroundGradient() const { return m_backgroundGradient; }
    Panel& backgroundGradient(const LinearGradientDef& grad) { m_backgroundGradient = grad; return *this; }
    Panel& backgroundGradient(std::optional<LinearGradientDef> grad) { m_backgroundGradient = grad; return *this; }

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

    bool backdropBlur() const { return m_backdropBlur; }
    float blurRadius() const { return m_blurRadius; }
    Panel& backdropBlur(bool enable, float radius = 1.0f) {
        m_backdropBlur = enable;
        m_blurRadius = radius;
        return *this;
    }

    bool liquidGlass() const { return m_liquidGlass; }
    Panel& liquidGlass(bool enable) {
        m_liquidGlass = enable;
        return *this;
    }

    TextureId backgroundImage() const;
    Panel& backgroundImage(TextureId texture);
    Panel& backgroundImageUV(Point uv0, Point uv1);
    Panel& backgroundImageSlice(Insets sourcePixels, Insets destinationPixels);

    using UIElement::style;
    Panel& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    void render(UIRenderBackend& backend) override;
    void onStyleApplied(const ComputedStyle& style) override;

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
    bool m_backdropBlur{ false };
    float m_blurRadius{ 1.0f };
    bool m_liquidGlass{ false };

    std::optional<Color> m_outlineColor;
    std::optional<float> m_outlineWidth;
    std::optional<float> m_outlineOffset;
    std::optional<LinearGradientDef> m_backgroundGradient;
};

} // namespace PerfUI
