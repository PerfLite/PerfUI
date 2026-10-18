#include "PerfUI/QuestTracker.h"
#include "PerfUI/Theme.h"
#include "PerfUI/UIRenderBackend.h"
#include <algorithm>

namespace PerfUI {

QuestTracker::QuestTracker(std::string name)
    : UIElement(std::move(name)) {
    m_layout.width(280.0f);
}

QuestTracker& QuestTracker::setQuestTitle(std::string title) {
    m_title = std::move(title);
    return *this;
}

QuestTracker& QuestTracker::setQuestCategory(std::string category) {
    m_category = std::move(category);
    return *this;
}

QuestTracker& QuestTracker::addObjective(
    std::string text,
    bool completed,
    int current,
    int max,
    bool optional
) {
    m_objectives.push_back({
        std::move(text),
        completed,
        optional,
        current,
        max,
        {}
    });
    return *this;
}

QuestTracker& QuestTracker::clearObjectives() {
    m_objectives.clear();
    m_hoveredObjective = -1;
    return *this;
}

void QuestTracker::setObjectiveCompleted(size_t index, bool completed) {
    if (index < m_objectives.size()) {
        m_objectives[index].completed = completed;
    }
}

void QuestTracker::setObjectiveProgress(size_t index, int current, int max) {
    if (index < m_objectives.size()) {
        m_objectives[index].currentProgress = current;
        m_objectives[index].maxProgress = max;
        if (max > 0 && current >= max) {
            m_objectives[index].completed = true;
        }
    }
}

bool QuestTracker::isAllCompleted() const {
    if (m_objectives.empty()) return false;
    for (const auto& obj : m_objectives) {
        if (!obj.optional && !obj.completed) {
            return false;
        }
    }
    return true;
}

void QuestTracker::measure(Dimensions availableSize) {
    (void)availableSize;
    float h = 16.0f; // Padding top & bottom
    if (!m_title.empty()) {
        h += 24.0f;
    }
    if (!m_category.empty()) {
        h += 14.0f;
    }
    if (!m_objectives.empty()) {
        h += 8.0f; // Gap after header
        h += static_cast<float>(m_objectives.size()) * 22.0f;
    }

    float finalW = (m_layout.width().mode == SizeMode::Fixed && m_layout.width().value > 0.0f)
        ? m_layout.width().value : 280.0f;
    float finalH = (m_layout.height().mode == SizeMode::Fixed && m_layout.height().value > 0.0f)
        ? m_layout.height().value : h;

    m_desiredSize = { finalW, finalH };
}

void QuestTracker::arrange(const Rect& finalRect) {
    UIElement::arrange(finalRect);

    float curY = finalRect.y + 10.0f;
    if (!m_title.empty()) curY += 24.0f;
    if (!m_category.empty()) curY += 14.0f;
    if (!m_objectives.empty()) curY += 8.0f;

    for (auto& obj : m_objectives) {
        obj.bounds = Rect{ finalRect.x + 8.0f, curY, finalRect.width - 16.0f, 20.0f };
        curY += 22.0f;
    }
}

void QuestTracker::render(UIRenderBackend& backend) {
    if (!m_visible || m_bounds.width <= 0.0f || m_bounds.height <= 0.0f) return;

    const auto& theme = Theme::Current();
    float radius = theme.metrics.radiusSmall > 0.0f ? theme.metrics.radiusSmall : 4.0f;

    // Background container
    Color bg = (m_style.box.normal && m_style.box.normal->backgroundColor.has_value())
        ? m_style.box.normal->backgroundColor.value()
        : Color(12, 14, 20, 190);
    backend.drawRoundedRect(m_bounds, bg, radius, theme.colors.borderSubtle, 1.0f);

    float curY = m_bounds.y + 10.0f;

    // Quest Title & Skyrim Marker Diamond
    if (!m_title.empty()) {
        float cx = m_bounds.x + 20.0f;
        float cy = curY + 8.0f;
        float d = 4.5f;

        // Draw diamond icon
        Color markerCol = isAllCompleted() ? theme.colors.textSuccess : theme.colors.borderFocus;
        backend.drawLine(Point{ cx, cy - d }, Point{ cx + d, cy }, markerCol, 1.5f);
        backend.drawLine(Point{ cx + d, cy }, Point{ cx, cy + d }, markerCol, 1.5f);
        backend.drawLine(Point{ cx, cy + d }, Point{ cx - d, cy }, markerCol, 1.5f);
        backend.drawLine(Point{ cx - d, cy }, Point{ cx, cy - d }, markerCol, 1.5f);

        TextStyle titleStyle;
        titleStyle.fontSize = 15.0f;
        titleStyle.color = theme.colors.textPrimary;
        titleStyle.bold = true;
        backend.drawText(m_title, Point{ m_bounds.x + 32.0f, curY }, titleStyle);
        curY += 22.0f;
    }

    // Quest Category
    if (!m_category.empty()) {
        TextStyle catStyle;
        catStyle.fontSize = 11.0f;
        catStyle.color = theme.colors.textAccent;
        backend.drawText(m_category, Point{ m_bounds.x + 32.0f, curY }, catStyle);
        curY += 14.0f;
    }

    if (!m_objectives.empty()) {
        curY += 6.0f;
        // Subtle divider under header
        backend.drawLine(
            Point{ m_bounds.x + 12.0f, curY },
            Point{ m_bounds.x + m_bounds.width - 12.0f, curY },
            theme.colors.borderSubtle,
            1.0f
        );
        curY += 6.0f;
    }

    // Render objectives
    for (size_t i = 0; i < m_objectives.size(); ++i) {
        const auto& obj = m_objectives[i];
        bool isHovered = (static_cast<int>(i) == m_hoveredObjective);

        if (isHovered) {
            backend.drawRoundedRect(obj.bounds, theme.colors.surfaceActive, 2.0f);
        }

        // Objective marker
        float bx = obj.bounds.x + 12.0f;
        float by = obj.bounds.y + 9.0f;

        if (obj.completed) {
            // Checkmark / filled marker
            backend.drawRoundedRect(Rect{ bx - 3.0f, by - 3.0f, 6.0f, 6.0f }, theme.colors.textSuccess, 1.0f);
        } else {
            // Open diamond / bullet
            float bd = 3.5f;
            Color bulletCol = isHovered ? theme.colors.borderFocus : theme.colors.borderSubtle;
            backend.drawLine(Point{ bx, by - bd }, Point{ bx + bd, by }, bulletCol, 1.0f);
            backend.drawLine(Point{ bx + bd, by }, Point{ bx, by + bd }, bulletCol, 1.0f);
            backend.drawLine(Point{ bx, by + bd }, Point{ bx - bd, by }, bulletCol, 1.0f);
            backend.drawLine(Point{ bx - bd, by }, Point{ bx, by - bd }, bulletCol, 1.0f);
        }

        // Text assembly
        std::string displayText = obj.text;
        if (obj.maxProgress > 0) {
            displayText += " (" + std::to_string(obj.currentProgress) + "/" + std::to_string(obj.maxProgress) + ")";
        }
        if (obj.optional) {
            displayText += " (Optional)";
        }

        Color textColor = obj.completed ? theme.colors.textDisabled :
                          isHovered ? theme.colors.textPrimary : theme.colors.textSecondary;

        TextStyle objStyle;
        objStyle.fontSize = 13.0f;
        objStyle.color = textColor;
        backend.drawText(displayText, Point{ obj.bounds.x + 24.0f, obj.bounds.y + 3.0f }, objStyle);
    }
}

bool QuestTracker::onPointerDown(const Point& localPoint) {
    for (size_t i = 0; i < m_objectives.size(); ++i) {
        const auto& obj = m_objectives[i];
        if (localPoint.x >= (obj.bounds.x - m_bounds.x) &&
            localPoint.x <= (obj.bounds.x - m_bounds.x + obj.bounds.width) &&
            localPoint.y >= (obj.bounds.y - m_bounds.y) &&
            localPoint.y <= (obj.bounds.y - m_bounds.y + obj.bounds.height)) {
            if (m_onObjectiveClicked) {
                m_onObjectiveClicked(i);
            }
            return true;
        }
    }
    return false;
}

void QuestTracker::onPointerMove(const Point& localPoint) {
    m_hoveredObjective = -1;
    for (size_t i = 0; i < m_objectives.size(); ++i) {
        const auto& obj = m_objectives[i];
        if (localPoint.x >= (obj.bounds.x - m_bounds.x) &&
            localPoint.x <= (obj.bounds.x - m_bounds.x + obj.bounds.width) &&
            localPoint.y >= (obj.bounds.y - m_bounds.y) &&
            localPoint.y <= (obj.bounds.y - m_bounds.y + obj.bounds.height)) {
            m_hoveredObjective = static_cast<int>(i);
            break;
        }
    }
}

void QuestTracker::onPointerLeave() {
    m_hoveredObjective = -1;
}

} // namespace PerfUI
