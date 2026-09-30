#pragma once

#include "Types.h"
#include "UIRenderBackend.h"
#include "UIElement.h"
#include <memory>
#include <vector>
#include <string>

namespace PerfUI {

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

    // Lifecycle
    void update(float deltaTime);
    void render(UIRenderBackend& backend);

    // Input Handling
    void onMouseMove(const Point& screenPos);
    void onMouseDown(int button, const Point& screenPos);
    void onMouseUp(int button, const Point& screenPos);
    void onNavigate(NavDirection direction);
    void onSubmit();
    void onCancel();

    // Focus Management
    void setFocus(UIElement* element);
    UIElement* focusedElement() const { return m_focusedElement; }
    void clearFocus();

    // Dirty flags
    void requestLayout();
    bool isLayoutDirty() const { return m_layoutDirty; }

private:
    void performLayout();

    std::unique_ptr<UIElement> m_root;
    Dimensions m_viewportSize{ 1920.0f, 1080.0f };

    UIElement* m_focusedElement{ nullptr };
    UIElement* m_hoveredElement{ nullptr };
    UIElement* m_pressedElement{ nullptr };

    Point m_lastMousePos{ -1.0f, -1.0f };
    bool m_layoutDirty{ true };
};

} // namespace PerfUI
