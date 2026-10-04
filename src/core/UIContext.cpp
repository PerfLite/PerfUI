#include "PerfUI/UIContext.h"
#include "PerfUI/ModalDialog.h"
#include "PerfUI/ComboBox.h"
#include <algorithm>
#include <chrono>

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
    m_uiThreadId = std::this_thread::get_id();
    assertUIThread();
    m_lastDeltaTime = deltaTime;

    // 1. Drain queued UI-thread tasks at the start of update
    std::vector<std::function<void()>> tasks;
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        tasks.swap(m_taskQueue);
    }
    for (auto& task : tasks) {
        if (task) {
            task();
        }
    }

    if (m_layoutDirty) {
        performLayout();
    }

    if (m_root) {
        m_root->update(deltaTime);
    }

    // Modal dialog lifecycle
    if (m_activeModal) {
        m_activeModal->update(deltaTime);
        if (!m_activeModal->isOpen() && m_activeModal->alpha() <= 0.02f) {
            m_activeModal.reset();
        }
    }

    // Toast notifications lifecycle
    m_toastManager.update(deltaTime);

    // Context menu lifecycle
    m_contextMenu.update(deltaTime);

    // Tooltip timer logic
    if (m_hoveredElement && !m_hoveredElement->tooltip().empty()) {
        if (m_hoveredElement->tooltip() != m_lastTooltipText) {
            m_lastTooltipText = m_hoveredElement->tooltip();
            m_tooltipHoverTimer = 0.0f;
            m_tooltipAlpha.snapTo(0.0f);
        }
        m_tooltipHoverTimer += deltaTime;
        if (m_tooltipHoverTimer >= 0.35f) {
            m_tooltipAlpha.setTarget(1.0f);
        }
    } else {
        m_tooltipHoverTimer = 0.0f;
        m_tooltipAlpha.setTarget(0.0f);
    }
    m_tooltipAlpha.update(deltaTime);
}

static uint32_t CountElementsRecursive(const UIElement* elem) {
    if (!elem) return 0;
    uint32_t count = 1;
    for (const auto& child : elem->children()) {
        count += CountElementsRecursive(child.get());
    }
    return count;
}

void UIContext::performLayout() {
    if (!m_root) return;

    auto start = std::chrono::high_resolution_clock::now();
    m_root->measure(m_viewportSize);
    m_root->arrange(Rect{ 0.0f, 0.0f, m_viewportSize.width, m_viewportSize.height });

    if (m_activeModal) {
        m_activeModal->measure(m_viewportSize);
        m_activeModal->arrange(Rect{ 0.0f, 0.0f, m_viewportSize.width, m_viewportSize.height });
    }

    m_layoutDirty = false;
    auto end = std::chrono::high_resolution_clock::now();

    m_metrics.layoutTimeUs = std::chrono::duration<float, std::micro>(end - start).count();
    m_metrics.elementCount = CountElementsRecursive(m_root.get());
}

void UIContext::render(UIRenderBackend& backend, bool renderTree) {
    m_renderBackend = &backend;

    if (renderTree && m_layoutDirty) {
        performLayout();
    }

    auto start = std::chrono::high_resolution_clock::now();
    backend.beginFrame();
    if (renderTree && m_root && m_root->isVisible()) {
        m_root->render(backend);
    }
    if (renderTree) {
        renderOverlay(backend);
    }

    // 5. Client Overlays (Clean Client Stage 1)
    OverlayContext overlayCtx{ backend, m_viewportSize, 1.0f, m_lastDeltaTime };
    m_overlayManager.renderOverlays(overlayCtx, renderTree);

    backend.endFrame();
    auto end = std::chrono::high_resolution_clock::now();

    m_metrics.renderTimeUs = std::chrono::duration<float, std::micro>(end - start).count();
}

void UIContext::renderOverlay(UIRenderBackend& backend) {
    // 1. Custom overlay callback (popups, dropdowns)
    if (m_overlayRenderCallback) {
        m_overlayRenderCallback(backend);
    }

    // 2. Modal Dialog rendering
    if (m_activeModal) {
        m_activeModal->render(backend);
    }

    // 3. ComboBox dropdown overlay
    if (m_activeComboBox && m_activeComboBox->isOpen()) {
        m_activeComboBox->renderDropdownOverlay(backend);
    }

    // 4. Tooltip rendering
    float alpha = m_tooltipAlpha.value();
    if (alpha > 0.01f && !m_lastTooltipText.empty()) {
        TextStyle textStyle;
        textStyle.fontSize = 12.0f;
        textStyle.color = Color(240, 246, 252, static_cast<uint8_t>(255.0f * alpha));

        Dimensions textDim = backend.measureText(m_lastTooltipText, textStyle);
        float padH = 12.0f;
        float padV = 8.0f;
        float cardW = textDim.width + padH * 2.0f;
        float cardH = textDim.height + padV * 2.0f;

        float posX = m_lastMousePos.x + 14.0f;
        float posY = m_lastMousePos.y + 16.0f;

        // Clamp to viewport
        if (posX + cardW > m_viewportSize.width - 8.0f) {
            posX = m_lastMousePos.x - cardW - 6.0f;
        }
        if (posY + cardH > m_viewportSize.height - 8.0f) {
            posY = m_lastMousePos.y - cardH - 6.0f;
        }
        posX = (std::max)(8.0f, posX);
        posY = (std::max)(8.0f, posY);

        Rect cardRect{ posX, posY, cardW, cardH };

        // Soft drop shadow
        Color shadowColor = Color(0, 0, 0, static_cast<uint8_t>(180.0f * alpha));
        backend.drawShadow(cardRect, 6.0f, shadowColor, 12.0f * alpha, { 0.0f, 4.0f });

        // Background & subtle gold border
        Color bg = Color(16, 21, 28, static_cast<uint8_t>(245.0f * alpha));
        Color border = Color(212, 175, 55, static_cast<uint8_t>(210.0f * alpha));
        backend.drawRoundedRect(cardRect, bg, 6.0f, border, 1.0f);

        // Tooltip text
        backend.drawText(m_lastTooltipText, Point{ posX + padH, posY + padV }, textStyle);
    }

    // 5. Toast notifications rendering
    m_toastManager.render(backend, m_viewportSize);

    // 6. Context Menu rendering (Phase 18)
    if (m_contextMenu.isOpen()) {
        m_contextMenu.render(backend, m_viewportSize);
    }
}

void UIContext::onMouseMove(const Point& screenPos) {
    m_lastMousePos = screenPos;

    if (m_contextMenu.isOpen()) {
        m_contextMenu.onMouseMove(screenPos);
    }

    if (m_activeComboBox && m_activeComboBox->isOpen()) {
        m_activeComboBox->onDropdownMouseMove(screenPos);
    }

    if (m_pressedElement) {
        Point localPoint = screenPos - m_pressedElement->bounds().topLeft();
        m_pressedElement->onPointerMove(localPoint);
    }

    UIElement* searchRoot = (m_activeModal && m_activeModal->isOpen()) ? m_activeModal.get() : m_root.get();
    if (!searchRoot) return;

    UIElement* hovered = searchRoot->hitTest(screenPos);

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

    // 1. Context Menu has top priority
    if (m_contextMenu.isOpen()) {
        if (m_contextMenu.onPointerDown(screenPos)) {
            return;
        }
    }

    // 2. ComboBox dropdown popup has next priority
    if (m_activeComboBox && m_activeComboBox->isOpen()) {
        ComboBox* activeCombo = m_activeComboBox;
        if (activeCombo->dropdownBounds().contains(screenPos)) {
            activeCombo->onDropdownPointerDown(screenPos);
            return;
        } else if (activeCombo->bounds().contains(screenPos)) {
            activeCombo->setOpen(false);
            return;
        } else {
            // Click outside closes the dropdown and swallows the click
            activeCombo->setOpen(false);
            return;
        }
    }

    // 3. Modal Dialog has next priority
    if (m_activeModal && m_activeModal->isOpen()) {
        if (button == 0) { // Left Mouse Button
            UIElement* target = m_activeModal->hitTest(screenPos);
            if (!target || target == m_activeModal.get()) {
                m_activeModal->onPointerDown(screenPos);
                return;
            }

            if (target != m_focusedElement) {
                setFocus(target && target->isFocusable() ? target : nullptr);
            }

            m_pressedElement = nullptr;
            UIElement* curr = target;
            while (curr && curr != m_activeModal->parent()) {
                Point localPoint = screenPos - curr->bounds().topLeft();
                if (curr->onPointerDown(localPoint)) {
                    m_pressedElement = curr;
                    break;
                }
                curr = curr->parent();
            }
        }
        return;
    }

    // 4. Main element hierarchy
    if (!m_root) return;

    UIElement* target = m_root->hitTest(screenPos);

    if (button == 0) { // Left Mouse Button
        if (target != m_focusedElement) {
            setFocus(target && target->isFocusable() ? target : nullptr);
        }

        m_pressedElement = nullptr;
        UIElement* curr = target;
        while (curr) {
            Point localPoint = screenPos - curr->bounds().topLeft();
            if (curr->onPointerDown(localPoint)) {
                m_pressedElement = curr;
                break;
            }
            curr = curr->parent();
        }
    } else if (button == 1) { // Right Mouse Button (Context Menu)
        UIElement* curr = target;
        while (curr) {
            Point localPoint = screenPos - curr->bounds().topLeft();
            if (curr->onContextMenu(localPoint)) {
                break;
            }
            curr = curr->parent();
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

void UIContext::onMouseWheel(float delta, const Point& screenPos) {
    m_lastMousePos = screenPos;

    if (m_activeComboBox && m_activeComboBox->isOpen()) {
        m_activeComboBox->setOpen(false);
    }

    UIElement* searchRoot = (m_activeModal && m_activeModal->isOpen()) ? m_activeModal.get() : m_root.get();
    if (!searchRoot) return;

    UIElement* target = searchRoot->hitTest(screenPos);
    for (UIElement* el = target; el != nullptr; el = el->parent()) {
        Point localPoint = screenPos - el->bounds().topLeft();
        if (el->onMouseWheel(delta, localPoint)) {
            break;
        }
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

void UIContext::onCharInput(uint32_t charCode) {
    if (m_focusedElement) {
        m_focusedElement->onCharInput(charCode);
    }
}

void UIContext::onKeyDown(int keyCode) {
    if (m_activeComboBox && m_activeComboBox->isOpen()) {
        if (keyCode == 0x1B) { // VK_ESCAPE
            m_activeComboBox->setOpen(false);
            return;
        }
    }

    if (m_activeModal && m_activeModal->isOpen()) {
        if (m_activeModal->onKeyDown(keyCode)) {
            return;
        }
    }

    if (m_focusedElement) {
        m_focusedElement->onKeyDown(keyCode);
    }
}

void UIContext::setActiveComboBox(ComboBox* cb) {
    if (m_activeComboBox && m_activeComboBox != cb) {
        m_activeComboBox->setOpen(false);
    }
    m_activeComboBox = cb;
}

void UIContext::clearActiveComboBox(ComboBox* cb) {
    if (!cb || m_activeComboBox == cb) {
        m_activeComboBox = nullptr;
    }
}

void UIContext::playSound(const std::string& soundId) {
    if (m_soundCallback) {
        m_soundCallback(soundId);
    }
}

void UIContext::showToast(std::string title, std::string message, ToastType type, float duration) {
    m_toastManager.show(std::move(title), std::move(message), type, duration);
}

void UIContext::showModal(std::shared_ptr<ModalDialog> modal) {
    if (m_activeComboBox) {
        m_activeComboBox->setOpen(false);
    }
    m_activeModal = std::move(modal);
    if (m_activeModal) {
        m_activeModal->setContext(this);
        m_activeModal->measure(m_viewportSize);
        m_activeModal->arrange(Rect{ 0.0f, 0.0f, m_viewportSize.width, m_viewportSize.height });
        m_activeModal->open();
        playSound("UIMenuOK");
    }
    requestLayout();
}

void UIContext::closeModal() {
    if (m_activeModal) {
        m_activeModal->close();
        playSound("UIMenuCancel");
    }
}

void UIContext::showContextMenu(const Point& screenPos, std::vector<ContextMenuItem> items) {
    if (m_activeComboBox) {
        m_activeComboBox->setOpen(false);
    }
    m_contextMenu.open(screenPos, std::move(items));
    playSound("UIMenuBlade");
}

void UIContext::closeContextMenu() {
    m_contextMenu.close();
}

void UIContext::setFocus(UIElement* element) {
    if (element && !element->isFocusable()) {
        element = nullptr;
    }

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

void UIContext::notifyElementDestroyed(UIElement* element) {
    if (!element) return;
    if (m_hoveredElement == element) {
        m_hoveredElement = nullptr;
    }
    if (m_pressedElement == element) {
        m_pressedElement = nullptr;
    }
    if (m_focusedElement == element) {
        m_focusedElement = nullptr;
    }
}

void UIContext::runOnUIThread(std::function<void()> task) {
    if (!task) return;
    std::lock_guard<std::mutex> lock(m_taskMutex);
    m_taskQueue.push_back(std::move(task));
}

bool UIContext::isUIThread() const {
    return m_uiThreadId == std::thread::id() || std::this_thread::get_id() == m_uiThreadId;
}

void UIContext::assertUIThread() const {
#ifndef NDEBUG
    assert(isUIThread() && "UI operation must execute on the designated UI thread!");
#endif
}

} // namespace PerfUI
