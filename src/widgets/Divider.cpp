#include "PerfUI/Divider.h"
#include "PerfUI/Theme.h"
#include <algorithm>

namespace PerfUI {

Divider::Divider(LayoutDirection orientation, std::string name)
    : UIElement(std::move(name))
    , m_orientation(orientation)
{
}

Color Divider::color() const {
    if (m_customColor.has_value()) {
        return *m_customColor;
    }
    const auto& themeDiv = Theme::Current().components.divider;
    BoxStyle resolved = resolveBoxStyle(currentState(), themeDiv.resolveBox(currentState()));
    return resolved.backgroundColor.value_or(Theme::Current().colors.borderSubtle);
}

void Divider::measure(Dimensions availableSize) {
    if (m_orientation == LayoutDirection::Horizontal) {
        float w = availableSize.width > 0.0f ? availableSize.width : 100.0f;
        if (layout().width().mode == SizeMode::Fixed) {
            w = layout().width().value;
        }
        m_desiredSize = Dimensions{ w, m_thickness };
    } else {
        float h = availableSize.height > 0.0f ? availableSize.height : 100.0f;
        if (layout().height().mode == SizeMode::Fixed) {
            h = layout().height().value;
        }
        m_desiredSize = Dimensions{ m_thickness, h };
    }
}

void Divider::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    backend.drawRect(m_bounds, color());
    UIElement::render(backend);
}

} // namespace PerfUI
