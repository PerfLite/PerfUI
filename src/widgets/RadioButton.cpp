#include "PerfUI/RadioButton.h"
#include "PerfUI/RadioGroup.h"
#include "PerfUI/Theme.h"
#include <algorithm>

namespace PerfUI {

RadioButton::RadioButton(int value, std::string label, std::string name)
    : UIElement(std::move(name))
    , m_value(value)
    , m_label(std::move(label))
{
    setFocusable(true);
}

void RadioButton::setSelected(bool sel) {
    if (m_selected != sel) {
        m_selected = sel;
        if (sel) {
            // If parent is RadioGroup, notify
            if (auto* grp = dynamic_cast<RadioGroup*>(m_parent)) {
                grp->notifySelection(this);
            }
            if (m_onSelect) {
                m_onSelect(m_value);
            }
        }
    }
}

void RadioButton::measure(Dimensions availableSize) {
    (void)availableSize;
    float circleDiameter = 18.0f;
    float spacing = 8.0f;
    float textWidth = static_cast<float>(m_label.length()) * 8.5f;

    float w = circleDiameter + spacing + textWidth;
    float h = (std::max)(circleDiameter, 20.0f);

    if (layout().width().mode == SizeMode::Fixed) {
        w = layout().width().value;
    }
    if (layout().height().mode == SizeMode::Fixed) {
        h = layout().height().value;
    }

    m_desiredSize = Dimensions{ w, h };
}

void RadioButton::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    WidgetState state = currentState();
    if (m_selected) {
        state = WidgetState::Selected;
    }

    const auto& themeRadio = Theme::Current().components.radio;
    BoxStyle themeBox = themeRadio.resolveBox(state);
    BoxStyle resolvedBox = resolveBoxStyle(state, themeBox);

    float circleRadius = 9.0f;
    Point circleCenter{ m_bounds.x + circleRadius, m_bounds.y + m_bounds.height * 0.5f };

    Color outerBorder = isFocused() ? Theme::Current().colors.borderFocus
        : resolvedBox.borderColor.value_or(m_selected ? Theme::Current().colors.borderFocus : Theme::Current().colors.borderStrong);
    Color outerBg = resolvedBox.backgroundColor.value_or(Color(26, 33, 44, 230));

    // Outer circle
    backend.drawCircle(circleCenter, circleRadius, outerBg, outerBorder, 1.5f);

    // Inner selected dot
    if (m_selected) {
        Color innerColor = Theme::Current().colors.borderFocus;
        backend.drawCircle(circleCenter, 4.5f, innerColor);
    }

    // Label text
    if (!m_label.empty()) {
        TextStyle textStyle;
        textStyle.color = m_selected ? Theme::Current().colors.textPrimary : Theme::Current().colors.textSecondary;
        textStyle.fontSize = 14.0f;

        Point textPos{ m_bounds.x + circleRadius * 2.0f + 8.0f, m_bounds.y + (m_bounds.height - 14.0f * 1.25f) * 0.5f };
        backend.drawText(m_label, textPos, textStyle);
    }

    UIElement::render(backend);
}

bool RadioButton::onPointerDown(const Point& localPoint) {
    UIElement::onPointerDown(localPoint);
    setSelected(true);
    return true;
}

bool RadioButton::onAction(NavDirection dir) {
    (void)dir;
    setSelected(true);
    return true;
}

} // namespace PerfUI
