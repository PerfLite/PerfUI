#pragma once

#include "Types.h"
#include "Layout.h"
#include "Style.h"
#include "PerfUI/Style/ComputedStyle.h"
#include <unordered_map>
#include "UIRenderBackend.h"
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <functional>

namespace PerfUI {

class UIContext;
class ElementAnimator;
class ElementSpring;
class WeakElementRef;
struct StyleTransitionController;

class PERFUI_API UIElement {
public:
    explicit UIElement(std::string name = "");
    virtual ~UIElement();

    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete;
    UIElement(UIElement&&) noexcept;
    UIElement& operator=(UIElement&&) noexcept;

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

    UIElement* addChild(std::unique_ptr<UIElement> child) {
        if (!child) return nullptr;
        UIElement* ptr = child.get();
        child->m_parent = this;
        child->setContext(m_context);
        m_children.push_back(std::move(child));
        markLayoutDirty();
        return ptr;
    }

    void removeChild(UIElement* child);
    void clearChildren();
    bool hasVisibleChildren() const {
        for (const auto& child : m_children) {
            if (child && child->isVisible()) {
                return true;
            }
        }
        return false;
    }

    // Identification & Context
    ElementId id() const { return m_id; }
    const std::string& name() const { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }
    UIElement& name(std::string name) { m_name = std::move(name); return *this; }
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

    // CSS-like classes (for styling / PSS)
    const std::vector<std::string>& classes() const { return m_classes; }
    void addClass(std::string className) {
        if (!className.empty() && !hasClass(className)) {
            m_classes.push_back(std::move(className));
            markStyleDirty();
        }
    }
    bool hasClass(std::string_view className) const {
        for (const auto& c : m_classes) {
            if (c == className) return true;
        }
        return false;
    }
    void removeClass(std::string_view className) {
        auto it = std::remove(m_classes.begin(), m_classes.end(), className);
        if (it != m_classes.end()) {
            m_classes.erase(it, m_classes.end());
            markStyleDirty();
        }
    }
    void toggleClass(std::string_view className) {
        if (hasClass(className)) removeClass(className);
        else addClass(std::string(className));
    }
    void setClasses(std::vector<std::string> cls) {
        m_classes = std::move(cls);
        markStyleDirty();
    }

    // Type Name & Pseudo-classes
    virtual std::string_view typeName() const;
    void setTypeName(std::string name);
    virtual uint8_t activePseudoClasses() const;
    virtual bool isChecked() const { return false; }

    // Computed style & PSS Cascade
    const ComputedStyle& computedStyle() const { return m_computedStyle; }
    void setComputedStyle(ComputedStyle s) { m_computedStyle = std::move(s); }
    const std::unordered_map<std::string, std::string>& inlineAttrs() const { return m_inlineAttrs; }
    void setInlineAttrs(std::unordered_map<std::string, std::string> attrs) { m_inlineAttrs = std::move(attrs); }
    void addInlineAttr(std::string k, std::string v) { m_inlineAttrs[std::move(k)] = std::move(v); }

    bool isHovered() const { return m_hovered; }
    bool isPressed() const { return m_pressed; }

    bool isStyleDirty() const { return m_styleDirty; }
    bool hasChildStyleDirty() const { return m_childStyleDirty; }
    void markStyleDirty();
    void invalidateStyleSubtree();
    void updateStyleCascade();
    void applyComputedStyle(const ComputedStyle& style);
    void applyComputedStyle(const ComputedStyle& oldStyle, const ComputedStyle& newStyle);
    bool isAlphaAnimating() const;
    bool isScaleAnimating() const;
    bool isPositionAnimating() const;
    StyleTransitionController* transitions() { return m_transitions.get(); }
    const StyleTransitionController* transitions() const { return m_transitions.get(); }
    virtual void onStyleApplied(const ComputedStyle& style) { (void)style; }

    // Opacity
    float opacity() const { return m_opacity; }
    float effectiveOpacity() const;
    void setOpacity(float opacity);
    UIElement& opacity(float opacity) { setOpacity(opacity); return *this; }

    // Outline (CSS Outline)
    std::optional<Color> outlineColor() const { return m_outlineColor; }
    void setOutlineColor(std::optional<Color> c) { m_outlineColor = c; }
    std::optional<float> outlineWidth() const { return m_outlineWidth; }
    void setOutlineWidth(std::optional<float> w) { m_outlineWidth = w; }
    std::optional<float> outlineOffset() const { return m_outlineOffset; }
    void setOutlineOffset(std::optional<float> o) { m_outlineOffset = o; }

    enum class AnimProp {
        Position,
        Scale,
        Size,
        Alpha,
        Color
    };
    void notifyAnimStarted(AnimProp prop);
    void notifySpringStarted(AnimProp prop);

    // Animation & Physics Springs
    ElementAnimator& animate();
    ElementSpring& spring();
    void stopAnimations();
    bool isAnimating() const;

    std::shared_ptr<bool> aliveToken() const { return m_aliveToken; }
    WeakElementRef weakRef();

    // Visual Transform Layer (Phase 2 - independent of flex layout, zero layout dirtying)
    Point renderOffset() const;
    void setRenderOffset(const Point& offset);
    void setRenderOffset(const Point& offset, const Point& offsetPercent);
    Point renderOffsetPercent() const;
    void setRenderOffsetPercent(const Point& percent);
    UIElement& renderOffset(Point offset);

    Point renderScale() const;
    void setRenderScale(const Point& scale);
    void setRenderScale(float uniformScale);
    UIElement& renderScale(Point scale);
    UIElement& renderScale(float uniformScale);

    Point pivot() const;
    void setPivot(const Point& pivot);
    UIElement& pivot(Point pivot);

    float renderOpacityMultiplier() const;
    void setRenderOpacityMultiplier(float mult);
    UIElement& renderOpacityMultiplier(float mult);

    bool hasVisualTransform() const;
    Point visualPosition() const;
    Point accumulatedRenderOffset() const;
    Point accumulatedRenderScale() const;
    Rect visualBounds() const;

    // Blending Mode (Zero struct size overhead)
    BlendMode blendMode() const;
    void setBlendMode(BlendMode mode);
    UIElement& blendMode(BlendMode mode);

    // Bounds & Geometry
    const Rect& bounds() const { return m_bounds; }
    void setBounds(const Rect& bounds);
    Point position() const { return { m_bounds.x, m_bounds.y }; }
    void setPosition(const Point& pos) { setBounds({ pos.x, pos.y, m_bounds.width, m_bounds.height }); }
    Dimensions size() const { return { m_bounds.width, m_bounds.height }; }
    void setSize(const Dimensions& size) { setBounds({ m_bounds.x, m_bounds.y, size.width, size.height }); }

    bool isAbsolute() const { return m_isAbsolute; }
    void setAbsolute(bool abs) { m_isAbsolute = abs; }

    const Dimensions& desiredSize() const { return m_desiredSize; }
    void setDesiredSize(Dimensions size) { m_desiredSize = size; }

    // Layout
    LayoutProps& layout() { return m_layout; }
    const LayoutProps& layout() const { return m_layout; }

    // Styling System
    WidgetStyle& style() { return m_style; }
    const WidgetStyle& style() const { return m_style; }
    UIElement& style(WidgetStyle s) { m_style = std::move(s); return *this; }

    BoxStyle resolveBoxStyle(WidgetState state, const BoxStyle& fallback = BoxStyle{}) const;
    TextStyle resolveTextStyle(WidgetState state, const TextStyle& fallback = TextStyle{}) const;

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

    float m_opacity{ 1.0f };
    std::shared_ptr<bool> m_aliveToken{ std::make_shared<bool>(true) };
    std::unique_ptr<ElementAnimator> m_animator;
    std::unique_ptr<ElementSpring> m_spring;

    Rect m_bounds{};
    Dimensions m_desiredSize{};
    LayoutProps m_layout{};

    bool m_visible{ true };
    bool m_enabled{ true };
    bool m_focusable{ false };
    bool m_hovered{ false };
    bool m_pressed{ false };

    bool m_isAbsolute{ false };
    bool m_layoutDirty{ true };
    WidgetStyle m_style{};
    std::string m_tooltip;
    std::vector<std::string> m_classes;
    std::function<bool(const Point&)> m_onContextMenu;

    std::string m_typeName;
    ComputedStyle m_computedStyle{};
    std::unordered_map<std::string, std::string> m_inlineAttrs;
    bool m_styleDirty{ true };
    bool m_childStyleDirty{ false };
    std::unique_ptr<StyleTransitionController> m_transitions;
    bool m_styleInitialized{ false };

    std::optional<Color> m_outlineColor;
    std::optional<float> m_outlineWidth;
    std::optional<float> m_outlineOffset;
};

} // namespace PerfUI
