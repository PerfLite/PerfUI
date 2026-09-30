#include "PerfUI/Panel.h"

namespace PerfUI {

Panel::Panel(std::string name)
    : UIElement(std::move(name))
{
}

void Panel::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    if (m_hasShadow && m_shadowColor.a > 0) {
        backend.drawShadow(m_bounds, m_cornerRadius, m_shadowColor, m_shadowBlur, m_shadowOffset);
    }

    if (m_backgroundColor.a > 0 || (m_borderColor.a > 0 && m_borderWidth > 0.0f)) {
        backend.drawRoundedRect(
            m_bounds,
            m_backgroundColor,
            m_cornerRadius,
            m_borderColor,
            m_borderWidth
        );
    }

    UIElement::render(backend);
}

} // namespace PerfUI
