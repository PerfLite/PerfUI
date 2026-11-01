#include "PerfUI/Panel.h"
#include "PerfUI/Theme.h"

namespace PerfUI {

Panel::Panel(std::string name)
    : UIElement(std::move(name))
{
}

void Panel::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    float effAlpha = effectiveOpacity();
    if (effAlpha <= 0.001f) return;

    auto applyAlpha = [effAlpha](Color c) -> Color {
        return Color(c.r, c.g, c.b, static_cast<uint8_t>(std::clamp(static_cast<float>(c.a) * effAlpha, 0.0f, 255.0f)));
    };

    WidgetState state = currentState();
    const auto& themePanel = Theme::Current().components.panel;
    BoxStyle themeBox = themePanel.resolveBox(state);
    BoxStyle resolvedBox = resolveBoxStyle(state, themeBox);

    Color bg = m_backgroundColor.has_value() 
        ? *m_backgroundColor 
        : resolvedBox.backgroundColor.value_or(Color::Transparent());
    Color border = m_borderColor.has_value() 
        ? *m_borderColor 
        : resolvedBox.borderColor.value_or(Color::Transparent());
    float borderW = m_borderWidth.has_value() 
        ? *m_borderWidth 
        : resolvedBox.borderWidth.value_or(0.0f);
    float radius = m_cornerRadius.has_value() 
        ? *m_cornerRadius 
        : resolvedBox.cornerRadius.value_or(0.0f);

    if (m_hasShadow && m_shadowColor.a > 0) {
        backend.drawShadow(m_bounds, radius, applyAlpha(m_shadowColor), m_shadowBlur, m_shadowOffset);
    } else if (resolvedBox.shadow.has_value() && !m_backgroundColor.has_value()) {
        backend.drawShadow(m_bounds, radius, applyAlpha(resolvedBox.shadow->color), resolvedBox.shadow->blur, resolvedBox.shadow->offset);
    }

    Color effBg = applyAlpha(bg);
    Color effBorder = applyAlpha(border);
    if (effBg.a > 0 || (effBorder.a > 0 && borderW > 0.0f)) {
        backend.drawRoundedRect(
            m_bounds,
            effBg,
            radius,
            effBorder,
            borderW
        );
    }

    if (m_clipChildren) {
        backend.pushClipRect(m_bounds);
    }

    UIElement::render(backend);

    if (m_clipChildren) {
        backend.popClipRect();
    }
}

} // namespace PerfUI
