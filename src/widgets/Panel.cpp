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

    Color bg = resolvedBox.backgroundColor.value_or(m_backgroundColor);
    Color border = resolvedBox.borderColor.value_or(m_borderColor);
    float borderW = resolvedBox.borderWidth.value_or(m_borderWidth);
    float radius = resolvedBox.cornerRadius.value_or(m_cornerRadius);

    if (resolvedBox.shadow.has_value()) {
        backend.drawShadow(m_bounds, radius, resolvedBox.shadow->color, resolvedBox.shadow->blur, resolvedBox.shadow->offset);
    } else if (m_hasShadow && m_shadowColor.a > 0) {
        backend.drawShadow(m_bounds, radius, m_shadowColor, m_shadowBlur, m_shadowOffset);
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
