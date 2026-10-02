#include "PerfUI/LayoutEngine.h"
#include "PerfUI/UIElement.h"
#include <algorithm>
#include <vector>

namespace PerfUI {

void LayoutEngine::Measure(UIElement* element, Dimensions availableSize) {
    if (!element) return;

    const auto& layout = element->layout();
    const auto& pad = layout.padding();

    // Target width constraint
    float targetW = -1.0f;
    if (layout.width().mode == SizeMode::Fixed) {
        targetW = layout.width().value;
    } else if (layout.width().mode == SizeMode::Percent) {
        targetW = availableSize.width * (layout.width().value / 100.0f);
    } else if (layout.width().mode == SizeMode::Flex) {
        targetW = availableSize.width;
    }

    // Target height constraint
    float targetH = -1.0f;
    if (layout.height().mode == SizeMode::Fixed) {
        targetH = layout.height().value;
    } else if (layout.height().mode == SizeMode::Percent) {
        targetH = availableSize.height * (layout.height().value / 100.0f);
    } else if (layout.height().mode == SizeMode::Flex) {
        targetH = availableSize.height;
    }

    // Inner space available for children
    float innerAvailW = (std::max)(0.0f, (targetW >= 0.0f ? targetW : availableSize.width) - pad.horizontal());
    float innerAvailH = (std::max)(0.0f, (targetH >= 0.0f ? targetH : availableSize.height) - pad.vertical());
    Dimensions childAvail{ innerAvailW, innerAvailH };

    float contentW = 0.0f;
    float contentH = 0.0f;
    int visibleCount = 0;

    for (const auto& child : element->children()) {
        if (!child || !child->isVisible()) continue;
        visibleCount++;

        child->measure(childAvail);

        const Dimensions& childDesired = child->desiredSize();
        const Insets& margin = child->layout().margin();
        float childTotalW = childDesired.width + margin.horizontal();
        float childTotalH = childDesired.height + margin.vertical();

        if (layout.direction() == LayoutDirection::Horizontal) {
            contentW += childTotalW;
            contentH = (std::max)(contentH, childTotalH);
        } else {
            contentW = (std::max)(contentW, childTotalW);
            contentH += childTotalH;
        }
    }

    if (visibleCount > 1) {
        if (layout.direction() == LayoutDirection::Horizontal) {
            contentW += layout.gap() * (visibleCount - 1);
        } else {
            contentH += layout.gap() * (visibleCount - 1);
        }
    }

    float finalW = (targetW >= 0.0f) ? targetW : (contentW + pad.horizontal());
    float finalH = (targetH >= 0.0f) ? targetH : (contentH + pad.vertical());

    finalW = (std::clamp)(finalW, layout.minWidth(), layout.maxWidth());
    finalH = (std::clamp)(finalH, layout.minHeight(), layout.maxHeight());

    element->setDesiredSize(Dimensions{ finalW, finalH });
}

void LayoutEngine::Arrange(UIElement* element, const Rect& finalRect) {
    if (!element) return;

    element->setBounds(finalRect);
    element->clearLayoutDirty();

    Rect contentRect = finalRect.inset(element->layout().padding());
    ArrangeContent(element, contentRect);
}

void LayoutEngine::ArrangeContent(UIElement* element, const Rect& contentRect) {
    if (!element || element->children().empty()) return;

    if (element->layout().direction() == LayoutDirection::Horizontal) {
        ArrangeHorizontal(element, contentRect);
    } else {
        ArrangeVertical(element, contentRect);
    }
}

void LayoutEngine::ArrangeHorizontal(UIElement* element, const Rect& contentRect) {
    std::vector<UIElement*> visible;
    for (const auto& child : element->children()) {
        if (child && child->isVisible()) {
            visible.push_back(child.get());
        }
    }
    if (visible.empty()) return;

    float totalFlex = 0.0f;
    float nonFlexWidth = 0.0f;

    for (auto* child : visible) {
        float flex = child->layout().flexGrow();
        if (flex <= 0.0f && child->layout().width().mode == SizeMode::Flex) {
            flex = child->layout().width().value;
        }

        if (flex > 0.0f) {
            totalFlex += flex;
        } else {
            nonFlexWidth += child->desiredSize().width + child->layout().margin().horizontal();
        }
    }

    float totalGap = (visible.size() > 1) ? (element->layout().gap() * (visible.size() - 1)) : 0.0f;
    float remainingSpace = contentRect.width - nonFlexWidth - totalGap;

    float currentX = contentRect.x;
    float currentGap = element->layout().gap();
    float flexSpacePerUnit = 0.0f;

    if (totalFlex > 0.0f) {
        flexSpacePerUnit = (std::max)(0.0f, remainingSpace) / totalFlex;
    } else {
        float leftover = (std::max)(0.0f, remainingSpace);
        switch (element->layout().justify()) {
        case JustifyContent::Start:
            currentX = contentRect.x;
            break;
        case JustifyContent::Center:
            currentX = contentRect.x + leftover * 0.5f;
            break;
        case JustifyContent::End:
            currentX = contentRect.x + leftover;
            break;
        case JustifyContent::SpaceBetween:
            currentX = contentRect.x;
            if (visible.size() > 1) {
                currentGap += leftover / (visible.size() - 1);
            }
            break;
        case JustifyContent::SpaceAround:
            if (!visible.empty()) {
                float space = leftover / visible.size();
                currentX = contentRect.x + space * 0.5f;
                currentGap += space;
            }
            break;
        }
    }

    for (auto* child : visible) {
        const auto& margin = child->layout().margin();
        float flex = child->layout().flexGrow();
        if (flex <= 0.0f && child->layout().width().mode == SizeMode::Flex) {
            flex = child->layout().width().value;
        }

        float childW = 0.0f;
        if (flex > 0.0f) {
            childW = (std::max)(0.0f, (flex * flexSpacePerUnit) - margin.horizontal());
        } else {
            childW = child->desiredSize().width;
        }
        childW = (std::clamp)(childW, child->layout().minWidth(), child->layout().maxWidth());
        float maxAvailableChildW = (std::max)(0.0f, (contentRect.x + contentRect.width) - (currentX + margin.left) - margin.right);
        childW = (std::min)(childW, maxAvailableChildW);

        Alignment align = element->layout().alignment();
        if (child->layout().alignment() != Alignment::Stretch) {
            align = child->layout().alignment();
        }

        float childH = child->desiredSize().height;
        if (child->layout().height().mode == SizeMode::Fixed) {
            childH = child->layout().height().value;
        } else if (align == Alignment::Stretch || child->layout().height().mode == SizeMode::Flex) {
            childH = (std::max)(0.0f, contentRect.height - margin.vertical());
        }
        childH = (std::clamp)(childH, child->layout().minHeight(), child->layout().maxHeight());

        float childY = contentRect.y + margin.top;
        if (align == Alignment::Center) {
            childY = contentRect.y + margin.top + (std::max)(0.0f, (contentRect.height - margin.vertical() - childH) * 0.5f);
        } else if (align == Alignment::End) {
            childY = contentRect.y + contentRect.height - margin.bottom - childH;
        }

        float childX = currentX + margin.left;
        child->arrange(Rect{ childX, childY, childW, childH });

        currentX += margin.left + childW + margin.right + currentGap;
    }
}

void LayoutEngine::ArrangeVertical(UIElement* element, const Rect& contentRect) {
    std::vector<UIElement*> visible;
    for (const auto& child : element->children()) {
        if (child && child->isVisible()) {
            visible.push_back(child.get());
        }
    }
    if (visible.empty()) return;

    float totalFlex = 0.0f;
    float nonFlexHeight = 0.0f;

    for (auto* child : visible) {
        float flex = child->layout().flexGrow();
        if (flex <= 0.0f && child->layout().height().mode == SizeMode::Flex) {
            flex = child->layout().height().value;
        }

        if (flex > 0.0f) {
            totalFlex += flex;
        } else {
            nonFlexHeight += child->desiredSize().height + child->layout().margin().vertical();
        }
    }

    float totalGap = (visible.size() > 1) ? (element->layout().gap() * (visible.size() - 1)) : 0.0f;
    float remainingSpace = contentRect.height - nonFlexHeight - totalGap;

    float currentY = contentRect.y;
    float currentGap = element->layout().gap();
    float flexSpacePerUnit = 0.0f;

    if (totalFlex > 0.0f) {
        flexSpacePerUnit = (std::max)(0.0f, remainingSpace) / totalFlex;
    } else {
        float leftover = (std::max)(0.0f, remainingSpace);
        switch (element->layout().justify()) {
        case JustifyContent::Start:
            currentY = contentRect.y;
            break;
        case JustifyContent::Center:
            currentY = contentRect.y + leftover * 0.5f;
            break;
        case JustifyContent::End:
            currentY = contentRect.y + leftover;
            break;
        case JustifyContent::SpaceBetween:
            currentY = contentRect.y;
            if (visible.size() > 1) {
                currentGap += leftover / (visible.size() - 1);
            }
            break;
        case JustifyContent::SpaceAround:
            if (!visible.empty()) {
                float space = leftover / visible.size();
                currentY = contentRect.y + space * 0.5f;
                currentGap += space;
            }
            break;
        }
    }

    for (auto* child : visible) {
        const auto& margin = child->layout().margin();
        float flex = child->layout().flexGrow();
        if (flex <= 0.0f && child->layout().height().mode == SizeMode::Flex) {
            flex = child->layout().height().value;
        }

        float childH = 0.0f;
        if (flex > 0.0f) {
            childH = (std::max)(0.0f, (flex * flexSpacePerUnit) - margin.vertical());
        } else {
            childH = child->desiredSize().height;
        }
        childH = (std::clamp)(childH, child->layout().minHeight(), child->layout().maxHeight());
        float maxAvailableChildH = (std::max)(0.0f, (contentRect.y + contentRect.height) - (currentY + margin.top) - margin.bottom);
        childH = (std::min)(childH, maxAvailableChildH);

        Alignment align = element->layout().alignment();
        if (child->layout().alignment() != Alignment::Stretch) {
            align = child->layout().alignment();
        }

        float childW = child->desiredSize().width;
        if (child->layout().width().mode == SizeMode::Fixed) {
            childW = child->layout().width().value;
        } else if (align == Alignment::Stretch || child->layout().width().mode == SizeMode::Flex) {
            childW = (std::max)(0.0f, contentRect.width - margin.horizontal());
        }
        childW = (std::clamp)(childW, child->layout().minWidth(), child->layout().maxWidth());
        childW = (std::min)(childW, (std::max)(0.0f, contentRect.width - margin.horizontal()));

        float childX = contentRect.x + margin.left;
        if (align == Alignment::Center) {
            childX = contentRect.x + margin.left + (std::max)(0.0f, (contentRect.width - margin.horizontal() - childW) * 0.5f);
        } else if (align == Alignment::End) {
            childX = contentRect.x + contentRect.width - margin.right - childW;
        }

        float childY = currentY + margin.top;
        child->arrange(Rect{ childX, childY, childW, childH });

        currentY += margin.top + childH + margin.bottom + currentGap;
    }
}

} // namespace PerfUI
