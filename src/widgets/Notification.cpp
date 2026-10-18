#include "PerfUI/Notification.h"
#include "PerfUI/Theme.h"
#include "PerfUI/UIRenderBackend.h"
#include <algorithm>

namespace PerfUI {

Notification::Notification(std::string name)
    : UIElement(std::move(name)) {
    m_layout.width(320.0f);
    m_alpha.snapTo(1.0f);
}

void Notification::dismiss() {
    if (!m_dismissed) {
        m_dismissed = true;
        m_alpha.setTarget(0.0f);
        if (m_onClose) {
            m_onClose();
        }
    }
}

void Notification::update(float deltaTime) {
    UIElement::update(deltaTime);
    m_alpha.update(deltaTime);

    if (m_dismissed && m_alpha.value() <= 0.01f) {
        m_visible = false;
        return;
    }

    if (!m_dismissed && m_duration > 0.0f) {
        m_elapsed += deltaTime;
        if (m_elapsed >= m_duration) {
            dismiss();
        }
    }
}

void Notification::measure(Dimensions availableSize) {
    (void)availableSize;
    float h = 18.0f; // Padding top & bottom
    if (!m_title.empty()) h += 20.0f;
    if (!m_message.empty()) h += 20.0f;
    if (!m_actionText.empty()) h += 28.0f;
    if (m_duration > 0.0f) h += 4.0f;

    float finalW = (m_layout.width().mode == SizeMode::Fixed && m_layout.width().value > 0.0f)
        ? m_layout.width().value : 320.0f;
    float finalH = (m_layout.height().mode == SizeMode::Fixed && m_layout.height().value > 0.0f)
        ? m_layout.height().value : h;

    m_desiredSize = { finalW, finalH };
}

void Notification::arrange(const Rect& finalRect) {
    UIElement::arrange(finalRect);

    // Close button top-right
    if (m_closable) {
        m_closeBtnBounds = Rect{ finalRect.x + finalRect.width - 24.0f, finalRect.y + 6.0f, 18.0f, 18.0f };
    }

    // Action button bottom-right
    if (!m_actionText.empty()) {
        float actW = static_cast<float>(m_actionText.length()) * 8.0f + 20.0f;
        m_actionBtnBounds = Rect{
            finalRect.x + finalRect.width - actW - 12.0f,
            finalRect.y + finalRect.height - 30.0f,
            actW,
            22.0f
        };
    }
}

void Notification::render(UIRenderBackend& backend) {
    if (!m_visible || m_alpha.value() <= 0.01f || m_bounds.width <= 0.0f || m_bounds.height <= 0.0f) return;

    const auto& theme = Theme::Current();
    float radius = theme.metrics.radiusSmall > 0.0f ? theme.metrics.radiusSmall : 4.0f;

    // Pick accent color by level
    Color accentCol = theme.colors.borderFocus;
    switch (m_level) {
        case NotificationLevel::Info:    accentCol = Color(70, 150, 240, 255); break;
        case NotificationLevel::Success: accentCol = theme.colors.textSuccess; break;
        case NotificationLevel::Warning: accentCol = Color(240, 180, 50, 255); break;
        case NotificationLevel::Error:   accentCol = Color(230, 60, 60, 255); break;
    }

    // Background & left accent stripe
    Color bg = (m_style.box.normal && m_style.box.normal->backgroundColor.has_value())
        ? m_style.box.normal->backgroundColor.value()
        : theme.colors.surfaceElevated;
    float curAlpha = m_alpha.value();
    bg.a = static_cast<uint8_t>(static_cast<float>(bg.a) * curAlpha);
    accentCol.a = static_cast<uint8_t>(static_cast<float>(accentCol.a) * curAlpha);

    backend.drawRoundedRect(m_bounds, bg, radius, theme.colors.borderSubtle, 1.0f);

    // Left accent bar
    Rect barRect{ m_bounds.x + 2.0f, m_bounds.y + 3.0f, 4.0f, m_bounds.height - 6.0f };
    backend.drawRoundedRect(barRect, accentCol, 2.0f);

    float curY = m_bounds.y + 10.0f;
    float textX = m_bounds.x + 16.0f;

    // Title
    if (!m_title.empty()) {
        Color titleCol = theme.colors.textPrimary;
        titleCol.a = static_cast<uint8_t>(static_cast<float>(titleCol.a) * curAlpha);

        TextStyle tstyle;
        tstyle.fontSize = 14.0f;
        tstyle.color = titleCol;
        tstyle.bold = true;
        backend.drawText(m_title, Point{ textX, curY }, tstyle);
        curY += 20.0f;
    }

    // Message
    if (!m_message.empty()) {
        Color msgCol = theme.colors.textSecondary;
        msgCol.a = static_cast<uint8_t>(static_cast<float>(msgCol.a) * curAlpha);

        TextStyle mstyle;
        mstyle.fontSize = 12.5f;
        mstyle.color = msgCol;
        backend.drawText(m_message, Point{ textX, curY }, mstyle);
        curY += 18.0f;
    }

    // Action button
    if (!m_actionText.empty()) {
        Color btnBg = m_hoveredAction ? theme.colors.surfaceActive : theme.colors.surfaceBase;
        backend.drawRoundedRect(m_actionBtnBounds, btnBg, radius - 1.0f, accentCol, 1.0f);

        float actTextX = m_actionBtnBounds.x + 10.0f;
        float actTextY = m_actionBtnBounds.y + 4.0f;

        TextStyle actStyle;
        actStyle.fontSize = 12.0f;
        actStyle.color = theme.colors.textPrimary;
        backend.drawText(m_actionText, Point{ actTextX, actTextY }, actStyle);
    }

    // Close button
    if (m_closable) {
        Color closeCol = m_hoveredClose ? theme.colors.textPrimary : theme.colors.textDisabled;
        TextStyle closeStyle;
        closeStyle.fontSize = 13.0f;
        closeStyle.color = closeCol;
        backend.drawText("x", Point{ m_closeBtnBounds.x + 4.0f, m_closeBtnBounds.y + 2.0f }, closeStyle);
    }

    // Timer progress bar at the bottom
    if (m_duration > 0.0f && !m_dismissed) {
        float progress = std::clamp(1.0f - (m_elapsed / m_duration), 0.0f, 1.0f);
        Rect progressRect{
            m_bounds.x + 6.0f,
            m_bounds.y + m_bounds.height - 3.0f,
            (m_bounds.width - 12.0f) * progress,
            2.0f
        };
        backend.drawRoundedRect(progressRect, accentCol, 1.0f);
    }
}

bool Notification::onPointerDown(const Point& localPoint) {
    Point globalPoint{ m_bounds.x + localPoint.x, m_bounds.y + localPoint.y };

    if (m_closable &&
        globalPoint.x >= m_closeBtnBounds.x && globalPoint.x <= (m_closeBtnBounds.x + m_closeBtnBounds.width) &&
        globalPoint.y >= m_closeBtnBounds.y && globalPoint.y <= (m_closeBtnBounds.y + m_closeBtnBounds.height)) {
        dismiss();
        return true;
    }

    if (!m_actionText.empty() &&
        globalPoint.x >= m_actionBtnBounds.x && globalPoint.x <= (m_actionBtnBounds.x + m_actionBtnBounds.width) &&
        globalPoint.y >= m_actionBtnBounds.y && globalPoint.y <= (m_actionBtnBounds.y + m_actionBtnBounds.height)) {
        if (m_onAction) {
            m_onAction();
        }
        return true;
    }

    return false;
}

void Notification::onPointerMove(const Point& localPoint) {
    Point globalPoint{ m_bounds.x + localPoint.x, m_bounds.y + localPoint.y };

    m_hoveredClose = m_closable &&
        (globalPoint.x >= m_closeBtnBounds.x && globalPoint.x <= (m_closeBtnBounds.x + m_closeBtnBounds.width) &&
         globalPoint.y >= m_closeBtnBounds.y && globalPoint.y <= (m_closeBtnBounds.y + m_closeBtnBounds.height));

    m_hoveredAction = !m_actionText.empty() &&
        (globalPoint.x >= m_actionBtnBounds.x && globalPoint.x <= (m_actionBtnBounds.x + m_actionBtnBounds.width) &&
         globalPoint.y >= m_actionBtnBounds.y && globalPoint.y <= (m_actionBtnBounds.y + m_actionBtnBounds.height));
}

void Notification::onPointerLeave() {
    m_hoveredClose = false;
    m_hoveredAction = false;
}

} // namespace PerfUI
