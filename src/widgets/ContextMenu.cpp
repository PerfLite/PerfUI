#include "PerfUI/ContextMenu.h"
#include <algorithm>

namespace PerfUI {

ContextMenu::ContextMenu(Point position, std::vector<ContextMenuItem> items) {
    open(position, std::move(items));
}

void ContextMenu::open(Point position, std::vector<ContextMenuItem> items) {
    m_position = position;
    m_items = std::move(items);
    m_isOpen = true;
    m_hoveredIndex = -1;
    m_animAlpha = 0.0f;
}

void ContextMenu::close() {
    m_isOpen = false;
    m_items.clear();
    m_hoveredIndex = -1;
    m_animAlpha = 0.0f;
}

void ContextMenu::update(float deltaTime) {
    if (!m_isOpen) return;
    m_animAlpha = (std::min)(1.0f, m_animAlpha + deltaTime * 14.0f);
}

void ContextMenu::render(UIRenderBackend& backend, Dimensions viewportSize) {
    if (!m_isOpen || m_items.empty()) return;

    constexpr float itemH = 28.0f;
    constexpr float sepH = 8.0f;
    constexpr float padH = 14.0f;
    constexpr float padV = 6.0f;

    TextStyle measureStyle;
    measureStyle.fontSize = 12.5f;

    float maxTextW = 130.0f;
    for (const auto& it : m_items) {
        if (!it.isSeparator) {
            Dimensions d = backend.measureText(it.text, measureStyle);
            maxTextW = (std::max)(maxTextW, d.width);
        }
    }

    float totalW = maxTextW + padH * 2.0f + 12.0f;
    float totalH = padV * 2.0f;
    for (const auto& it : m_items) {
        totalH += it.isSeparator ? sepH : itemH;
    }

    // Viewport clamping
    float posX = m_position.x;
    float posY = m_position.y;

    if (posX + totalW > viewportSize.width - 8.0f) {
        posX = (std::max)(8.0f, m_position.x - totalW);
    }
    if (posY + totalH > viewportSize.height - 8.0f) {
        posY = (std::max)(8.0f, m_position.y - totalH);
    }

    m_bounds = Rect{ posX, posY, totalW, totalH };

    float alpha = m_animAlpha;
    if (alpha <= 0.01f) return;

    // Soft drop shadow
    Color shadowColor = Color(0, 0, 0, static_cast<uint8_t>(220.0f * alpha));
    backend.drawShadow(m_bounds, 6.0f, shadowColor, 16.0f, { 0.0f, 6.0f });

    // Panel background & gold border
    Color bg = Color(16, 21, 30, static_cast<uint8_t>(250.0f * alpha));
    Color border = Color(212, 175, 55, static_cast<uint8_t>(210.0f * alpha));
    backend.drawRoundedRect(m_bounds, bg, 6.0f, border, 1.0f);

    float curY = m_bounds.y + padV;

    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& it = m_items[i];
        if (it.isSeparator) {
            float lineY = curY + sepH * 0.5f;
            Color sepColor = Color(55, 68, 88, static_cast<uint8_t>(180.0f * alpha));
            backend.drawRect(Rect{ m_bounds.x + 8.0f, lineY, totalW - 16.0f, 1.0f }, sepColor);
            curY += sepH;
        } else {
            Rect itemRect{ m_bounds.x + 4.0f, curY, totalW - 8.0f, itemH };
            bool hovered = (static_cast<int>(i) == m_hoveredIndex) && !it.disabled;

            if (hovered) {
                Color hBg = Color(38, 52, 72, static_cast<uint8_t>(240.0f * alpha));
                backend.drawRoundedRect(itemRect, hBg, 4.0f);

                // Subtle gold accent pill on left edge
                Color pillCol = Color(212, 175, 55, static_cast<uint8_t>(255.0f * alpha));
                backend.drawRoundedRect(Rect{ itemRect.x + 2.0f, itemRect.y + 5.0f, 2.5f, itemRect.height - 10.0f }, pillCol, 1.0f);
            }

            TextStyle style;
            style.fontSize = 12.5f;
            if (it.disabled) {
                style.color = Color(100, 110, 125, static_cast<uint8_t>(160.0f * alpha));
            } else if (hovered) {
                style.color = Color(255, 235, 170, static_cast<uint8_t>(255.0f * alpha));
            } else {
                style.color = Color(225, 230, 240, static_cast<uint8_t>(255.0f * alpha));
            }

            Dimensions td = backend.measureText(it.text, style);
            float ty = itemRect.y + (std::max)(0.0f, (itemRect.height - td.height) * 0.5f);
            backend.drawText(it.text, Point{ itemRect.x + 10.0f, ty }, style);

            curY += itemH;
        }
    }
}

void ContextMenu::onMouseMove(const Point& screenPos) {
    if (!m_isOpen) return;

    if (!m_bounds.contains(screenPos)) {
        m_hoveredIndex = -1;
        return;
    }

    constexpr float itemH = 28.0f;
    constexpr float sepH = 8.0f;
    constexpr float padV = 6.0f;

    float curY = m_bounds.y + padV;
    m_hoveredIndex = -1;

    for (size_t i = 0; i < m_items.size(); ++i) {
        float h = m_items[i].isSeparator ? sepH : itemH;
        Rect r{ m_bounds.x, curY, m_bounds.width, h };
        if (r.contains(screenPos) && !m_items[i].isSeparator && !m_items[i].disabled) {
            m_hoveredIndex = static_cast<int>(i);
            break;
        }
        curY += h;
    }
}

bool ContextMenu::onPointerDown(const Point& screenPos) {
    if (!m_isOpen) return false;

    if (!m_bounds.contains(screenPos)) {
        close();
        return true; // Click outside swallowed, menu closed
    }

    constexpr float itemH = 28.0f;
    constexpr float sepH = 8.0f;
    constexpr float padV = 6.0f;

    float curY = m_bounds.y + padV;

    for (size_t i = 0; i < m_items.size(); ++i) {
        float h = m_items[i].isSeparator ? sepH : itemH;
        Rect r{ m_bounds.x, curY, m_bounds.width, h };
        if (r.contains(screenPos)) {
            if (!m_items[i].isSeparator && !m_items[i].disabled) {
                auto action = m_items[i].action;
                close();
                if (action) {
                    action();
                }
            }
            return true;
        }
        curY += h;
    }

    return true;
}

} // namespace PerfUI
