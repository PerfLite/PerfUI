#include "PerfUI/Panel.h"
#include "PerfUI/Theme.h"

namespace PerfUI {

Panel::Panel(std::string name)
    : UIElement(std::move(name))
{
}

void Panel::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

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
        backend.drawShadow(m_bounds, radius, m_shadowColor, m_shadowBlur, m_shadowOffset);
    } else if (resolvedBox.shadow.has_value() && !m_backgroundColor.has_value()) {
        backend.drawShadow(m_bounds, radius, resolvedBox.shadow->color, resolvedBox.shadow->blur, resolvedBox.shadow->offset);
    }

    if (bg.a > 0 || (border.a > 0 && borderW > 0.0f)) {
        backend.drawRoundedRect(
            m_bounds,
            bg,
            radius,
            border,
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
