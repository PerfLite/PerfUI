#include "PerfUI/CategoryList.h"
#include "PerfUI/Theme.h"
#include "PerfUI/UIRenderBackend.h"
#include <algorithm>

namespace PerfUI {

static const std::string s_emptyCategoryString = "";

CategoryList::CategoryList(std::string name)
    : UIElement(std::move(name)) {
    m_layout.width(220.0f);
}

CategoryList& CategoryList::addCategory(std::string id, std::string title, std::string icon, std::string badge) {
    m_items.push_back({ std::move(id), std::move(title), std::move(icon), std::move(badge), {} });
    return *this;
}

CategoryList& CategoryList::clearCategories() {
    m_items.clear();
    m_selectedIndex = 0;
    m_hoveredIndex = -1;
    return *this;
}

const std::string& CategoryList::selectedId() const {
    if (m_selectedIndex < m_items.size()) {
        return m_items[m_selectedIndex].id;
    }
    return s_emptyCategoryString;
}

void CategoryList::selectCategory(const std::string& id) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id == id) {
            selectIndex(i);
            return;
        }
    }
}

void CategoryList::selectIndex(size_t index) {
    if (m_items.empty()) return;
    if (index >= m_items.size()) index = m_items.size() - 1;

    if (m_selectedIndex != index) {
        m_selectedIndex = index;
        if (m_onCategoryChanged) {
            m_onCategoryChanged(m_items[m_selectedIndex].id);
        }
    }
}

void CategoryList::measure(Dimensions availableSize) {
    (void)availableSize;
    float totalH = static_cast<float>(m_items.size()) * m_itemHeight;
    float finalW = (m_layout.width().mode == SizeMode::Fixed && m_layout.width().value > 0.0f)
        ? m_layout.width().value : 220.0f;
    float finalH = (m_layout.height().mode == SizeMode::Fixed && m_layout.height().value > 0.0f)
        ? m_layout.height().value : totalH;

    m_desiredSize = { finalW, finalH };
}

void CategoryList::arrange(const Rect& finalRect) {
    UIElement::arrange(finalRect);

    float curY = finalRect.y;
    for (auto& item : m_items) {
        item.bounds = Rect{ finalRect.x, curY, finalRect.width, m_itemHeight };
        curY += m_itemHeight;
    }
}

void CategoryList::render(UIRenderBackend& backend) {
    if (!m_visible || m_bounds.width <= 0.0f || m_bounds.height <= 0.0f) return;

    const auto& theme = Theme::Current();
    float radius = theme.metrics.radiusSmall > 0.0f ? theme.metrics.radiusSmall : 4.0f;

    // Optional background
    if (m_style.box.normal && m_style.box.normal->backgroundColor.has_value() && m_style.box.normal->backgroundColor->a > 0) {
        backend.drawRoundedRect(m_bounds, m_style.box.normal->backgroundColor.value(), radius);
    }

    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        bool isSelected = (i == m_selectedIndex);
        bool isHovered = (static_cast<int>(i) == m_hoveredIndex);

        // Item background
        Rect itemRect = item.bounds;
        if (isSelected) {
            backend.drawRoundedRect(itemRect, theme.colors.surfaceElevated, radius);

            // Left active accent indicator bar
            Rect barRect{ itemRect.x + 2.0f, itemRect.y + 4.0f, 3.5f, itemRect.height - 8.0f };
            backend.drawRoundedRect(barRect, theme.colors.borderFocus, 2.0f);
        } else if (isHovered) {
            backend.drawRoundedRect(itemRect, theme.colors.surfaceActive, radius);
        }

        // Text & Icon positioning
        float textX = itemRect.x + 14.0f;
        if (!item.icon.empty()) {
            Point center{ textX + 6.0f, itemRect.y + itemRect.height * 0.5f };
            backend.drawCircle(center, 4.0f, isSelected ? theme.colors.borderFocus : theme.colors.textSecondary);
            textX += 18.0f;
        }

        // Title text
        Color textColor = isSelected ? theme.colors.textPrimary :
                          isHovered ? theme.colors.textPrimary : theme.colors.textSecondary;
        float textY = itemRect.y + (itemRect.height - 14.0f) * 0.5f;

        TextStyle tstyle;
        tstyle.fontSize = 14.0f;
        tstyle.color = textColor;
        tstyle.bold = isSelected;
        backend.drawText(item.title, Point{ textX, textY }, tstyle);

        // Badge if present
        if (!item.badge.empty()) {
            float badgeW = static_cast<float>(item.badge.length()) * 7.0f + 12.0f;
            float badgeH = 18.0f;
            Rect badgeRect{
                itemRect.x + itemRect.width - badgeW - 10.0f,
                itemRect.y + (itemRect.height - badgeH) * 0.5f,
                badgeW,
                badgeH
            };
            Color badgeBg = isSelected ? theme.colors.borderFocus : theme.colors.surfaceActive;
            Color badgeText = isSelected ? theme.colors.surfaceBase : theme.colors.textSecondary;
            backend.drawRoundedRect(badgeRect, badgeBg, 9.0f);

            TextStyle bstyle;
            bstyle.fontSize = 11.0f;
            bstyle.color = badgeText;
            backend.drawText(item.badge, Point{ badgeRect.x + 6.0f, badgeRect.y + 2.0f }, bstyle);
        }
    }
}

bool CategoryList::onPointerDown(const Point& localPoint) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        if (localPoint.x >= 0.0f && localPoint.x <= m_bounds.width &&
            localPoint.y >= (item.bounds.y - m_bounds.y) &&
            localPoint.y <= (item.bounds.y - m_bounds.y + item.bounds.height)) {
            selectIndex(i);
            return true;
        }
    }
    return false;
}

void CategoryList::onPointerMove(const Point& localPoint) {
    m_hoveredIndex = -1;
    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        if (localPoint.x >= 0.0f && localPoint.x <= m_bounds.width &&
            localPoint.y >= (item.bounds.y - m_bounds.y) &&
            localPoint.y <= (item.bounds.y - m_bounds.y + item.bounds.height)) {
            m_hoveredIndex = static_cast<int>(i);
            break;
        }
    }
}

void CategoryList::onPointerLeave() {
    m_hoveredIndex = -1;
}

} // namespace PerfUI
