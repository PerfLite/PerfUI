#pragma once

#include "Types.h"
#include "Animation.h"
#include "UIRenderBackend.h"
#include "UIElement.h"
#include "Toast.h"
#include "ContextMenu.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace PerfUI {

class ModalDialog;
class ComboBox;

class UIContext {
public:
    UIContext();
    ~UIContext();

    UIContext(const UIContext&) = delete;
    UIContext& operator=(const UIContext&) = delete;

    // Viewport & Root
    void setViewportSize(Dimensions size);
    Dimensions viewportSize() const { return m_viewportSize; }

    UIElement* root() const { return m_root.get(); }

    template <typename T, typename... Args>
    T* addWindow(Args&&... args) {
        return m_root->add<T>(std::forward<Args>(args)...);
    }

    // Backend
    UIRenderBackend* renderBackend() const { return m_renderBackend; }
    void setRenderBackend(UIRenderBackend* backend) { m_renderBackend = backend; }

    // Lifecycle
    void update(float deltaTime);
    void render(UIRenderBackend& backend);

    // Input Handling
    void onMouseMove(const Point& screenPos);
    void onMouseDown(int button, const Point& screenPos);
    void onMouseUp(int button, const Point& screenPos);
    void onMouseWheel(float delta, const Point& screenPos);
    void onNavigate(NavDirection direction);
    void onSubmit();
    void onCancel();
    void onCharInput(uint32_t charCode);
    void onKeyDown(int keyCode);

    // Focus Management
    void setFocus(UIElement* element);
    UIElement* focusedElement() const { return m_focusedElement; }
    void clearFocus();

    // Element destruction lifecycle
    void notifyElementDestroyed(UIElement* element);

    // Dirty flags
    void requestLayout();
    bool isLayoutDirty() const { return m_layoutDirty; }

    // Performance & Profiling (Phase 12)
    struct ProfilerMetrics {
        float layoutTimeUs{ 0.0f };
        float renderTimeUs{ 0.0f };
        uint32_t elementCount{ 0 };
    };

    const ProfilerMetrics& metrics() const { return m_metrics; }

    // Overlay Layer (Tooltips, Popups, Dropdowns)
    void setOverlayRender(std::function<void(UIRenderBackend&)> callback) {
        m_overlayRenderCallback = std::move(callback);
    }
    void clearOverlayRender() {
        m_overlayRenderCallback = nullptr;
    }
    Point lastMousePos() const { return m_lastMousePos; }

    // Sound Feedback
    using SoundCallback = std::function<void(const std::string&)>;
    void setSoundCallback(SoundCallback cb) { m_soundCallback = std::move(cb); }
    void playSound(const std::string& soundId);

    // Toast Notifications
    void showToast(std::string title, std::string message, ToastType type = ToastType::Info, float duration = 3.2f);

    // Modal Dialog System
    void showModal(std::shared_ptr<ModalDialog> modal);
    void closeModal();
    ModalDialog* activeModal() const { return m_activeModal.get(); }
    bool hasActiveModal() const { return m_activeModal != nullptr; }

    // Keybind capturing state
    bool isCapturingKeybind() const { return m_capturingKeybind; }
    void setCapturingKeybind(bool capturing) { m_capturingKeybind = capturing; }

    // ComboBox Dropdown Overlay Tracking
    void setActiveComboBox(ComboBox* cb);
    void clearActiveComboBox(ComboBox* cb = nullptr);
    ComboBox* activeComboBox() const { return m_activeComboBox; }
    bool hasActiveComboBox() const { return m_activeComboBox != nullptr; }

    // Context Menu System (Phase 18)
    void showContextMenu(const Point& screenPos, std::vector<ContextMenuItem> items);
    void closeContextMenu();
    bool hasContextMenu() const { return m_contextMenu.isOpen(); }

private:
    void performLayout();

    std::unique_ptr<UIElement> m_root;
    UIRenderBackend* m_renderBackend{ nullptr };
    Dimensions m_viewportSize{ 1920.0f, 1080.0f };

    UIElement* m_focusedElement{ nullptr };
    UIElement* m_hoveredElement{ nullptr };
    UIElement* m_pressedElement{ nullptr };

    Point m_lastMousePos{ -1.0f, -1.0f };
    bool m_layoutDirty{ true };
    ProfilerMetrics m_metrics;

    void renderOverlay(UIRenderBackend& backend);

    std::function<void(UIRenderBackend&)> m_overlayRenderCallback;
    float m_tooltipHoverTimer{ 0.0f };
    AnimatedFloat m_tooltipAlpha{ 0.0f, 14.0f };
    std::string m_lastTooltipText;

    SoundCallback m_soundCallback;
    ToastManager m_toastManager;
    std::shared_ptr<ModalDialog> m_activeModal;
    bool m_capturingKeybind{ false };
    ContextMenu m_contextMenu;
    ComboBox* m_activeComboBox{ nullptr };
};

} // namespace PerfUI
