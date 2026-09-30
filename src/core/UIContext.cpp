#include "PerfUI/UIContext.h"
#include <algorithm>

namespace PerfUI {

UIContext::UIContext() {
    m_root = std::make_unique<UIElement>("Root");
    m_root->setContext(this);
}

UIContext::~UIContext() = default;

void UIContext::setViewportSize(Dimensions size) {
    if (m_viewportSize.width != size.width || m_viewportSize.height != size.height) {
        m_viewportSize = size;
        requestLayout();
    }
}

void UIContext::requestLayout() {
    m_layoutDirty = true;
}

void UIContext::update(float deltaTime) {
    if (m_layoutDirty) {
        performLayout();
    }

    if (m_root) {
        m_root->update(deltaTime);
    }
}

void UIContext::performLayout() {
    if (!m_root) return;

    m_root->measure(m_viewportSize);
    m_root->arrange(Rect{ 0.0f, 0.0f, m_viewportSize.width, m_viewportSize.height });
    m_layoutDirty = false;
}

void UIContext::render(UIRenderBackend& backend) {
    if (m_layoutDirty) {
        performLayout();
    }

    backend.beginFrame();
    if (m_root && m_root->isVisible()) {
        m_root->render(backend);
    }
    backend.endFrame();
}

void UIContext::onMouseMove(const Point& screenPos) {
    m_lastMousePos = screenPos;
    if (!m_root) return;

    UIElement* hovered = m_root->hitTest(screenPos);

    if (hovered != m_hoveredElement) {
        if (m_hoveredElement) {
            m_hoveredElement->onPointerLeave();
        }
        m_hoveredElement = hovered;
        if (m_hoveredElement) {
            m_hoveredElement->onPointerEnter();
        }
    }
}

void UIContext::onMouseDown(int button, const Point& screenPos) {
    m_lastMousePos = screenPos;
    if (!m_root) return;

    UIElement* target = m_root->hitTest(screenPos);

    if (button == 0) { // Left Mouse Button
        if (target != m_focusedElement) {
            setFocus(target && target->isFocusable() ? target : nullptr);
        }

        m_pressedElement = target;
        if (m_pressedElement) {
            Point localPoint = screenPos - m_pressedElement->bounds().topLeft();
            m_pressedElement->onPointerDown(localPoint);
        }
    }
}

void UIContext::onMouseUp(int button, const Point& screenPos) {
    m_lastMousePos = screenPos;
    if (button == 0 && m_pressedElement) {
        Point localPoint = screenPos - m_pressedElement->bounds().topLeft();
        m_pressedElement->onPointerUp(localPoint);
        m_pressedElement = nullptr;
    }
}

void UIContext::onNavigate(NavDirection direction) {
    if (m_focusedElement) {
        m_focusedElement->onAction(direction);
    }
}

void UIContext::onSubmit() {
    if (m_focusedElement) {
        m_focusedElement->onAction(NavDirection::Down); // Default submit action
    }
}

void UIContext::onCancel() {
    clearFocus();
}

void UIContext::setFocus(UIElement* element) {
    if (m_focusedElement == element) return;

    if (m_focusedElement) {
        m_focusedElement->onFocusChanged(false);
    }

    m_focusedElement = element;

    if (m_focusedElement) {
        m_focusedElement->onFocusChanged(true);
    }
}

void UIContext::clearFocus() {
    setFocus(nullptr);
}

} // namespace PerfUI
