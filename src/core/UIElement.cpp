#include "PerfUI/UIElement.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/LayoutEngine.h"
#include <atomic>
#include <algorithm>

namespace PerfUI {

ElementId UIElement::generateNextId() {
    static std::atomic<ElementId> s_nextId{ 1 };
    return s_nextId.fetch_add(1, std::memory_order_relaxed);
}

UIElement::UIElement(std::string name)
    : m_id(generateNextId())
    , m_name(std::move(name))
{
}

void UIElement::setContext(UIContext* ctx) {
    m_context = ctx;
    for (auto& child : m_children) {
        if (child) {
            child->setContext(ctx);
        }
    }
}

void UIElement::removeChild(UIElement* child) {
    if (!child) return;
    auto it = std::remove_if(m_children.begin(), m_children.end(),
        [child](const std::unique_ptr<UIElement>& ptr) {
            return ptr.get() == child;
        });

    if (it != m_children.end()) {
        m_children.erase(it, m_children.end());
        markLayoutDirty();
    }
}

void UIElement::clearChildren() {
    m_children.clear();
    markLayoutDirty();
}

void UIElement::setVisible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        markLayoutDirty();
    }
}

void UIElement::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool UIElement::isFocused() const {
    return m_context && m_context->focusedElement() == this;
}

WidgetState UIElement::currentState() const {
    if (!m_enabled) return WidgetState::Disabled;
    if (m_pressed) return WidgetState::Pressed;
    if (m_hovered) return WidgetState::Hovered;
    if (isFocused()) return WidgetState::Focused;
    return WidgetState::Normal;
}

void UIElement::setBounds(const Rect& bounds) {
    m_bounds = bounds;
}

void UIElement::markLayoutDirty() {
    m_layoutDirty = true;
    if (m_parent) {
        m_parent->markLayoutDirty();
    } else if (m_context) {
        m_context->requestLayout();
    }
}

void UIElement::update(float deltaTime) {
    if (!m_visible) return;

    for (auto& child : m_children) {
        if (child && child->isVisible()) {
            child->update(deltaTime);
        }
    }
}

void UIElement::measure(Dimensions availableSize) {
    LayoutEngine::Measure(this, availableSize);
}

void UIElement::arrange(const Rect& finalRect) {
    LayoutEngine::Arrange(this, finalRect);
}

void UIElement::render(UIRenderBackend& backend) {
    if (!m_visible) return;

    for (auto& child : m_children) {
        if (child && child->isVisible()) {
            child->render(backend);
        }
    }
}

UIElement* UIElement::hitTest(const Point& point) {
    if (!m_visible || !m_bounds.contains(point)) {
        return nullptr;
    }

    // Traverse children top to bottom (last child is on top)
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if (auto* target = (*it)->hitTest(point)) {
            return target;
        }
    }

    return this;
}

bool UIElement::onPointerDown(const Point& localPoint) {
    (void)localPoint;
    m_pressed = true;
    return true;
}

bool UIElement::onPointerUp(const Point& localPoint) {
    (void)localPoint;
    m_pressed = false;
    return true;
}

void UIElement::onPointerEnter() {
    m_hovered = true;
}

void UIElement::onPointerLeave() {
    m_hovered = false;
    m_pressed = false;
}

bool UIElement::onMouseWheel(float delta, const Point& localPoint) {
    (void)delta;
    (void)localPoint;
    return false;
}

void UIElement::onFocusChanged(bool focused) {
    (void)focused;
}

bool UIElement::onAction(NavDirection dir) {
    (void)dir;
    return false;
}

} // namespace PerfUI
