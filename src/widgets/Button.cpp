#include "PerfUI/Button.h"
#include "PerfUI/UIContext.h"
#include <algorithm>

namespace PerfUI {

Button::Button(std::string label, std::string name)
    : UIElement(std::move(name))
    , m_label(std::move(label))
{
    setFocusable(true);
}

Button& Button::label(std::string_view text) {
    if (m_label != text) {
        m_label = text;
        markLayoutDirty();
    }
    return *this;
}

void Button::measure(Dimensions availableSize) {
    (void)availableSize;

    Dimensions textDim{ 0.0f, 0.0f };
    if (m_context && m_context->renderBackend() && !m_label.empty()) {
        textDim = m_context->renderBackend()->measureText(m_label, m_textStyle);
    } else if (!m_label.empty()) {
        textDim.width = static_cast<float>(m_label.length()) * (m_textStyle.fontSize * 0.58f);
        textDim.height = m_textStyle.fontSize * 1.25f;
    }

    // Default button padding: 16px horizontal, 8px vertical
    const auto& pad = layout().padding();
    float padH = pad.horizontal() > 0.0f ? pad.horizontal() : 32.0f;
    float padV = pad.vertical() > 0.0f ? pad.vertical() : 16.0f;

    float w = textDim.width + padH;
    float h = textDim.height + padV;

    if (layout().width().mode == SizeMode::Fixed) {
        w = layout().width().value;
    }
    if (layout().height().mode == SizeMode::Fixed) {
        h = layout().height().value;
    }

    w = (std::clamp)(w, layout().minWidth(), layout().maxWidth());
    h = (std::clamp)(h, layout().minHeight(), layout().maxHeight());

    m_desiredSize = Dimensions{ w, h };
}

void Button::update(float deltaTime) {
    UIElement::update(deltaTime);

    bool isTargetHovered = (currentState() == WidgetState::Hovered || currentState() == WidgetState::Pressed);
    m_hoverAnim.setTarget(isTargetHovered ? 1.0f : 0.0f);
    m_hoverAnim.update(deltaTime);
}

void Button::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    float t = m_hoverAnim.value();
    Color bg = Color::Lerp(m_normalBg, m_hoverBg, t);
    Color border = Color::Lerp(m_normalBorder, m_hoverBorder, t);
    Color textCol = Color::Lerp(m_normalTextColor, m_hoverTextColor, t);

    WidgetState state = currentState();
    if (state == WidgetState::Pressed) {
        bg = m_pressBg;
        border = m_pressBorder;
        textCol = m_pressTextColor;
    } else if (state == WidgetState::Focused) {
        border = m_focusBorder;
    }

    // Smooth hover glow based on animation
    if (t > 0.01f) {
        Color glow = Color::FocusGlow();
        glow.a = static_cast<uint8_t>(static_cast<float>(glow.a) * t);
        backend.drawShadow(m_bounds, m_cornerRadius, glow, 8.0f * t, { 0.0f, 2.0f * t });
    }

    backend.drawRoundedRect(m_bounds, bg, m_cornerRadius, border, m_borderWidth);

    if (!m_label.empty()) {
        TextStyle renderStyle = m_textStyle;
        renderStyle.color = textCol;

        Dimensions textDim = backend.measureText(m_label, renderStyle);
        float textX = m_bounds.x + (std::max)(0.0f, (m_bounds.width - textDim.width) * 0.5f);
        float textY = m_bounds.y + (std::max)(0.0f, (m_bounds.height - textDim.height) * 0.5f);

        backend.drawText(m_label, Point{ textX, textY }, renderStyle);
    }

    UIElement::render(backend);
}

bool Button::onPointerDown(const Point& localPoint) {
    UIElement::onPointerDown(localPoint);
    return true;
}

bool Button::onPointerUp(const Point& localPoint) {
    bool wasPressed = m_pressed;
    UIElement::onPointerUp(localPoint);

    bool inside = (localPoint.x >= 0.0f && localPoint.x <= m_bounds.width &&
                   localPoint.y >= 0.0f && localPoint.y <= m_bounds.height);

    if (wasPressed || inside) {
        if (m_onClick) {
            m_onClick();
        }
    }
    return true;
}

bool Button::onAction(NavDirection dir) {
    (void)dir;
    if (m_onClick) {
        m_onClick();
        return true;
    }
    return false;
}

} // namespace PerfUI
