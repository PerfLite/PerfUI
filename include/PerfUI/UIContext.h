#pragma once

#include "Types.h"
#include "Animation.h"
#include "UIRenderBackend.h"
#include "UIElement.h"
#include "Toast.h"
#include "ContextMenu.h"
#include "OverlayManager.h"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <thread>
#include <mutex>
#include <cassert>

namespace PerfUI {

class ModalDialog;
class ComboBox;

class PERFUI_API UIContext {
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
    void render(UIRenderBackend& backend, bool renderTree = true);

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

    // Thread Safety & Task Dispatch
    void runOnUIThread(std::function<void()> task);
    bool isUIThread() const;
    void assertUIThread() const;

    // Client Overlays System (Phase 1 Clean Client)
    OverlayManager& overlayManager() { return m_overlayManager; }
    const OverlayManager& overlayManager() const { return m_overlayManager; }

    OverlayId registerOverlay(const char* name, OverlayCallback cb, int zOrder = 0, bool alwaysVisible = true) {
        return m_overlayManager.registerOverlay(name, std::move(cb), zOrder, alwaysVisible);
    }
    void unregisterOverlay(OverlayId id) {
        m_overlayManager.unregisterOverlay(id);
    }
    void setOverlayVisible(OverlayId id, bool visible) {
        m_overlayManager.setOverlayVisible(id, visible);
    }
    bool isOverlayVisible(OverlayId id) const {
        return m_overlayManager.isOverlayVisible(id);
    }
    bool hasVisibleOverlays() const {
        return m_overlayManager.hasVisibleOverlays();
    }

    // Texture Management API (Stage 2 Clean Client)
    TextureId loadTexture(std::string_view filePath) {
        return m_renderBackend ? m_renderBackend->loadTexture(filePath) : 0;
    }
    TextureId createDynamicTexture(uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
        return m_renderBackend ? m_renderBackend->createDynamicTexture(width, height, rgbaPixels) : 0;
    }
    bool updateDynamicTexture(TextureId id, uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
        return m_renderBackend ? m_renderBackend->updateDynamicTexture(id, width, height, rgbaPixels) : false;
    }
    void destroyTexture(TextureId id) {
        if (m_renderBackend) m_renderBackend->destroyTexture(id);
    }
    Dimensions getTextureSize(TextureId id) {
        return m_renderBackend ? m_renderBackend->getTextureSize(id) : Dimensions{ 0.0f, 0.0f };
    }

    // Hotkey Management API (Stage 3 Clean Client)
    bool registerHotkey(std::string id, uint32_t keyCode, std::function<void()> callback);
    void unregisterHotkey(const std::string& id);
    bool triggerHotkey(uint32_t keyCode);
    size_t hotkeyCount() const;
    void clearHotkeys();

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
    OverlayManager m_overlayManager;
    float m_lastDeltaTime{ 0.016f };

    // Hotkeys storage
    struct HotkeyItem {
        uint32_t keyCode{ 0 };
        std::function<void()> callback;
    };
    mutable std::mutex m_hotkeyMutex;
    std::unordered_map<std::string, HotkeyItem> m_hotkeys;

    // Threading
    std::thread::id m_uiThreadId{};
    mutable std::mutex m_taskMutex;
    std::vector<std::function<void()>> m_taskQueue;
};

} // namespace PerfUI
