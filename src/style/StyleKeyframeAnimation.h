#pragma once

#include "PerfUI/Types.h"
#include "PerfUI/Animation.h"
#include "PerfUI/Theme.h"
#include "PerfUI/Style/StyleTypes.h"
#include "PerfUI/Style/StyleSheet.h"
#include "PerfUI/UIElement.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>

namespace PerfUI {

namespace detail {

template<typename T>
inline T LerpValue(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}

template<>
inline Color LerpValue<Color>(const Color& a, const Color& b, float t) {
    if (a == b) return a;
    if (a.a == 0 && b.a == 0) return Color{ 0, 0, 0, 0 };
    if (a.a == 0) {
        return Color(b.r, b.g, b.b, static_cast<uint8_t>(b.a * t + 0.5f));
    }
    if (b.a == 0) {
        return Color(a.r, a.g, a.b, static_cast<uint8_t>(a.a * (1.0f - t) + 0.5f));
    }
    // Premultiplied alpha Lerp to avoid dark fringes:
    float af = a.a / 255.0f;
    float bf = b.a / 255.0f;
    float outA = af + (bf - af) * t;
    if (outA <= 0.0001f) {
        return Color{ 0, 0, 0, 0 };
    }
    float outR = (a.r * af * (1.0f - t) + b.r * bf * t) / outA;
    float outG = (a.g * af * (1.0f - t) + b.g * bf * t) / outA;
    float outB = (a.b * af * (1.0f - t) + b.b * bf * t) / outA;
    return Color(
        static_cast<uint8_t>((std::clamp)(outR + 0.5f, 0.0f, 255.0f)),
        static_cast<uint8_t>((std::clamp)(outG + 0.5f, 0.0f, 255.0f)),
        static_cast<uint8_t>((std::clamp)(outB + 0.5f, 0.0f, 255.0f)),
        static_cast<uint8_t>((std::clamp)(outA * 255.0f + 0.5f, 0.0f, 255.0f))
    );
}

template<>
inline Point LerpValue<Point>(const Point& a, const Point& b, float t) {
    return Point{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

} // namespace detail

struct ElementBaseStyleSnapshot {
    float opacity{ 1.0f };
    Point transformScale{ 1.0f, 1.0f };
    Point transformOffsetPx{ 0.0f, 0.0f };
    Point transformOffsetPct{ 0.0f, 0.0f };
    std::optional<Color> backgroundColor;
    std::optional<Color> borderColor;
    std::optional<float> borderWidth;
    std::optional<float> cornerRadius;
    std::optional<Color> outlineColor;
    float outlineWidth{ 0.0f };
    float outlineOffset{ 0.0f };
    std::optional<Color> color;
    std::optional<float> fontSize;
    DimensionConstraint width{ DimensionConstraint::Auto() };
    DimensionConstraint height{ DimensionConstraint::Auto() };
    uint32_t cascadeMask{ 0 };
    bool captured{ false };

    enum CascadePropBits : uint32_t {
        CascadeOpacity          = 1 << 0,
        CascadeTransformScale   = 1 << 1,
        CascadeTransformOffset  = 1 << 2,
        CascadeBackgroundColor  = 1 << 3,
        CascadeBorderColor      = 1 << 4,
        CascadeBorderWidth      = 1 << 5,
        CascadeCornerRadius     = 1 << 6,
        CascadeOutlineColor     = 1 << 7,
        CascadeOutlineWidth     = 1 << 8,
        CascadeOutlineOffset    = 1 << 9,
        CascadeColor            = 1 << 10,
        CascadeFontSize         = 1 << 11,
        CascadeWidth            = 1 << 12,
        CascadeHeight           = 1 << 13,
    };

    void capture(const UIElement* el) {
        if (!el) return;
        opacity = el->opacity();
        transformScale = el->renderScale();
        transformOffsetPx = el->renderOffset();
        transformOffsetPct = el->renderOffsetPercent();
        if (el->style().box.normal.has_value()) {
            backgroundColor = el->style().box.normal->backgroundColor;
            borderColor = el->style().box.normal->borderColor;
            borderWidth = el->style().box.normal->borderWidth;
            cornerRadius = el->style().box.normal->cornerRadius;
        }
        outlineColor = el->outlineColor();
        outlineWidth = el->outlineWidth().value_or(0.0f);
        outlineOffset = el->outlineOffset().value_or(0.0f);
        if (el->style().text.normal.has_value()) {
            color = el->style().text.normal->color;
            fontSize = el->style().text.normal->fontSize;
        }
        width = el->layout().width();
        height = el->layout().height();
        captured = true;
    }

    void updateFromComputed(const ComputedStyle& c, const UIElement* el) {
        if (!captured) {
            capture(el);
        }

        if (c.opacity.has_value()) {
            opacity = *c.opacity;
            cascadeMask |= CascadeOpacity;
        } else if (cascadeMask & CascadeOpacity) {
            opacity = 1.0f;
            cascadeMask &= ~CascadeOpacity;
        }

        if (c.transformScale.has_value()) {
            transformScale = *c.transformScale;
            cascadeMask |= CascadeTransformScale;
        } else if (cascadeMask & CascadeTransformScale) {
            transformScale = Point{ 1.0f, 1.0f };
            cascadeMask &= ~CascadeTransformScale;
        }

        if (c.transformOffsetPx.has_value() || c.transformOffsetPct.has_value()) {
            transformOffsetPx = c.transformOffsetPx.value_or(Point{ 0.0f, 0.0f });
            transformOffsetPct = c.transformOffsetPct.value_or(Point{ 0.0f, 0.0f });
            cascadeMask |= CascadeTransformOffset;
        } else if (cascadeMask & CascadeTransformOffset) {
            transformOffsetPx = Point{ 0.0f, 0.0f };
            transformOffsetPct = Point{ 0.0f, 0.0f };
            cascadeMask &= ~CascadeTransformOffset;
        }

        if (c.backgroundColor.has_value()) {
            backgroundColor = *c.backgroundColor;
            cascadeMask |= CascadeBackgroundColor;
        } else if (cascadeMask & CascadeBackgroundColor) {
            backgroundColor = std::nullopt;
            cascadeMask &= ~CascadeBackgroundColor;
        }

        if (c.borderColor.has_value()) {
            borderColor = *c.borderColor;
            cascadeMask |= CascadeBorderColor;
        } else if (cascadeMask & CascadeBorderColor) {
            borderColor = std::nullopt;
            cascadeMask &= ~CascadeBorderColor;
        }

        if (c.borderWidth.has_value()) {
            borderWidth = *c.borderWidth;
            cascadeMask |= CascadeBorderWidth;
        } else if (cascadeMask & CascadeBorderWidth) {
            borderWidth = std::nullopt;
            cascadeMask &= ~CascadeBorderWidth;
        }

        if (c.cornerRadius.has_value()) {
            cornerRadius = *c.cornerRadius;
            cascadeMask |= CascadeCornerRadius;
        } else if (cascadeMask & CascadeCornerRadius) {
            cornerRadius = std::nullopt;
            cascadeMask &= ~CascadeCornerRadius;
        }

        if (c.outlineColor.has_value()) {
            outlineColor = *c.outlineColor;
            cascadeMask |= CascadeOutlineColor;
        } else if (cascadeMask & CascadeOutlineColor) {
            outlineColor = std::nullopt;
            cascadeMask &= ~CascadeOutlineColor;
        }

        if (c.outlineWidth.has_value()) {
            outlineWidth = *c.outlineWidth;
            cascadeMask |= CascadeOutlineWidth;
        } else if (cascadeMask & CascadeOutlineWidth) {
            outlineWidth = 0.0f;
            cascadeMask &= ~CascadeOutlineWidth;
        }

        if (c.outlineOffset.has_value()) {
            outlineOffset = *c.outlineOffset;
            cascadeMask |= CascadeOutlineOffset;
        } else if (cascadeMask & CascadeOutlineOffset) {
            outlineOffset = 0.0f;
            cascadeMask &= ~CascadeOutlineOffset;
        }

        if (c.color.has_value()) {
            color = *c.color;
            cascadeMask |= CascadeColor;
        } else if (cascadeMask & CascadeColor) {
            color = std::nullopt;
            cascadeMask &= ~CascadeColor;
        }

        if (c.fontSize.has_value()) {
            fontSize = *c.fontSize;
            cascadeMask |= CascadeFontSize;
        } else if (cascadeMask & CascadeFontSize) {
            fontSize = std::nullopt;
            cascadeMask &= ~CascadeFontSize;
        }

        if (c.widthConstraint.has_value()) {
            width = *c.widthConstraint;
            cascadeMask |= CascadeWidth;
        } else if (c.width.has_value()) {
            width = DimensionConstraint::Fixed(*c.width);
            cascadeMask |= CascadeWidth;
        } else if (cascadeMask & CascadeWidth) {
            width = DimensionConstraint::Auto();
            cascadeMask &= ~CascadeWidth;
        }

        if (c.heightConstraint.has_value()) {
            height = *c.heightConstraint;
            cascadeMask |= CascadeHeight;
        } else if (c.height.has_value()) {
            height = DimensionConstraint::Fixed(*c.height);
            cascadeMask |= CascadeHeight;
        } else if (cascadeMask & CascadeHeight) {
            height = DimensionConstraint::Auto();
            cascadeMask &= ~CascadeHeight;
        }

        captured = true;
    }

    void restoreProperties(uint16_t propsMask, UIElement* el) const {
        if (!el || !captured || propsMask == 0) return;

        if ((propsMask & (1 << 0)) && !el->isAlphaAnimating()) {
            el->setOpacity(opacity);
        }
        if ((propsMask & (1 << 1)) && !el->isScaleAnimating()) {
            el->setRenderScale(transformScale);
        }
        if ((propsMask & (1 << 2)) && !el->isPositionAnimating()) {
            el->setRenderOffset(transformOffsetPx, transformOffsetPct);
        }

        bool restoreBox = false;
        BoxStyle box = el->style().box.normal.value_or(BoxStyle{});
        if (propsMask & (1 << 3)) {
            box.backgroundColor = backgroundColor;
            restoreBox = true;
        }
        if (propsMask & (1 << 4)) {
            box.borderColor = borderColor;
            restoreBox = true;
        }
        if (propsMask & (1 << 5)) {
            box.borderWidth = borderWidth;
            restoreBox = true;
        }
        if (propsMask & (1 << 6)) {
            box.cornerRadius = cornerRadius;
            restoreBox = true;
        }
        if (restoreBox) {
            if (!box.backgroundColor.has_value() && !box.borderColor.has_value() &&
                !box.borderWidth.has_value() && !box.cornerRadius.has_value() &&
                !box.shadow.has_value() && !box.opacity.has_value() &&
                !box.liquidGlass.has_value() && box.backgroundTexture == 0) {
                el->style().box.normal.reset();
            } else {
                el->style().box.normal = box;
            }
        }

        if (propsMask & (1 << 7)) {
            if (outlineColor.has_value()) el->setOutlineColor(*outlineColor);
            else el->setOutlineColor(Color{ 0, 0, 0, 0 });
            el->setOutlineWidth(outlineWidth);
            el->setOutlineOffset(outlineOffset);
        }

        if (propsMask & (1 << 8)) {
            if (el->style().text.normal.has_value()) {
                el->style().text.normal->color = color;
            } else if (color.has_value()) {
                TextStyleOverride text;
                text.color = color;
                el->style().text.normal = text;
            }
        }

        if (propsMask & (1 << 11)) {
            if (el->style().text.normal.has_value()) {
                el->style().text.normal->fontSize = fontSize;
            } else if (fontSize.has_value()) {
                TextStyleOverride text;
                text.fontSize = fontSize;
                el->style().text.normal = text;
            }
        }

        if (el->style().text.normal.has_value()) {
            const auto& t = *el->style().text.normal;
            if (!t.color.has_value() && !t.fontSize.has_value() && !t.bold.has_value() &&
                !t.italic.has_value() && !t.wrapWidth.has_value() && !t.shadowColor.has_value() &&
                !t.shadowOffset.has_value()) {
                el->style().text.normal.reset();
            }
        }

        bool dirtyLayout = false;
        if (propsMask & (1 << 9)) {
            el->layout().width(width);
            dirtyLayout = true;
        }
        if (propsMask & (1 << 10)) {
            el->layout().height(height);
            dirtyLayout = true;
        }
        if (dirtyLayout) {
            el->markLayoutDirty();
        }
    }

    void restoreAll(UIElement* el) const {
        restoreProperties(0xFFFF, el);
    }
};

template<typename T>
struct PropertyKeyframe {
    float offset{ 0.0f };
    T value{};
    std::optional<EasingDef> timingFunction;
};

template<typename T>
struct PropertyTrack {
    std::vector<PropertyKeyframe<T>> stops;
    bool active{ false };
    bool hasImplicitStart{ false };
    bool hasImplicitEnd{ false };

    template<typename Getter>
    void buildFromSteps(const std::vector<KeyframeStep>& steps, const T& baseValue, Getter getter) {
        stops.clear();
        active = false;
        hasImplicitStart = false;
        hasImplicitEnd = false;

        for (const auto& step : steps) {
            auto valOpt = getter(step);
            if (valOpt.has_value()) {
                active = true;
                stops.push_back({ step.offset, *valOpt, step.timingFunction });
            }
        }

        if (!active || stops.empty()) {
            active = false;
            return;
        }

        // Implicit start keyframe (offset == 0.0f)
        if (stops.front().offset > 0.0001f) {
            std::optional<EasingDef> firstTiming;
            if (!steps.empty() && steps.front().offset < 0.0001f) {
                firstTiming = steps.front().timingFunction;
            }
            stops.insert(stops.begin(), { 0.0f, baseValue, firstTiming });
            hasImplicitStart = true;
        }

        // Implicit end keyframe (offset == 1.0f)
        if (stops.back().offset < 0.9999f) {
            stops.push_back({ 1.0f, baseValue, std::nullopt });
            hasImplicitEnd = true;
        }
    }

    void updateImplicitStops(const T& baseValue) {
        if (!active) return;
        if (hasImplicitStart && !stops.empty() && stops.front().offset <= 0.0001f) {
            stops.front().value = baseValue;
        }
        if (hasImplicitEnd && !stops.empty() && stops.back().offset >= 0.9999f) {
            stops.back().value = baseValue;
        }
    }

    bool evaluate(float progress, const EasingDef& defaultEasing, T& outValue) const {
        if (!active || stops.empty()) return false;
        if (stops.size() == 1) {
            outValue = stops[0].value;
            return true;
        }

        float p = (std::clamp)(progress, 0.0f, 1.0f);

        size_t idx = 0;
        for (size_t i = 0; i + 1 < stops.size(); ++i) {
            if (p >= stops[i].offset && p <= stops[i + 1].offset) {
                idx = i;
                break;
            }
            if (p > stops[i + 1].offset) {
                idx = i;
            }
        }

        const auto& k0 = stops[idx];
        const auto& k1 = stops[(std::min)(idx + 1, stops.size() - 1)];

        float span = k1.offset - k0.offset;
        float segT = (span > 0.0001f) ? ((p - k0.offset) / span) : 0.0f;
        segT = (std::clamp)(segT, 0.0f, 1.0f);

        const EasingDef& segEasing = k0.timingFunction.has_value() ? *k0.timingFunction : defaultEasing;
        float easedT = ApplyEasing(segT, segEasing);
        easedT = (std::clamp)(easedT, 0.0f, 1.0f);

        outValue = detail::LerpValue(k0.value, k1.value, easedT);
        return true;
    }
};

struct AnimationTracks {
    PropertyTrack<float> opacity;
    PropertyTrack<Point> transformScale;
    PropertyTrack<Point> transformOffsetPx;
    PropertyTrack<Point> transformOffsetPct;
    PropertyTrack<Color> backgroundColor;
    PropertyTrack<Color> borderColor;
    PropertyTrack<float> borderWidth;
    PropertyTrack<float> cornerRadius;
    PropertyTrack<Color> outlineColor;
    PropertyTrack<float> outlineWidth;
    PropertyTrack<float> outlineOffset;
    PropertyTrack<Color> color;
    PropertyTrack<float> fontSize;
    PropertyTrack<float> width;
    PropertyTrack<float> height;

    void build(const KeyframesDefinition& def, const ElementBaseStyleSnapshot& base, const UIElement* el) {
        opacity.buildFromSteps(def.steps, base.opacity, [](const KeyframeStep& s) { return s.opacity; });
        transformScale.buildFromSteps(def.steps, base.transformScale, [](const KeyframeStep& s) { return s.transformScale; });
        transformOffsetPx.buildFromSteps(def.steps, base.transformOffsetPx, [](const KeyframeStep& s) { return s.transformOffsetPx; });
        transformOffsetPct.buildFromSteps(def.steps, base.transformOffsetPct, [](const KeyframeStep& s) { return s.transformOffsetPct; });

        // Background color: if base has none, use target color with alpha 0
        Color defaultBg = Color{ 0, 0, 0, 0 };
        for (const auto& s : def.steps) {
            if (s.backgroundColor.has_value()) {
                defaultBg = Color(s.backgroundColor->r, s.backgroundColor->g, s.backgroundColor->b, 0);
                break;
            }
        }
        backgroundColor.buildFromSteps(def.steps, base.backgroundColor.value_or(defaultBg), [](const KeyframeStep& s) { return s.backgroundColor; });

        // Border color: if base has none, use target color with alpha 0
        Color defaultBc = Color{ 0, 0, 0, 0 };
        for (const auto& s : def.steps) {
            if (s.borderColor.has_value()) {
                defaultBc = Color(s.borderColor->r, s.borderColor->g, s.borderColor->b, 0);
                break;
            }
        }
        borderColor.buildFromSteps(def.steps, base.borderColor.value_or(defaultBc), [](const KeyframeStep& s) { return s.borderColor; });

        // Border width: if base has none, base width is 0.0f
        float baseBw = base.borderWidth.value_or(0.0f);
        borderWidth.buildFromSteps(def.steps, baseBw, [](const KeyframeStep& s) { return s.borderWidth; });

        cornerRadius.buildFromSteps(def.steps, base.cornerRadius.value_or(0.0f), [](const KeyframeStep& s) { return s.cornerRadius; });

        // Outline color: if base has none, use target color with alpha 0
        Color defaultOc = Color{ 0, 0, 0, 0 };
        for (const auto& s : def.steps) {
            if (s.outlineColor.has_value()) {
                defaultOc = Color(s.outlineColor->r, s.outlineColor->g, s.outlineColor->b, 0);
                break;
            }
        }
        outlineColor.buildFromSteps(def.steps, base.outlineColor.value_or(defaultOc), [](const KeyframeStep& s) { return s.outlineColor; });

        outlineWidth.buildFromSteps(def.steps, base.outlineWidth, [](const KeyframeStep& s) { return s.outlineWidth; });
        outlineOffset.buildFromSteps(def.steps, base.outlineOffset, [](const KeyframeStep& s) { return s.outlineOffset; });

        // Color (text): if base has none, inherit from computed style or theme
        Color defaultTextCol = (el && el->computedStyle().color.has_value()) ? *el->computedStyle().color : Theme::Current().colors.textPrimary;
        color.buildFromSteps(def.steps, base.color.value_or(defaultTextCol), [](const KeyframeStep& s) { return s.color; });

        float baseFs = base.fontSize.value_or(16.0f);
        fontSize.buildFromSteps(def.steps, baseFs, [](const KeyframeStep& s) { return s.fontSize; });

        float baseW = (base.width.mode == SizeMode::Fixed) ? base.width.value : (el ? el->bounds().width : 0.0f);
        float baseH = (base.height.mode == SizeMode::Fixed) ? base.height.value : (el ? el->bounds().height : 0.0f);
        width.buildFromSteps(def.steps, baseW, [](const KeyframeStep& s) { return s.width; });
        height.buildFromSteps(def.steps, baseH, [](const KeyframeStep& s) { return s.height; });
    }

    void updateBase(const ElementBaseStyleSnapshot& base, const UIElement* el) {
        if (opacity.active) opacity.updateImplicitStops(base.opacity);
        if (transformScale.active) transformScale.updateImplicitStops(base.transformScale);
        if (transformOffsetPx.active) transformOffsetPx.updateImplicitStops(base.transformOffsetPx);
        if (transformOffsetPct.active) transformOffsetPct.updateImplicitStops(base.transformOffsetPct);
        if (backgroundColor.active) {
            Color defaultBg = Color{ 0, 0, 0, 0 };
            if (base.backgroundColor.has_value()) {
                backgroundColor.updateImplicitStops(*base.backgroundColor);
            } else if (!backgroundColor.stops.empty()) {
                const auto& last = backgroundColor.stops.back().value;
                defaultBg = Color(last.r, last.g, last.b, 0);
                backgroundColor.updateImplicitStops(defaultBg);
            }
        }
        if (borderColor.active) {
            Color defaultBc = Color{ 0, 0, 0, 0 };
            if (base.borderColor.has_value()) {
                borderColor.updateImplicitStops(*base.borderColor);
            } else if (!borderColor.stops.empty()) {
                const auto& last = borderColor.stops.back().value;
                defaultBc = Color(last.r, last.g, last.b, 0);
                borderColor.updateImplicitStops(defaultBc);
            }
        }
        if (borderWidth.active) {
            borderWidth.updateImplicitStops(base.borderWidth.value_or(0.0f));
        }
        if (cornerRadius.active) {
            cornerRadius.updateImplicitStops(base.cornerRadius.value_or(0.0f));
        }
        if (outlineColor.active) {
            Color defaultOc = Color{ 0, 0, 0, 0 };
            if (base.outlineColor.has_value()) {
                outlineColor.updateImplicitStops(*base.outlineColor);
            } else if (!outlineColor.stops.empty()) {
                const auto& last = outlineColor.stops.back().value;
                defaultOc = Color(last.r, last.g, last.b, 0);
                outlineColor.updateImplicitStops(defaultOc);
            }
        }
        if (outlineWidth.active) outlineWidth.updateImplicitStops(base.outlineWidth);
        if (outlineOffset.active) outlineOffset.updateImplicitStops(base.outlineOffset);
        if (color.active) {
            Color defaultTextCol = (el && el->computedStyle().color.has_value()) ? *el->computedStyle().color : Theme::Current().colors.textPrimary;
            color.updateImplicitStops(base.color.value_or(defaultTextCol));
        }
        if (fontSize.active) {
            fontSize.updateImplicitStops(base.fontSize.value_or(16.0f));
        }

        float baseW = (base.width.mode == SizeMode::Fixed) ? base.width.value : (el ? el->bounds().width : 0.0f);
        float baseH = (base.height.mode == SizeMode::Fixed) ? base.height.value : (el ? el->bounds().height : 0.0f);
        if (width.active) width.updateImplicitStops(baseW);
        if (height.active) height.updateImplicitStops(baseH);
    }

    void apply(float progress, const EasingDef& defaultEasing, UIElement* element) const {
        if (!element) return;

        float valAlpha = 1.0f;
        if (!element->isAlphaAnimating() && opacity.evaluate(progress, defaultEasing, valAlpha)) {
            element->setOpacity(valAlpha);
        }

        Point valScale{ 1.0f, 1.0f };
        if (!element->isScaleAnimating() && transformScale.evaluate(progress, defaultEasing, valScale)) {
            element->setRenderScale(valScale);
        }

        Point px = element->renderOffset();
        Point pct = element->renderOffsetPercent();
        bool hasOffset = false;
        if (!element->isPositionAnimating()) {
            Point newPx{ 0.0f, 0.0f };
            if (transformOffsetPx.evaluate(progress, defaultEasing, newPx)) {
                px = newPx;
                hasOffset = true;
            }
            Point newPct{ 0.0f, 0.0f };
            if (transformOffsetPct.evaluate(progress, defaultEasing, newPct)) {
                pct = newPct;
                hasOffset = true;
            }
            if (hasOffset) {
                element->setRenderOffset(px, pct);
            }
        }

        Color bg;
        if (backgroundColor.evaluate(progress, defaultEasing, bg)) {
            element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).bg(bg);
        }

        Color bc;
        float bw = 1.0f;
        bool hasBorder = borderColor.evaluate(progress, defaultEasing, bc);
        bool hasBorderW = borderWidth.evaluate(progress, defaultEasing, bw);
        if (hasBorder || hasBorderW) {
            Color curBc = hasBorder ? bc : (element->style().box.normal.has_value() ? element->style().box.normal->borderColor.value_or(Color::White()) : Color::White());
            float curBw = hasBorderW ? bw : (element->style().box.normal.has_value() ? element->style().box.normal->borderWidth.value_or(1.0f) : 1.0f);
            element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).border(curBc, curBw);
        }

        float radius = 0.0f;
        if (cornerRadius.evaluate(progress, defaultEasing, radius)) {
            element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).radius(radius);
        }

        Color oc;
        if (outlineColor.evaluate(progress, defaultEasing, oc)) {
            element->setOutlineColor(oc);
        }
        float ow = 0.0f;
        if (outlineWidth.evaluate(progress, defaultEasing, ow)) {
            element->setOutlineWidth(ow);
        }
        float oo = 0.0f;
        if (outlineOffset.evaluate(progress, defaultEasing, oo)) {
            element->setOutlineOffset(oo);
        }

        Color tc;
        if (color.evaluate(progress, defaultEasing, tc)) {
            element->style().text.normal = element->style().text.normal.value_or(TextStyleOverride{}).col(tc);
        }

        float fsVal = 0.0f;
        if (fontSize.evaluate(progress, defaultEasing, fsVal)) {
            element->style().text.normal = element->style().text.normal.value_or(TextStyleOverride{}).size(fsVal);
        }

        float wVal = 0.0f;
        if (width.evaluate(progress, defaultEasing, wVal)) {
            element->layout().width(DimensionConstraint::Fixed(wVal));
            element->markLayoutDirty();
        }
        float hVal = 0.0f;
        if (height.evaluate(progress, defaultEasing, hVal)) {
            element->layout().height(DimensionConstraint::Fixed(hVal));
            element->markLayoutDirty();
        }
    }
};

struct ActiveKeyframeAnimation {
    AnimationSpec spec;
    const KeyframesDefinition* def{ nullptr };
    AnimationTracks tracks;
    float elapsedTime{ 0.0f };
    bool finished{ false };

    bool isInfinite() const {
        return spec.isInfinite();
    }
};

class StyleKeyframeAnimationController {
public:
    uint16_t getActiveProperties(const AnimationTracks& t) const {
        uint16_t mask = 0;
        if (t.opacity.active) mask |= (1 << 0);
        if (t.transformScale.active) mask |= (1 << 1);
        if (t.transformOffsetPx.active || t.transformOffsetPct.active) mask |= (1 << 2);
        if (t.backgroundColor.active) mask |= (1 << 3);
        if (t.borderColor.active) mask |= (1 << 4);
        if (t.borderWidth.active) mask |= (1 << 5);
        if (t.cornerRadius.active) mask |= (1 << 6);
        if (t.outlineColor.active || t.outlineWidth.active || t.outlineOffset.active) mask |= (1 << 7);
        if (t.color.active) mask |= (1 << 8);
        if (t.width.active) mask |= (1 << 9);
        if (t.height.active) mask |= (1 << 10);
        if (t.fontSize.active) mask |= (1 << 11);
        return mask;
    }

    void restoreInstanceProperties(const ActiveKeyframeAnimation& targetInst, UIElement* element) {
        if (!element) return;
        uint16_t targetMask = getActiveProperties(targetInst.tracks);
        // Do not restore properties that are still being actively animated by other running instances
        for (const auto& other : m_instances) {
            if (&other != &targetInst && !other.finished && other.spec.playState == AnimationPlayState::Running) {
                targetMask &= ~getActiveProperties(other.tracks);
            }
        }
        m_baseSnapshot.restoreProperties(targetMask, element);
    }

    void updateBaseFromComputed(UIElement* el, const ComputedStyle& c) {
        m_baseSnapshot.updateFromComputed(c, el);
        for (auto& inst : m_instances) {
            inst.tracks.updateBase(m_baseSnapshot, el);
        }
    }

    void applyAnimations(UIElement* element, const std::vector<AnimationSpec>& specs, const StyleSheet& sheet) {
        if (element && !m_baseSnapshot.captured) {
            m_baseSnapshot.capture(element);
        }

        // Restore properties for animations that were removed and not filled forwards
        if (element) {
            uint16_t removedMask = 0;
            uint16_t retainedRunningMask = 0;

            for (const auto& prev : m_instances) {
                bool retained = false;
                for (const auto& sp : specs) {
                    if (sp.name == prev.spec.name) {
                        retained = true;
                        if (sp.playState == AnimationPlayState::Running && !prev.finished) {
                            retainedRunningMask |= getActiveProperties(prev.tracks);
                        }
                        break;
                    }
                }
                if (!retained) {
                    if (prev.spec.fillMode != AnimationFillMode::Forwards && prev.spec.fillMode != AnimationFillMode::Both) {
                        removedMask |= getActiveProperties(prev.tracks);
                    }
                }
            }

            uint16_t restoreMask = removedMask & ~retainedRunningMask;
            if (restoreMask != 0) {
                m_baseSnapshot.restoreProperties(restoreMask, element);
            }
        }

        std::vector<ActiveKeyframeAnimation> newInstances;
        newInstances.reserve(specs.size());

        for (const auto& sp : specs) {
            const KeyframesDefinition* kfDef = sheet.findKeyframes(sp.name);
            if (!kfDef || kfDef->steps.empty()) continue;

            ActiveKeyframeAnimation inst;
            inst.spec = sp;
            inst.def = kfDef;
            inst.tracks.build(*kfDef, m_baseSnapshot, element);

            // Preserve elapsed time if animation already running with same name
            for (const auto& prev : m_instances) {
                if (prev.spec.name == sp.name && prev.def == kfDef) {
                    inst.elapsedTime = prev.elapsedTime;
                    inst.finished = prev.finished;
                    break;
                }
            }
            newInstances.push_back(std::move(inst));
        }

        m_instances = std::move(newInstances);

        if (element) {
            bool reduced = element->context() && element->context()->reducedMotion();
            if (reduced) {
                for (auto& inst : m_instances) {
                    inst.finished = true;
                    if (inst.isInfinite()) {
                        applyStepAtProgress(0.0f, inst, element);
                    } else {
                        if (inst.spec.fillMode == AnimationFillMode::Forwards || inst.spec.fillMode == AnimationFillMode::Both) {
                            float finalP = calculateFinalProgress(inst);
                            applyStepAtProgress(finalP, inst, element);
                        } else {
                            restoreInstanceProperties(inst, element);
                        }
                    }
                }
                return;
            }

            for (const auto& inst : m_instances) {
                if (inst.elapsedTime == 0.0f && (inst.spec.delay <= 0.0f || inst.spec.fillMode == AnimationFillMode::Backwards || inst.spec.fillMode == AnimationFillMode::Both)) {
                    applyStepAtProgress(0.0f, inst, element);
                }
            }
        }
    }

    bool hasActiveAnimations() const {
        for (const auto& inst : m_instances) {
            if (!inst.finished && inst.spec.playState == AnimationPlayState::Running) {
                return true;
            }
        }
        return false;
    }

    void clear(UIElement* element = nullptr) {
        if (element && m_baseSnapshot.captured) {
            uint16_t allProps = 0;
            for (const auto& inst : m_instances) {
                if (inst.spec.fillMode != AnimationFillMode::Forwards && inst.spec.fillMode != AnimationFillMode::Both) {
                    allProps |= getActiveProperties(inst.tracks);
                }
            }
            m_baseSnapshot.restoreProperties(allProps, element);
            m_baseSnapshot.captured = false;
        }
        m_instances.clear();
    }

    const ElementBaseStyleSnapshot& baseSnapshot() const {
        return m_baseSnapshot;
    }

    static float calculateFinalProgress(const ActiveKeyframeAnimation& inst) {
        float cycleFloat = inst.spec.iterationCount;
        int cycleIndex = static_cast<int>(cycleFloat);
        float frac = cycleFloat - static_cast<float>(cycleIndex);
        if (frac <= 0.0001f && cycleIndex > 0) {
            cycleIndex -= 1;
            frac = 1.0f;
        }
        bool reverse = false;
        switch (inst.spec.direction) {
        case AnimationDirection::Reverse:
            reverse = true;
            break;
        case AnimationDirection::Alternate:
            reverse = (cycleIndex % 2 == 1);
            break;
        case AnimationDirection::AlternateReverse:
            reverse = (cycleIndex % 2 == 0);
            break;
        default:
            break;
        }
        return reverse ? (1.0f - frac) : frac;
    }

    void update(float dt, UIElement* element) {
        if (!element || m_instances.empty()) return;

        bool reducedMotion = element && element->context() && element->context()->reducedMotion();
        if (reducedMotion) {
            for (auto& inst : m_instances) {
                inst.finished = true;
                if (inst.isInfinite()) {
                    // In reduced motion mode, freeze infinite animations at initial 0% frame
                    applyStepAtProgress(0.0f, inst, element);
                } else {
                    if (inst.spec.fillMode == AnimationFillMode::Forwards || inst.spec.fillMode == AnimationFillMode::Both) {
                        float finalP = calculateFinalProgress(inst);
                        applyStepAtProgress(finalP, inst, element);
                    } else {
                        restoreInstanceProperties(inst, element);
                    }
                }
            }
            return;
        }

        for (auto& inst : m_instances) {
            if (inst.spec.playState == AnimationPlayState::Paused || inst.finished) continue;

            inst.elapsedTime += dt;
            float delay = inst.spec.delay;
            float duration = (std::max)(0.0001f, inst.spec.duration);

            if (inst.elapsedTime < delay) {
                if (inst.spec.fillMode == AnimationFillMode::Backwards || inst.spec.fillMode == AnimationFillMode::Both) {
                    applyStepAtProgress(0.0f, inst, element);
                }
                continue;
            }

            float activeTime = inst.elapsedTime - delay;
            bool infinite = inst.isInfinite();
            float totalDuration = infinite ? 0.0f : (duration * inst.spec.iterationCount);

            if (!infinite && activeTime >= totalDuration) {
                inst.finished = true;
                if (inst.spec.fillMode == AnimationFillMode::Forwards || inst.spec.fillMode == AnimationFillMode::Both) {
                    float finalP = calculateFinalProgress(inst);
                    applyStepAtProgress(finalP, inst, element);
                } else {
                    restoreInstanceProperties(inst, element);
                }
                continue;
            }

            float cycleFloat = activeTime / duration;
            int cycleIndex = static_cast<int>(cycleFloat);
            float progress = cycleFloat - static_cast<float>(cycleIndex);

            bool reverse = false;
            switch (inst.spec.direction) {
            case AnimationDirection::Reverse:
                reverse = true;
                break;
            case AnimationDirection::Alternate:
                reverse = (cycleIndex % 2 == 1);
                break;
            case AnimationDirection::AlternateReverse:
                reverse = (cycleIndex % 2 == 0);
                break;
            default:
                break;
            }
            if (reverse) progress = 1.0f - progress;

            applyStepAtProgress(progress, inst, element);
        }
    }

private:
    void applyStepAtProgress(float rawProgress, const ActiveKeyframeAnimation& inst, UIElement* element) {
        if (!inst.def || !element) return;
        inst.tracks.apply(rawProgress, inst.spec.easing, element);
    }

    std::vector<ActiveKeyframeAnimation> m_instances;
    ElementBaseStyleSnapshot m_baseSnapshot;
};

} // namespace PerfUI
