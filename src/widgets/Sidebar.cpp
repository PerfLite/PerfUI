#include "PerfUI/Sidebar.h"
#include "PerfUI/Theme.h"
#include "PerfUI/UIRenderBackend.h"
#include <algorithm>

namespace PerfUI {

static const std::string s_emptySidebarString = "";

Sidebar::Sidebar(std::string name)
    : Panel(std::move(name)) {
    m_layout.width(m_expandedWidth);
    m_widthAnim.snapTo(m_expandedWidth);
}

Sidebar& Sidebar::addItem(std::string id, std::string label, std::string icon, std::string badge) {
    m_items.push_back({ std::move(id), std::move(label), std::move(icon), std::move(badge), {} });
    return *this;
}

Sidebar& Sidebar::clearItems() {
    m_items.clear();
    m_selectedIndex = 0;
    m_hoveredIndex = -1;
    return *this;
}

const std::string& Sidebar::selectedId() const {
    if (m_selectedIndex < m_items.size()) {
        return m_items[m_selectedIndex].id;
    }
    return s_emptySidebarString;
}

void Sidebar::selectItem(const std::string& id) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id == id) {
            selectIndex(i);
            return;
        }
    }
}

void Sidebar::selectIndex(size_t index) {
    if (m_items.empty()) return;
    if (index >= m_items.size()) index = m_items.size() - 1;

    if (m_selectedIndex != index) {
        m_selectedIndex = index;
        if (m_onItemSelected) {
            m_onItemSelected(m_items[m_selectedIndex].id);
        }
    }
}

void Sidebar::setCollapsed(bool collapsed) {
    if (m_collapsed != collapsed) {
        m_collapsed = collapsed;
        m_widthAnim.setTarget(m_collapsed ? m_collapsedWidth : m_expandedWidth);
        if (m_onCollapseToggled) {
            m_onCollapseToggled(m_collapsed);
        }
    }
}

void Sidebar::update(float deltaTime) {
    Panel::update(deltaTime);
    m_widthAnim.update(deltaTime);
}

void Sidebar::measure(Dimensions availableSize) {
    float curW = m_widthAnim.value();
    float totalH = 50.0f + static_cast<float>(m_items.size()) * m_itemHeight + 20.0f;
    float finalH = availableSize.height > 0.0f ? availableSize.height : totalH;

    m_desiredSize = { curW, finalH };
}

void Sidebar::arrange(const Rect& finalRect) {
    Rect adjustedRect = finalRect;
    adjustedRect.width = m_widthAnim.value();
    Panel::arrange(adjustedRect);

    // Toggle button in header
    m_toggleBtnBounds = Rect{ adjustedRect.x + adjustedRect.width - 34.0f, adjustedRect.y + 12.0f, 24.0f, 24.0f };

    // Navigation items
    float curY = adjustedRect.y + 48.0f;
    for (auto& item : m_items) {
        item.bounds = Rect{ adjustedRect.x + 4.0f, curY, adjustedRect.width - 8.0f, m_itemHeight };
        curY += m_itemHeight + 2.0f;
    }
}

void Sidebar::render(UIRenderBackend& backend) {
    if (!m_visible || m_bounds.width <= 0.0f || m_bounds.height <= 0.0f) return;

    const auto& theme = Theme::Current();

    // Background container
    Color bg = (m_style.box.normal && m_style.box.normal->backgroundColor.has_value())
        ? m_style.box.normal->backgroundColor.value()
        : theme.colors.surfaceBase;
    backend.drawRoundedRect(m_bounds, bg, 0.0f, theme.colors.borderSubtle, 1.0f);

    bool isNarrow = m_bounds.width < (m_expandedWidth * 0.7f);

    // Header title (if expanded enough)
    if (!isNarrow && !m_title.empty()) {
        TextStyle titleStyle;
        titleStyle.fontSize = 16.0f;
        titleStyle.color = theme.colors.textPrimary;
        titleStyle.bold = true;
        backend.drawText(m_title, Point{ m_bounds.x + 16.0f, m_bounds.y + 16.0f }, titleStyle);
    }

    // Toggle button
    Color toggleCol = m_hoveredToggle ? theme.colors.borderFocus : theme.colors.textSecondary;
    std::string toggleIcon = m_collapsed ? ">>" : "<<";
    TextStyle toggleStyle;
    toggleStyle.fontSize = 13.0f;
    toggleStyle.color = toggleCol;
    backend.drawText(toggleIcon, Point{ m_toggleBtnBounds.x + 4.0f, m_toggleBtnBounds.y + 4.0f }, toggleStyle);

    // Divider under header
    backend.drawLine(
        Point{ m_bounds.x + 6.0f, m_bounds.y + 42.0f },
        Point{ m_bounds.x + m_bounds.width - 6.0f, m_bounds.y + 42.0f },
        theme.colors.borderSubtle,
        1.0f
    );

    // Render items
    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        bool isSelected = (i == m_selectedIndex);
        bool isHovered = (static_cast<int>(i) == m_hoveredIndex);

        if (isSelected) {
            backend.drawRoundedRect(item.bounds, theme.colors.surfaceElevated, 4.0f);
            // Left active bar
            Rect barRect{ item.bounds.x + 1.0f, item.bounds.y + 4.0f, 3.0f, item.bounds.height - 8.0f };
            backend.drawRoundedRect(barRect, theme.colors.borderFocus, 1.5f);
        } else if (isHovered) {
            backend.drawRoundedRect(item.bounds, theme.colors.surfaceActive, 4.0f);
        }

        // Draw icon or fallback letter
        float iconX = item.bounds.x + 12.0f;
        float iconY = item.bounds.y + (item.bounds.height - 18.0f) * 0.5f;

        // Initial letter
        std::string initial = item.label.empty() ? "?" : item.label.substr(0, 1);
        Color initialCol = isSelected ? theme.colors.borderFocus : theme.colors.textSecondary;
        TextStyle iconStyle;
        iconStyle.fontSize = 14.0f;
        iconStyle.color = initialCol;
        iconStyle.bold = true;
        backend.drawText(initial, Point{ iconX + 4.0f, iconY }, iconStyle);

        // Label if expanded
        if (!isNarrow) {
            Color textColor = isSelected ? theme.colors.textPrimary :
                              isHovered ? theme.colors.textPrimary : theme.colors.textSecondary;
            TextStyle labelStyle;
            labelStyle.fontSize = 14.0f;
            labelStyle.color = textColor;
            labelStyle.bold = isSelected;
            backend.drawText(item.label, Point{ item.bounds.x + 40.0f, item.bounds.y + 11.0f }, labelStyle);

            // Badge
            if (!item.badge.empty()) {
                float bw = static_cast<float>(item.badge.length()) * 7.0f + 10.0f;
                Rect badgeRect{ item.bounds.x + item.bounds.width - bw - 8.0f, item.bounds.y + 10.0f, bw, 18.0f };
                backend.drawRoundedRect(badgeRect, theme.colors.surfaceActive, 9.0f);

                TextStyle badgeStyle;
                badgeStyle.fontSize = 11.0f;
                badgeStyle.color = theme.colors.textSecondary;
                backend.drawText(item.badge, Point{ badgeRect.x + 5.0f, badgeRect.y + 2.0f }, badgeStyle);
            }
        }
    }
}

bool Sidebar::onPointerDown(const Point& localPoint) {
    Point globalPoint{ m_bounds.x + localPoint.x, m_bounds.y + localPoint.y };

    // Check toggle button
    if (globalPoint.x >= m_toggleBtnBounds.x && globalPoint.x <= (m_toggleBtnBounds.x + m_toggleBtnBounds.width) &&
        globalPoint.y >= m_toggleBtnBounds.y && globalPoint.y <= (m_toggleBtnBounds.y + m_toggleBtnBounds.height)) {
        toggleCollapsed();
        return true;
    }

    // Check items
    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        if (globalPoint.x >= item.bounds.x && globalPoint.x <= (item.bounds.x + item.bounds.width) &&
            globalPoint.y >= item.bounds.y && globalPoint.y <= (item.bounds.y + item.bounds.height)) {
            selectIndex(i);
            return true;
        }
    }

    return Panel::onPointerDown(localPoint);
}

void Sidebar::onPointerMove(const Point& localPoint) {
    Point globalPoint{ m_bounds.x + localPoint.x, m_bounds.y + localPoint.y };

    m_hoveredToggle = (globalPoint.x >= m_toggleBtnBounds.x && globalPoint.x <= (m_toggleBtnBounds.x + m_toggleBtnBounds.width) &&
                       globalPoint.y >= m_toggleBtnBounds.y && globalPoint.y <= (m_toggleBtnBounds.y + m_toggleBtnBounds.height));

    m_hoveredIndex = -1;
    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        if (globalPoint.x >= item.bounds.x && globalPoint.x <= (item.bounds.x + item.bounds.width) &&
            globalPoint.y >= item.bounds.y && globalPoint.y <= (item.bounds.y + item.bounds.height)) {
            m_hoveredIndex = static_cast<int>(i);
            break;
        }
    }

    Panel::onPointerMove(localPoint);
}

void Sidebar::onPointerLeave() {
    m_hoveredToggle = false;
    m_hoveredIndex = -1;
    Panel::onPointerLeave();
}

} // namespace PerfUI
