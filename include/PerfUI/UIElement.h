#pragma once

#include "Types.h"
#include "Layout.h"
#include "UIRenderBackend.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace PerfUI {

class UIContext;

class UIElement {
public:
    explicit UIElement(std::string name = "");
    virtual ~UIElement();

    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete;
    UIElement(UIElement&&) noexcept = default;
    UIElement& operator=(UIElement&&) noexcept = default;

    // Hierarchy
    UIElement* parent() const { return m_parent; }
    const std::vector<std::unique_ptr<UIElement>>& children() const { return m_children; }

    template <typename T, typename... Args>
    T* add(Args&&... args) {
        static_assert(std::is_base_of_v<UIElement, T>, "T must derive from UIElement");
        auto child = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = child.get();
        child->m_parent = this;
        child->setContext(m_context);
        m_children.push_back(std::move(child));
        markLayoutDirty();
        return ptr;
    }

    void removeChild(UIElement* child);
    void clearChildren();

    // Identification & Context
    ElementId id() const { return m_id; }
    const std::string& name() const { return m_name; }
    UIContext* context() const { return m_context; }
    void setContext(UIContext* ctx);

    // Visibility & Interaction
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible);

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    bool isFocusable() const { return m_focusable; }
    void setFocusable(bool focusable) { m_focusable = focusable; }

    bool isFocused() const;

    WidgetState currentState() const;

    // Tooltip
    const std::string& tooltip() const { return m_tooltip; }
    UIElement& tooltip(std::string text) { m_tooltip = std::move(text); return *this; }

    // Bounds & Geometry
    const Rect& bounds() const { return m_bounds; }
    void setBounds(const Rect& bounds);

    const Dimensions& desiredSize() const { return m_desiredSize; }
    void setDesiredSize(Dimensions size) { m_desiredSize = size; }

    // Layout
    LayoutProps& layout() { return m_layout; }
    const LayoutProps& layout() const { return m_layout; }

    // Dirty flags
    bool isLayoutDirty() const { return m_layoutDirty; }
    void markLayoutDirty();
    void clearLayoutDirty() { m_layoutDirty = false; }

    // Lifecycle passes
    virtual void update(float deltaTime);
    virtual void measure(Dimensions availableSize);
    virtual void arrange(const Rect& finalRect);
    virtual void render(UIRenderBackend& backend);

    // Hit Testing & Events
    virtual UIElement* hitTest(const Point& point);
    virtual bool onPointerDown(const Point& localPoint);
    virtual bool onPointerUp(const Point& localPoint);
    virtual void onPointerMove(const Point& localPoint) { (void)localPoint; }
    virtual void onPointerEnter();
    virtual void onPointerLeave();
    virtual bool onMouseWheel(float delta, const Point& localPoint);
    virtual void onFocusChanged(bool focused);
    virtual bool onAction(NavDirection dir);
    virtual bool onCharInput(uint32_t charCode) { (void)charCode; return false; }
    virtual bool onKeyDown(int keyCode) { (void)keyCode; return false; }
    virtual bool onContextMenu(const Point& localPoint);

    void onContextMenu(std::function<bool(const Point&)> callback) {
        m_onContextMenu = std::move(callback);
    }

protected:
    static ElementId generateNextId();

    ElementId m_id{ 0 };
    std::string m_name;
    UIElement* m_parent{ nullptr };
    UIContext* m_context{ nullptr };
    std::vector<std::unique_ptr<UIElement>> m_children;

    Rect m_bounds{};
    Dimensions m_desiredSize{};
    LayoutProps m_layout{};

    bool m_visible{ true };
    bool m_enabled{ true };
    bool m_focusable{ false };
    bool m_hovered{ false };
    bool m_pressed{ false };

    bool m_layoutDirty{ true };
    std::string m_tooltip;
    std::function<bool(const Point&)> m_onContextMenu;
};

} // namespace PerfUI
