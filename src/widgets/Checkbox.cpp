#include "PerfUI/Checkbox.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/Theme.h"
#include <algorithm>

namespace PerfUI {

Checkbox::Checkbox(std::string label, bool checked, std::string name)
    : UIElement(std::move(name))
    , m_checked(checked)
    , m_label(std::move(label))
    , m_checkAnim(checked ? 1.0f : 0.0f, 16.0f)
{
    setFocusable(true);
}

Checkbox& Checkbox::checked(bool chk) {
    if (m_checked != chk) {
        m_checked = chk;
        m_checkAnim.setTarget(chk ? 1.0f : 0.0f);
        markStyleDirty();
        markLayoutDirty();
    }
    return *this;
}

void Checkbox::update(float deltaTime) {
    UIElement::update(deltaTime);

    bool isHovered = (currentState() == WidgetState::Hovered || currentState() == WidgetState::Pressed);
    m_hoverAnim.setTarget(isHovered ? 1.0f : 0.0f);

    m_hoverAnim.update(deltaTime);
    m_checkAnim.update(deltaTime);

    if (!m_checked && m_checkAnim.value() < 0.05f) {
        m_checkAnim.snapTo(0.0f);
    }
}

Checkbox& Checkbox::label(std::string_view text) {
    if (m_label != text) {
        m_label = text;
        markLayoutDirty();
    }
    return *this;
}

void Checkbox::measure(Dimensions availableSize) {
    (void)availableSize;

    Dimensions textDim{ 0.0f, 0.0f };
    if (m_context && m_context->renderBackend() && !m_label.empty()) {
        textDim = m_context->renderBackend()->measureText(m_label, m_labelStyle);
    } else if (!m_label.empty()) {
        textDim.width = static_cast<float>(m_label.length()) * (m_labelStyle.fontSize * 0.58f);
        textDim.height = m_labelStyle.fontSize * 1.25f;
    }

    float gap = m_label.empty() ? 0.0f : 8.0f;
    float totalW = m_boxSize + gap + textDim.width;
    float totalH = (std::max)(m_boxSize, textDim.height);

    const auto& pad = layout().padding();
    totalW += pad.horizontal();
    totalH += pad.vertical();

    if (layout().width().mode == SizeMode::Fixed) {
        totalW = layout().width().value;
    }
    if (layout().height().mode == SizeMode::Fixed) {
        totalH = layout().height().value;
    }

    totalW = (std::clamp)(totalW, layout().minWidth(), layout().maxWidth());
    totalH = (std::clamp)(totalH, layout().minHeight(), layout().maxHeight());

    m_desiredSize = Dimensions{ totalW, totalH };
}

void Checkbox::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    float effAlpha = effectiveOpacity();
    if (effAlpha <= 0.001f) return;

    auto applyAlpha = [effAlpha](Color c) -> Color {
        return Color(c.r, c.g, c.b, static_cast<uint8_t>(std::clamp(static_cast<float>(c.a) * effAlpha, 0.0f, 255.0f)));
    };

    const auto& theme = Theme::Current();
    const auto& pad = layout().padding();
    float boxX = m_bounds.x + pad.left;
    float boxY = m_bounds.y + pad.top + (std::max)(0.0f, (m_bounds.height - pad.vertical() - m_boxSize) * 0.5f);
    Rect boxRect{ boxX, boxY, m_boxSize, m_boxSize };

    float hoverT = m_hoverAnim.value();
    float checkT = m_checkAnim.value();

    Color normalBorder = m_checked ? m_checkColor : m_boxBorder;
    Color border = Color::Lerp(normalBorder, theme.colors.borderFocus, hoverT);

    Color normalBg = m_boxBg;
    Color hoverBg = theme.colors.surfaceActive;
    Color bg = Color::Lerp(normalBg, hoverBg, hoverT);
    if (m_pressed) {
        bg = Color::BackgroundActive();
    }

    if (m_computedStyle.backgroundColor.has_value()) {
        bg = *m_computedStyle.backgroundColor;
    }
    if (m_computedStyle.borderColor.has_value()) {
        border = *m_computedStyle.borderColor;
    }
    float radius = m_computedStyle.cornerRadius.value_or(m_cornerRadius);

    if (hoverT > 0.01f) {
        Color glow = theme.colors.focusGlow;
        glow.a = static_cast<uint8_t>(static_cast<float>(glow.a) * hoverT * effAlpha);
        backend.drawShadow(boxRect, radius, glow, 6.0f * hoverT, { 0.0f, 1.0f * hoverT });
    }

    backend.drawRoundedRect(boxRect, applyAlpha(bg), radius, applyAlpha(border), 1.0f);

    // Draw check indicator with scale & fade animation
    if ((m_checked || checkT > 0.05f) && checkT > 0.02f) {
        float innerPadding = 3.0f + (1.0f - checkT) * 3.0f;
        Rect innerRect{
            boxRect.x + innerPadding,
            boxRect.y + innerPadding,
            boxRect.width - innerPadding * 2.0f,
            boxRect.height - innerPadding * 2.0f
        };
        Color checkCol = m_checkColor;
        checkCol.a = static_cast<uint8_t>(static_cast<float>(checkCol.a) * checkT * effAlpha);
        backend.drawRoundedRect(innerRect, checkCol, radius * 0.5f);
    }

    // Draw label
    if (!m_label.empty()) {
        float textX = boxRect.x + boxRect.width + 8.0f;
        Dimensions textDim = backend.measureText(m_label, m_labelStyle);
        float textY = m_bounds.y + pad.top + (std::max)(0.0f, (m_bounds.height - pad.vertical() - textDim.height) * 0.5f);

        TextStyle style = m_labelStyle;
        if (m_computedStyle.color.has_value()) {
            style.color = applyAlpha(*m_computedStyle.color);
        } else {
            style.color = applyAlpha(Color::Lerp(m_labelStyle.color, Color::White(), hoverT));
        }
        if (m_computedStyle.fontSize.has_value()) style.fontSize = *m_computedStyle.fontSize;
        if (m_computedStyle.bold.has_value()) style.bold = *m_computedStyle.bold;
        if (m_computedStyle.italic.has_value()) style.italic = *m_computedStyle.italic;

        backend.drawText(m_label, Point{ textX, textY }, style);
    }

    UIElement::render(backend);
}

bool Checkbox::onPointerDown(const Point& localPoint) {
    UIElement::onPointerDown(localPoint);
    return true;
}

bool Checkbox::onPointerUp(const Point& localPoint) {
    bool wasPressed = m_pressed;
    UIElement::onPointerUp(localPoint);

    if (wasPressed && m_bounds.contains(Point{ m_bounds.x + localPoint.x, m_bounds.y + localPoint.y })) {
        checked(!m_checked);
        if (m_onToggle) {
            m_onToggle(m_checked);
        }
    }
    return true;
}

bool Checkbox::onAction(NavDirection dir) {
    (void)dir;
    checked(!m_checked);
    if (m_onToggle) {
        m_onToggle(m_checked);
    }
    return true;
}

} // namespace PerfUI
