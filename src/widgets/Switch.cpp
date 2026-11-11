#include "PerfUI/Switch.h"
#include "PerfUI/Theme.h"
#include <algorithm>

namespace PerfUI {

Switch::Switch(bool checked, std::string name)
    : UIElement(std::move(name))
    , m_checked(checked)
{
    setFocusable(true);
    m_thumbAnim.snapTo(checked ? 1.0f : 0.0f);

    const auto& themeSwitch = Theme::Current().components.switchToggle;
    Color initColor = checked
        ? themeSwitch.box.selected.has_value() && themeSwitch.box.selected->backgroundColor.has_value()
            ? *themeSwitch.box.selected->backgroundColor
            : Theme::Current().colors.borderFocus
        : themeSwitch.box.normal.has_value() && themeSwitch.box.normal->backgroundColor.has_value()
            ? *themeSwitch.box.normal->backgroundColor
            : Color(48, 54, 61, 200);

    m_trackColorAnim.snapTo(initColor);
}

void Switch::setChecked(bool checked) {
    if (m_checked != checked) {
        m_checked = checked;
        markStyleDirty();
        m_thumbAnim.setTarget(checked ? 1.0f : 0.0f);

        const auto& themeSwitch = Theme::Current().components.switchToggle;
        Color targetColor = checked
            ? themeSwitch.box.selected.has_value() && themeSwitch.box.selected->backgroundColor.has_value()
                ? *themeSwitch.box.selected->backgroundColor
                : Theme::Current().colors.borderFocus
            : themeSwitch.box.normal.has_value() && themeSwitch.box.normal->backgroundColor.has_value()
                ? *themeSwitch.box.normal->backgroundColor
                : Color(48, 54, 61, 200);

        m_trackColorAnim.setTarget(targetColor);

        if (m_onToggle) {
            m_onToggle(m_checked);
        }
    }
}

void Switch::update(float deltaTime) {
    UIElement::update(deltaTime);
    m_thumbAnim.update(deltaTime);
    m_trackColorAnim.update(deltaTime);
}

void Switch::measure(Dimensions availableSize) {
    (void)availableSize;
    float w = 44.0f;
    float h = 24.0f;

    if (layout().width().mode == SizeMode::Fixed) {
        w = layout().width().value;
    }
    if (layout().height().mode == SizeMode::Fixed) {
        h = layout().height().value;
    }

    m_desiredSize = Dimensions{ w, h };
}

void Switch::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    WidgetState state = currentState();
    if (m_checked) {
        state = WidgetState::Selected;
    }

    const auto& themeSwitch = Theme::Current().components.switchToggle;
    BoxStyle themeBox = themeSwitch.resolveBox(state);
    BoxStyle resolvedBox = resolveBoxStyle(state, themeBox);

    float effAlpha = effectiveOpacity();
    if (effAlpha <= 0.001f) return;

    auto applyAlpha = [effAlpha](Color c) -> Color {
        return Color(c.r, c.g, c.b, static_cast<uint8_t>(std::clamp(static_cast<float>(c.a) * effAlpha, 0.0f, 255.0f)));
    };

    float radius = m_bounds.height * 0.5f;
    Color trackColor = resolvedBox.backgroundColor.value_or(m_trackColorAnim.value());
    Color border = isFocused() ? Theme::Current().colors.borderFocus : resolvedBox.borderColor.value_or(Color::Transparent());
    float borderWidth = isFocused() ? 1.5f : resolvedBox.borderWidth.value_or(0.0f);

    // Draw track
    backend.drawRoundedRect(m_bounds, applyAlpha(trackColor), radius, applyAlpha(border), borderWidth);

    // Draw thumb
    float thumbDiameter = m_bounds.height - 4.0f;
    float thumbRadius = thumbDiameter * 0.5f;
    float minX = m_bounds.x + 2.0f;
    float maxX = m_bounds.x + m_bounds.width - thumbDiameter - 2.0f;
    float currentX = minX + (maxX - minX) * m_thumbAnim.value();
    float currentY = m_bounds.y + 2.0f;

    Rect thumbRect{ currentX, currentY, thumbDiameter, thumbDiameter };
    Point thumbCenter{ currentX + thumbRadius, currentY + thumbRadius };

    // Thumb shadow
    backend.drawShadow(thumbRect, thumbRadius, Color(0, 0, 0, static_cast<uint8_t>(100.0f * effAlpha)), 4.0f, { 0.0f, 1.0f });

    // Thumb fill
    backend.drawCircle(thumbCenter, thumbRadius, applyAlpha(Color::White()), applyAlpha(Color(200, 200, 200, 180)), 0.5f);

    UIElement::render(backend);
}

bool Switch::onPointerDown(const Point& localPoint) {
    UIElement::onPointerDown(localPoint);
    setChecked(!m_checked);
    return true;
}

bool Switch::onAction(NavDirection dir) {
    (void)dir;
    setChecked(!m_checked);
    return true;
}

} // namespace PerfUI
