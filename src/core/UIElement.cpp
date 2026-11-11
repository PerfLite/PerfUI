#include "PerfUI/UIElement.h"
#include "PerfUI/UIContext.h"
#include "PerfUI/LayoutEngine.h"
#include "PerfUI/Animation.h"
#include "PerfUI/Style/StyleSheet.h"
#include <atomic>
#include <algorithm>
#include <mutex>
#include <unordered_map>

namespace {
    struct VisualTransformData {
        PerfUI::Point offset{ 0.0f, 0.0f };
        PerfUI::Point offsetPercent{ 0.0f, 0.0f };
        bool hasOffsetPercent{ false };
        PerfUI::Point scale{ 1.0f, 1.0f };
        PerfUI::Point pivot{ 0.5f, 0.5f };
        float opacityMultiplier{ 1.0f };
        PerfUI::BlendMode blendMode{ PerfUI::BlendMode::Normal };
    };

    static std::mutex s_xfMutex;
    static std::unordered_map<const PerfUI::UIElement*, VisualTransformData> s_xfMap;
}

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

UIElement::~UIElement() {
    {
        std::lock_guard<std::mutex> lock(s_xfMutex);
        s_xfMap.erase(this);
    }
    m_layout.clearAbsoluteAnchors();
    if (m_aliveToken) {
        *m_aliveToken = false;
    }
    stopAnimations();
    if (m_context) {
        m_context->notifyElementDestroyed(this);
    }
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
    if (m_enabled != enabled) {
        m_enabled = enabled;
        markStyleDirty();
    }
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

void UIElement::setOpacity(float opacity) {
    m_opacity = (std::clamp)(opacity, 0.0f, 1.0f);
}

Point UIElement::renderOffset() const {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    auto it = s_xfMap.find(this);
    if (it == s_xfMap.end()) return Point{ 0.0f, 0.0f };
    Point pt = it->second.offset;
    if (it->second.hasOffsetPercent) {
        pt.x += m_bounds.width * (it->second.offsetPercent.x / 100.0f);
        pt.y += m_bounds.height * (it->second.offsetPercent.y / 100.0f);
    }
    return pt;
}

Point UIElement::renderOffsetPercent() const {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    auto it = s_xfMap.find(this);
    return it != s_xfMap.end() ? it->second.offsetPercent : Point{ 0.0f, 0.0f };
}

void UIElement::setRenderOffsetPercent(const Point& percent) {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    s_xfMap[this].offsetPercent = percent;
    s_xfMap[this].hasOffsetPercent = (percent.x != 0.0f || percent.y != 0.0f);
}

void UIElement::setRenderOffset(const Point& offset) {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    if (offset.x == 0.0f && offset.y == 0.0f) {
        auto it = s_xfMap.find(this);
        if (it != s_xfMap.end()) {
            it->second.offset = offset;
        }
    } else {
        s_xfMap[this].offset = offset;
    }
}

void UIElement::setRenderOffset(const Point& offset, const Point& offsetPercent) {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    s_xfMap[this].offset = offset;
    s_xfMap[this].offsetPercent = offsetPercent;
    s_xfMap[this].hasOffsetPercent = (offsetPercent.x != 0.0f || offsetPercent.y != 0.0f);
}

UIElement& UIElement::renderOffset(Point offset) {
    setRenderOffset(offset);
    return *this;
}

Point UIElement::renderScale() const {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    auto it = s_xfMap.find(this);
    return it != s_xfMap.end() ? it->second.scale : Point{ 1.0f, 1.0f };
}

void UIElement::setRenderScale(const Point& scale) {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    s_xfMap[this].scale = scale;
}

void UIElement::setRenderScale(float uniformScale) {
    setRenderScale(Point{ uniformScale, uniformScale });
}

UIElement& UIElement::renderScale(Point scale) {
    setRenderScale(scale);
    return *this;
}

UIElement& UIElement::renderScale(float uniformScale) {
    setRenderScale(uniformScale);
    return *this;
}

Point UIElement::pivot() const {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    auto it = s_xfMap.find(this);
    return it != s_xfMap.end() ? it->second.pivot : Point{ 0.5f, 0.5f };
}

void UIElement::setPivot(const Point& pivot) {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    s_xfMap[this].pivot = pivot;
}

UIElement& UIElement::pivot(Point pivot) {
    setPivot(pivot);
    return *this;
}

float UIElement::renderOpacityMultiplier() const {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    auto it = s_xfMap.find(this);
    return it != s_xfMap.end() ? it->second.opacityMultiplier : 1.0f;
}

void UIElement::setRenderOpacityMultiplier(float mult) {
    float clamped = (std::clamp)(mult, 0.0f, 1.0f);
    std::lock_guard<std::mutex> lock(s_xfMutex);
    s_xfMap[this].opacityMultiplier = clamped;
}

UIElement& UIElement::renderOpacityMultiplier(float mult) {
    setRenderOpacityMultiplier(mult);
    return *this;
}

bool UIElement::hasVisualTransform() const {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    auto it = s_xfMap.find(this);
    if (it == s_xfMap.end()) return false;
    return it->second.offset.x != 0.0f || it->second.offset.y != 0.0f ||
           (it->second.hasOffsetPercent && (it->second.offsetPercent.x != 0.0f || it->second.offsetPercent.y != 0.0f)) ||
           it->second.scale.x != 1.0f || it->second.scale.y != 1.0f ||
           it->second.pivot.x != 0.5f || it->second.pivot.y != 0.5f;
}

Point UIElement::visualPosition() const {
    Point off = accumulatedRenderOffset();
    return { m_bounds.x + off.x, m_bounds.y + off.y };
}

Point UIElement::accumulatedRenderOffset() const {
    Point pt = renderOffset();
    const UIElement* p = m_parent;
    while (p) {
        Point pOff = p->renderOffset();
        pt.x += pOff.x;
        pt.y += pOff.y;
        p = p->m_parent;
    }
    return pt;
}

Point UIElement::accumulatedRenderScale() const {
    Point s = renderScale();
    const UIElement* p = m_parent;
    while (p) {
        Point pScale = p->renderScale();
        s.x *= pScale.x;
        s.y *= pScale.y;
        p = p->m_parent;
    }
    return s;
}

Rect UIElement::visualBounds() const {
    Point offset = accumulatedRenderOffset();
    Point scale = accumulatedRenderScale();
    Point piv = pivot();

    float w = m_bounds.width * scale.x;
    float h = m_bounds.height * scale.y;
    float vx = m_bounds.x + offset.x;
    float vy = m_bounds.y + offset.y;

    if (scale.x != 1.0f || scale.y != 1.0f) {
        float px = (m_bounds.x + offset.x) + m_bounds.width * piv.x;
        float py = (m_bounds.y + offset.y) + m_bounds.height * piv.y;
        vx = px - w * piv.x;
        vy = py - h * piv.y;
    }

    return Rect{ vx, vy, w, h };
}

BlendMode UIElement::blendMode() const {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    auto it = s_xfMap.find(this);
    return it != s_xfMap.end() ? it->second.blendMode : BlendMode::Normal;
}

void UIElement::setBlendMode(BlendMode mode) {
    std::lock_guard<std::mutex> lock(s_xfMutex);
    s_xfMap[this].blendMode = mode;
}

UIElement& UIElement::blendMode(BlendMode mode) {
    setBlendMode(mode);
    return *this;
}

float UIElement::effectiveOpacity() const {
    float eff = (std::clamp)(m_opacity, 0.0f, 1.0f) * (std::clamp)(renderOpacityMultiplier(), 0.0f, 1.0f);
    const UIElement* p = m_parent;
    while (p) {
        eff *= (std::clamp)(p->m_opacity, 0.0f, 1.0f) * (std::clamp)(p->renderOpacityMultiplier(), 0.0f, 1.0f);
        p = p->m_parent;
    }
    return eff;
}

void UIElement::notifyAnimStarted(AnimProp prop) {
    if (m_spring) {
        switch (prop) {
        case AnimProp::Position: m_spring->cancelPosition(); break;
        case AnimProp::Scale:    m_spring->cancelScale(); break;
        case AnimProp::Size:     m_spring->cancelSize(); break;
        case AnimProp::Alpha:    m_spring->cancelAlpha(); break;
        case AnimProp::Color:    m_spring->cancelColor(); break;
        }
    }
}

void UIElement::notifySpringStarted(AnimProp prop) {
    if (m_animator) {
        switch (prop) {
        case AnimProp::Position: m_animator->cancelPosition(); break;
        case AnimProp::Scale:    m_animator->cancelScale(); break;
        case AnimProp::Size:     m_animator->cancelSize(); break;
        case AnimProp::Alpha:    m_animator->cancelAlpha(); break;
        case AnimProp::Color:    m_animator->cancelColor(); break;
        }
    }
}

ElementAnimator& UIElement::animate() {
    if (!m_animator) {
        m_animator = std::make_unique<ElementAnimator>(this);
    }
    return *m_animator;
}

ElementSpring& UIElement::spring() {
    if (!m_spring) {
        m_spring = std::make_unique<ElementSpring>(this);
    }
    return *m_spring;
}

void UIElement::stopAnimations() {
    if (m_animator) {
        m_animator->stop();
    }
    if (m_spring) {
        m_spring->stop();
    }
}

bool UIElement::isAnimating() const {
    return (m_animator && !m_animator->isFinished()) ||
           (m_spring && !m_spring->isResting());
}

WeakElementRef UIElement::weakRef() {
    return WeakElementRef(this);
}

void UIElement::update(float deltaTime) {
    if (!m_visible) return;

    if (m_styleDirty) {
        updateStyleCascade();
    }

    if (m_context && m_context->animationScheduler().isReducedMotion()) {
        if (m_animator && !m_animator->isFinished()) {
            m_animator->finish();
        }
        if (m_spring && !m_spring->isResting()) {
            m_spring->finish();
        }
    } else {
        float effectiveDt = deltaTime;
        if (m_context) {
            if (m_context->animationScheduler().isPaused()) {
                effectiveDt = 0.0f;
            } else {
                effectiveDt *= m_context->animationScheduler().timeScale();
            }
        }
        if (effectiveDt > 0.0f) {
            if (m_animator && !m_animator->isFinished()) {
                m_animator->update(effectiveDt);
            }
            if (m_spring && !m_spring->isResting()) {
                m_spring->update(effectiveDt);
            }
        }
    }

    for (auto& child : m_children) {
        if (child) {
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

    bool hasBlend = blendMode() != BlendMode::Normal;
    if (hasBlend) {
        backend.pushBlendMode(blendMode());
    }

    for (auto& child : m_children) {
        if (child && child->isVisible()) {
            bool hasXf = child->hasVisualTransform();
            if (hasXf) {
                backend.pushTransform(child->renderOffset(), child->renderScale(), child->pivot(), child->bounds());
            }
            child->render(backend);
            if (hasXf) {
                backend.popTransform();
            }
        }
    }

    if (hasBlend) {
        backend.popBlendMode();
    }
}

UIElement* UIElement::hitTest(const Point& point) {
    if (!m_visible || !visualBounds().contains(point)) {
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
    if (!m_pressed) {
        m_pressed = true;
        markStyleDirty();
    }
    return false;
}

bool UIElement::onPointerUp(const Point& localPoint) {
    (void)localPoint;
    if (m_pressed) {
        m_pressed = false;
        markStyleDirty();
    }
    return false;
}

void UIElement::onPointerEnter() {
    if (!m_hovered) {
        m_hovered = true;
        markStyleDirty();
    }
}

void UIElement::onPointerLeave() {
    if (m_hovered) {
        m_hovered = false;
        markStyleDirty();
    }
}

bool UIElement::onMouseWheel(float delta, const Point& localPoint) {
    (void)delta;
    (void)localPoint;
    return false;
}

void UIElement::onFocusChanged(bool focused) {
    (void)focused;
    markStyleDirty();
}

bool UIElement::onAction(NavDirection dir) {
    (void)dir;
    return false;
}

bool UIElement::onContextMenu(const Point& localPoint) {
    if (m_onContextMenu) {
        return m_onContextMenu(localPoint);
    }
    return false;
}

BoxStyle UIElement::resolveBoxStyle(WidgetState state, const BoxStyle& fallback) const {
    return m_style.resolveBox(state, fallback);
}

TextStyle UIElement::resolveTextStyle(WidgetState state, const TextStyle& fallback) const {
    return m_style.resolveText(state, fallback);
}

std::string_view UIElement::typeName() const {
    return m_typeName.empty() ? "Element" : std::string_view(m_typeName);
}

void UIElement::setTypeName(std::string name) {
    m_typeName = std::move(name);
}


uint8_t UIElement::activePseudoClasses() const {
    uint8_t mask = 0;
    if (m_hovered) mask |= static_cast<uint8_t>(PseudoClass::Hover);
    if (m_pressed) mask |= static_cast<uint8_t>(PseudoClass::Active);
    if (isFocused()) mask |= static_cast<uint8_t>(PseudoClass::Focus);
    if (!m_enabled) mask |= static_cast<uint8_t>(PseudoClass::Disabled);
    if (isChecked()) mask |= static_cast<uint8_t>(PseudoClass::Checked);

    if (m_parent) {
        const auto& siblings = m_parent->children();
        if (!siblings.empty()) {
            if (siblings.front().get() == this) mask |= static_cast<uint8_t>(PseudoClass::FirstChild);
            if (siblings.back().get() == this) mask |= static_cast<uint8_t>(PseudoClass::LastChild);
        }
    }
    return mask;
}

void UIElement::markStyleDirty() {
    m_styleDirty = true;
    for (UIElement* p = m_parent; p != nullptr; p = p->m_parent) {
        if (p->m_childStyleDirty) break;
        p->m_childStyleDirty = true;
    }
}

void UIElement::invalidateStyleSubtree() {
    m_styleDirty = true;
    m_childStyleDirty = !m_children.empty();
    for (auto& child : m_children) {
        if (child) child->invalidateStyleSubtree();
    }
}

void UIElement::updateStyleCascade() {
    bool selfDirty = m_styleDirty;
    if (m_context && selfDirty) {
        const auto& sheet = m_context->styleSheet();
        const ComputedStyle* parentStyle = m_parent ? &m_parent->computedStyle() : nullptr;
        ComputedStyle newStyle = sheet.computeStyle(this, parentStyle);

        if (m_computedStyle.hasLayoutDifferences(newStyle)) {
            markLayoutDirty();
        }

        m_computedStyle = newStyle;
        applyComputedStyle(m_computedStyle);
        m_styleDirty = false;
    }

    if (m_childStyleDirty || selfDirty) {
        m_childStyleDirty = false;
        for (auto& child : m_children) {
            if (child) {
                if (selfDirty) {
                    child->markStyleDirty();
                }
                if (child->isStyleDirty() || child->hasChildStyleDirty()) {
                    child->updateStyleCascade();
                }
            }
        }
    }
}

void UIElement::applyComputedStyle(const ComputedStyle& c) {
    bool animAlpha = m_animator && m_animator->trackAlpha().active;
    bool animScale = m_animator && m_animator->trackScale().active;
    bool animPos = m_animator && m_animator->trackPosition().active;

    if (c.visible.has_value()) setVisible(*c.visible);
    if (c.opacity.has_value() && !animAlpha) setOpacity(*c.opacity);
    if (c.blendMode.has_value()) setBlendMode(*c.blendMode);
    if (c.transformScale.has_value() && !animScale) setRenderScale(*c.transformScale);
    if (c.transformPivot.has_value()) setPivot(*c.transformPivot);
    if ((c.transformOffsetPx.has_value() || c.transformOffsetPct.has_value()) && !animPos) {
        setRenderOffset(c.transformOffsetPx.value_or(Point{0, 0}), c.transformOffsetPct.value_or(Point{0, 0}));
    }

    auto& props = layout();
    if (c.width.has_value()) props.width(DimensionConstraint::Fixed(*c.width));
    if (c.height.has_value()) props.height(DimensionConstraint::Fixed(*c.height));
    if (c.minWidth.has_value()) props.minWidth(*c.minWidth);
    if (c.maxWidth.has_value()) props.maxWidth(*c.maxWidth);
    if (c.minHeight.has_value()) props.minHeight(*c.minHeight);
    if (c.maxHeight.has_value()) props.maxHeight(*c.maxHeight);
    if (c.flex.has_value()) props.flex(*c.flex);
    if (c.direction.has_value()) props.direction(*c.direction);
    if (c.alignment.has_value()) props.alignment(*c.alignment);
    if (c.justify.has_value()) props.justify(*c.justify);
    if (c.gap.has_value()) props.gap(*c.gap);
    if (c.padding.has_value()) props.padding(c.padding->left, c.padding->top, c.padding->right, c.padding->bottom);
    if (c.margin.has_value()) props.margin(c.margin->left, c.margin->top, c.margin->right, c.margin->bottom);
    if (c.isAbsolute.has_value()) setAbsolute(*c.isAbsolute);
    if (c.left.has_value()) props.left(*c.left);
    if (c.right.has_value()) props.right(*c.right);
    if (c.top.has_value()) props.top(*c.top);
    if (c.bottom.has_value()) props.bottom(*c.bottom);
    if (c.centerX.has_value()) props.centerX(*c.centerX);

    if (c.backgroundColor.has_value()) {
        m_style.box.normal = m_style.box.normal.value_or(BoxStyle{}).bg(*c.backgroundColor);
    }
    if (c.borderColor.has_value() || c.borderWidth.has_value()) {
        Color bc = c.borderColor.value_or(Color::White());
        float bw = c.borderWidth.value_or(1.0f);
        m_style.box.normal = m_style.box.normal.value_or(BoxStyle{}).border(bc, bw);
    }
    if (c.cornerRadius.has_value()) {
        m_style.box.normal = m_style.box.normal.value_or(BoxStyle{}).radius(*c.cornerRadius);
    }
    if (c.shadow.has_value() && *c.shadow) {
        m_style.box.normal = m_style.box.normal.value_or(BoxStyle{}).dropShadow(Color(0, 0, 0, 180));
    }
    if (c.color.has_value()) {
        m_style.text.normal = m_style.text.normal.value_or(TextStyleOverride{}).col(*c.color);
    }
    if (c.fontSize.has_value()) {
        m_style.text.normal = m_style.text.normal.value_or(TextStyleOverride{}).size(*c.fontSize);
    }
    if (c.bold.has_value()) {
        m_style.text.normal = m_style.text.normal.value_or(TextStyleOverride{}).setBold(*c.bold);
    }
    if (c.italic.has_value()) {
        m_style.text.normal = m_style.text.normal.value_or(TextStyleOverride{}).setItalic(*c.italic);
    }
    if (c.wrap.has_value()) {
        m_style.text.normal = m_style.text.normal.value_or(TextStyleOverride{}).wrap(*c.wrap);
    }

    onStyleApplied(c);
}

} // namespace PerfUI
