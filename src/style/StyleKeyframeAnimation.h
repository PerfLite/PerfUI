#pragma once

#include "PerfUI/Types.h"
#include "PerfUI/Animation.h"
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
    return Color::Lerp(a, b, t);
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
    DimensionConstraint width{ DimensionConstraint::Auto() };
    DimensionConstraint height{ DimensionConstraint::Auto() };
    bool captured{ false };

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
        }
        width = el->layout().width();
        height = el->layout().height();
        captured = true;
    }

    void restore(UIElement* el) const {
        if (!el || !captured) return;
        if (!el->isAlphaAnimating()) el->setOpacity(opacity);
        if (!el->isScaleAnimating()) el->setRenderScale(transformScale);
        if (!el->isPositionAnimating()) el->setRenderOffset(transformOffsetPx, transformOffsetPct);
        if (el->style().box.normal.has_value() || backgroundColor || borderColor || borderWidth || cornerRadius) {
            BoxStyle box = el->style().box.normal.value_or(BoxStyle{});
            box.backgroundColor = backgroundColor;
            box.borderColor = borderColor;
            box.borderWidth = borderWidth;
            box.cornerRadius = cornerRadius;
            el->style().box.normal = box;
        }
        if (outlineColor.has_value()) el->setOutlineColor(*outlineColor);
        else el->setOutlineColor(Color{ 0, 0, 0, 0 });
        el->setOutlineWidth(outlineWidth);
        el->setOutlineOffset(outlineOffset);
        if (color.has_value()) {
            TextStyleOverride text = el->style().text.normal.value_or(TextStyleOverride{});
            text.color = color;
            el->style().text.normal = text;
        }
        el->layout().width(width);
        el->layout().height(height);
        el->markLayoutDirty();
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

    template<typename Getter>
    void buildFromSteps(const std::vector<KeyframeStep>& steps, const T& baseValue, Getter getter) {
        stops.clear();
        active = false;

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
        }

        // Implicit end keyframe (offset == 1.0f)
        if (stops.back().offset < 0.9999f) {
            stops.push_back({ 1.0f, baseValue, std::nullopt });
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
    PropertyTrack<float> width;
    PropertyTrack<float> height;

    void build(const KeyframesDefinition& def, const ElementBaseStyleSnapshot& base, const UIElement* el) {
        opacity.buildFromSteps(def.steps, base.opacity, [](const KeyframeStep& s) { return s.opacity; });
        transformScale.buildFromSteps(def.steps, base.transformScale, [](const KeyframeStep& s) { return s.transformScale; });
        transformOffsetPx.buildFromSteps(def.steps, base.transformOffsetPx, [](const KeyframeStep& s) { return s.transformOffsetPx; });
        transformOffsetPct.buildFromSteps(def.steps, base.transformOffsetPct, [](const KeyframeStep& s) { return s.transformOffsetPct; });
        backgroundColor.buildFromSteps(def.steps, base.backgroundColor.value_or(Color{ 0, 0, 0, 0 }), [](const KeyframeStep& s) { return s.backgroundColor; });
        borderColor.buildFromSteps(def.steps, base.borderColor.value_or(Color{ 0, 0, 0, 0 }), [](const KeyframeStep& s) { return s.borderColor; });
        borderWidth.buildFromSteps(def.steps, base.borderWidth.value_or(1.0f), [](const KeyframeStep& s) { return s.borderWidth; });
        cornerRadius.buildFromSteps(def.steps, base.cornerRadius.value_or(0.0f), [](const KeyframeStep& s) { return s.cornerRadius; });
        outlineColor.buildFromSteps(def.steps, base.outlineColor.value_or(Color{ 0, 0, 0, 0 }), [](const KeyframeStep& s) { return s.outlineColor; });
        outlineWidth.buildFromSteps(def.steps, base.outlineWidth, [](const KeyframeStep& s) { return s.outlineWidth; });
        outlineOffset.buildFromSteps(def.steps, base.outlineOffset, [](const KeyframeStep& s) { return s.outlineOffset; });
        color.buildFromSteps(def.steps, base.color.value_or(Color{ 255, 255, 255, 255 }), [](const KeyframeStep& s) { return s.color; });

        float baseW = (base.width.mode == SizeMode::Fixed) ? base.width.value : (el ? el->bounds().width : 0.0f);
        float baseH = (base.height.mode == SizeMode::Fixed) ? base.height.value : (el ? el->bounds().height : 0.0f);
        width.buildFromSteps(def.steps, baseW, [](const KeyframeStep& s) { return s.width; });
        height.buildFromSteps(def.steps, baseH, [](const KeyframeStep& s) { return s.height; });
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
    void applyAnimations(UIElement* element, const std::vector<AnimationSpec>& specs, const StyleSheet& sheet) {
        if (element && !m_baseSnapshot.captured) {
            m_baseSnapshot.capture(element);
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
            m_baseSnapshot.restore(element);
            m_baseSnapshot.captured = false;
        }
        m_instances.clear();
    }

    const ElementBaseStyleSnapshot& baseSnapshot() const {
        return m_baseSnapshot;
    }

    void update(float dt, UIElement* element) {
        if (!element || m_instances.empty()) return;

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
                    // Final frame depends on direction & iteration count
                    float finalP = 1.0f;
                    int finalCycle = static_cast<int>(inst.spec.iterationCount);
                    if (inst.spec.direction == AnimationDirection::Reverse) {
                        finalP = 0.0f;
                    } else if (inst.spec.direction == AnimationDirection::Alternate) {
                        if (finalCycle % 2 == 0) finalP = 0.0f;
                    } else if (inst.spec.direction == AnimationDirection::AlternateReverse) {
                        if (finalCycle % 2 != 0) finalP = 0.0f;
                    }
                    applyStepAtProgress(finalP, inst, element);
                } else {
                    m_baseSnapshot.restore(element);
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
