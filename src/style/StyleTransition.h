#pragma once

#include "PerfUI/Types.h"
#include "PerfUI/Animation.h"
#include "PerfUI/Style/StyleTypes.h"
#include "PerfUI/Style/ComputedStyle.h"
#include "PerfUI/UIElement.h"
#include <cmath>
#include <vector>

namespace PerfUI {

template<typename T>
inline T LerpVal(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}

template<>
inline Color LerpVal<Color>(const Color& a, const Color& b, float t) {
    return Color::Lerp(a, b, t);
}

template<>
inline Point LerpVal<Point>(const Point& a, const Point& b, float t) {
    return Point{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

template<typename T>
struct TransitionTrack {
    bool active{ false };
    T startVal{};
    T currentVal{};
    T targetVal{};
    float duration{ 0.0f };
    float delay{ 0.0f };
    float elapsed{ 0.0f };
    EasingDef easing{ Easing::EaseOutCubic };

    void start(const T& current, const T& target, float dur, float del, EasingDef ease) {
        startVal = current;
        currentVal = current;
        targetVal = target;
        duration = dur;
        delay = del;
        elapsed = 0.0f;
        easing = ease;
        active = true;
    }

    bool update(float dt) {
        if (!active) return false;
        elapsed += dt;
        if (elapsed < delay) {
            return true;
        }

        float progress = (duration > 0.0001f) ? (elapsed - delay) / duration : 1.0f;
        if (progress >= 1.0f) {
            currentVal = targetVal;
            active = false;
        } else {
            float t = ApplyEasing(progress, easing);
            currentVal = LerpVal(startVal, targetVal, t);
        }
        return true;
    }

    void finish() {
        if (active) {
            currentVal = targetVal;
            active = false;
        }
    }
};

inline const TransitionSpec* findTransitionSpec(const std::vector<TransitionSpec>& specs, TransitionProperty prop) {
    const TransitionSpec* allSpec = nullptr;
    for (const auto& s : specs) {
        if (s.property == prop) return &s;
        if (s.property == TransitionProperty::All && !allSpec) allSpec = &s;
    }
    return allSpec;
}

struct StyleTransitionController {
    TransitionTrack<Color> trackBackground;
    TransitionTrack<Color> trackBorderColor;
    TransitionTrack<float> trackBorderWidth;
    TransitionTrack<float> trackCornerRadius;
    TransitionTrack<Color> trackColor;
    TransitionTrack<float> trackOpacity;
    TransitionTrack<Point> trackTransformScale;
    TransitionTrack<Point> trackTransformOffset;
    TransitionTrack<float> trackWidth;
    TransitionTrack<float> trackHeight;

    bool hasActiveTransitions() const {
        return trackBackground.active ||
               trackBorderColor.active ||
               trackBorderWidth.active ||
               trackCornerRadius.active ||
               trackColor.active ||
               trackOpacity.active ||
               trackTransformScale.active ||
               trackTransformOffset.active ||
               trackWidth.active ||
               trackHeight.active;
    }

    void finishAll(UIElement* element) {
        if (trackBackground.active) {
            trackBackground.finish();
            element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).bg(trackBackground.currentVal);
        }
        if (trackBorderColor.active) {
            trackBorderColor.finish();
            BoxStyle bs = element->style().box.normal.value_or(BoxStyle{});
            bs.borderColor = trackBorderColor.currentVal;
            element->style().box.normal = bs;
        }
        if (trackBorderWidth.active) {
            trackBorderWidth.finish();
            BoxStyle bs = element->style().box.normal.value_or(BoxStyle{});
            bs.borderWidth = trackBorderWidth.currentVal;
            element->style().box.normal = bs;
        }
        if (trackCornerRadius.active) {
            trackCornerRadius.finish();
            BoxStyle bs = element->style().box.normal.value_or(BoxStyle{});
            bs.cornerRadius = trackCornerRadius.currentVal;
            element->style().box.normal = bs;
        }
        if (trackColor.active) {
            trackColor.finish();
            element->style().text.normal = element->style().text.normal.value_or(TextStyleOverride{}).col(trackColor.currentVal);
        }
        if (trackOpacity.active) {
            trackOpacity.finish();
            element->setOpacity(trackOpacity.currentVal);
        }
        if (trackTransformScale.active) {
            trackTransformScale.finish();
            element->setRenderScale(trackTransformScale.currentVal);
        }
        if (trackTransformOffset.active) {
            trackTransformOffset.finish();
            element->setRenderOffset(trackTransformOffset.currentVal);
        }
        if (trackWidth.active) {
            trackWidth.finish();
            element->layout().width(DimensionConstraint::Fixed(trackWidth.currentVal));
            element->markLayoutDirty();
        }
        if (trackHeight.active) {
            trackHeight.finish();
            element->layout().height(DimensionConstraint::Fixed(trackHeight.currentVal));
            element->markLayoutDirty();
        }
    }

    void update(float dt, UIElement* element) {
        if (trackBackground.active) {
            trackBackground.update(dt);
            element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).bg(trackBackground.currentVal);
        }
        if (trackBorderColor.active) {
            trackBorderColor.update(dt);
            BoxStyle bs = element->style().box.normal.value_or(BoxStyle{});
            bs.borderColor = trackBorderColor.currentVal;
            element->style().box.normal = bs;
        }
        if (trackBorderWidth.active) {
            trackBorderWidth.update(dt);
            BoxStyle bs = element->style().box.normal.value_or(BoxStyle{});
            bs.borderWidth = trackBorderWidth.currentVal;
            element->style().box.normal = bs;
        }
        if (trackCornerRadius.active) {
            trackCornerRadius.update(dt);
            BoxStyle bs = element->style().box.normal.value_or(BoxStyle{});
            bs.cornerRadius = trackCornerRadius.currentVal;
            element->style().box.normal = bs;
        }
        if (trackColor.active) {
            trackColor.update(dt);
            element->style().text.normal = element->style().text.normal.value_or(TextStyleOverride{}).col(trackColor.currentVal);
        }
        if (trackOpacity.active) {
            trackOpacity.update(dt);
            element->setOpacity(trackOpacity.currentVal);
        }
        if (trackTransformScale.active) {
            trackTransformScale.update(dt);
            element->setRenderScale(trackTransformScale.currentVal);
        }
        if (trackTransformOffset.active) {
            trackTransformOffset.update(dt);
            element->setRenderOffset(trackTransformOffset.currentVal);
        }
        if (trackWidth.active) {
            trackWidth.update(dt);
            element->layout().width(DimensionConstraint::Fixed(trackWidth.currentVal));
            element->markLayoutDirty();
        }
        if (trackHeight.active) {
            trackHeight.update(dt);
            element->layout().height(DimensionConstraint::Fixed(trackHeight.currentVal));
            element->markLayoutDirty();
        }
    }

    void applyTransitions(UIElement* element, const ComputedStyle& oldStyle, const ComputedStyle& newStyle) {
        // 1. Background
        if (newStyle.backgroundColor.has_value()) {
            Color target = *newStyle.backgroundColor;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::Background);
            if (spec && spec->duration > 0.0001f) {
                Color current = trackBackground.active ? trackBackground.currentVal
                    : (element->style().box.normal && element->style().box.normal->backgroundColor.has_value()
                       ? *element->style().box.normal->backgroundColor
                       : oldStyle.backgroundColor.value_or(target));
                if (trackBackground.active && trackBackground.targetVal == target) {
                    // Continuing towards same target
                } else if (current != target) {
                    trackBackground.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackBackground.finish();
                element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).bg(target);
            }
        }

        // 2. BorderColor
        if (newStyle.borderColor.has_value()) {
            Color target = *newStyle.borderColor;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::BorderColor);
            if (spec && spec->duration > 0.0001f) {
                Color current = trackBorderColor.active ? trackBorderColor.currentVal
                    : (element->style().box.normal && element->style().box.normal->borderColor.has_value()
                       ? *element->style().box.normal->borderColor
                       : oldStyle.borderColor.value_or(target));
                if (trackBorderColor.active && trackBorderColor.targetVal == target) {
                } else if (current != target) {
                    trackBorderColor.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackBorderColor.finish();
                BoxStyle bs = element->style().box.normal.value_or(BoxStyle{});
                bs.borderColor = target;
                element->style().box.normal = bs;
            }
        }

        // 3. BorderWidth
        if (newStyle.borderWidth.has_value()) {
            float target = *newStyle.borderWidth;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::BorderWidth);
            if (spec && spec->duration > 0.0001f) {
                float current = trackBorderWidth.active ? trackBorderWidth.currentVal
                    : (element->style().box.normal && element->style().box.normal->borderWidth.has_value()
                       ? *element->style().box.normal->borderWidth
                       : oldStyle.borderWidth.value_or(target));
                if (trackBorderWidth.active && std::abs(trackBorderWidth.targetVal - target) < 0.001f) {
                } else if (std::abs(current - target) >= 0.001f) {
                    trackBorderWidth.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackBorderWidth.finish();
                BoxStyle bs = element->style().box.normal.value_or(BoxStyle{});
                bs.borderWidth = target;
                element->style().box.normal = bs;
            }
        }

        // 4. CornerRadius
        if (newStyle.cornerRadius.has_value()) {
            float target = *newStyle.cornerRadius;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::CornerRadius);
            if (spec && spec->duration > 0.0001f) {
                float current = trackCornerRadius.active ? trackCornerRadius.currentVal
                    : (element->style().box.normal && element->style().box.normal->cornerRadius.has_value()
                       ? *element->style().box.normal->cornerRadius
                       : oldStyle.cornerRadius.value_or(target));
                if (trackCornerRadius.active && std::abs(trackCornerRadius.targetVal - target) < 0.001f) {
                } else if (std::abs(current - target) >= 0.001f) {
                    trackCornerRadius.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackCornerRadius.finish();
                BoxStyle bs = element->style().box.normal.value_or(BoxStyle{});
                bs.cornerRadius = target;
                element->style().box.normal = bs;
            }
        }

        // 5. Color (text)
        if (newStyle.color.has_value()) {
            Color target = *newStyle.color;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::Color);
            if (spec && spec->duration > 0.0001f) {
                Color current = trackColor.active ? trackColor.currentVal
                    : (element->style().text.normal && element->style().text.normal->color.has_value()
                       ? *element->style().text.normal->color
                       : oldStyle.color.value_or(target));
                if (trackColor.active && trackColor.targetVal == target) {
                } else if (current != target) {
                    trackColor.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackColor.finish();
                element->style().text.normal = element->style().text.normal.value_or(TextStyleOverride{}).col(target);
            }
        }

        // 6. Opacity
        if (newStyle.opacity.has_value() && !element->isAlphaAnimating()) {
            float target = *newStyle.opacity;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::Opacity);
            if (spec && spec->duration > 0.0001f) {
                float current = trackOpacity.active ? trackOpacity.currentVal : element->opacity();
                if (trackOpacity.active && std::abs(trackOpacity.targetVal - target) < 0.001f) {
                } else if (std::abs(current - target) >= 0.001f) {
                    trackOpacity.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackOpacity.finish();
                element->setOpacity(target);
            }
        }

        // 7. TransformScale
        if (newStyle.transformScale.has_value() && !element->isScaleAnimating()) {
            Point target = *newStyle.transformScale;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::TransformScale);
            if (spec && spec->duration > 0.0001f) {
                Point current = trackTransformScale.active ? trackTransformScale.currentVal : element->renderScale();
                if (trackTransformScale.active && trackTransformScale.targetVal == target) {
                } else if (current != target) {
                    trackTransformScale.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackTransformScale.finish();
                element->setRenderScale(target);
            }
        }

        // 8. TransformOffset
        if (newStyle.transformOffsetPx.has_value() && !element->isPositionAnimating()) {
            Point target = *newStyle.transformOffsetPx;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::TransformOffset);
            if (spec && spec->duration > 0.0001f) {
                Point current = trackTransformOffset.active ? trackTransformOffset.currentVal : element->renderOffset();
                if (trackTransformOffset.active && trackTransformOffset.targetVal == target) {
                } else if (current != target) {
                    trackTransformOffset.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackTransformOffset.finish();
                element->setRenderOffset(target);
            }
        }

        // 9. Width
        if (newStyle.width.has_value()) {
            float target = *newStyle.width;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::Width);
            if (spec && spec->duration > 0.0001f) {
                float current = trackWidth.active ? trackWidth.currentVal
                    : (element->bounds().width > 0.0f ? element->bounds().width : target);
                if (trackWidth.active && std::abs(trackWidth.targetVal - target) < 0.001f) {
                } else if (std::abs(current - target) >= 0.001f) {
                    trackWidth.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackWidth.finish();
                element->layout().width(DimensionConstraint::Fixed(target));
                element->markLayoutDirty();
            }
        }

        // 10. Height
        if (newStyle.height.has_value()) {
            float target = *newStyle.height;
            const TransitionSpec* spec = findTransitionSpec(newStyle.transitions, TransitionProperty::Height);
            if (spec && spec->duration > 0.0001f) {
                float current = trackHeight.active ? trackHeight.currentVal
                    : (element->bounds().height > 0.0f ? element->bounds().height : target);
                if (trackHeight.active && std::abs(trackHeight.targetVal - target) < 0.001f) {
                } else if (std::abs(current - target) >= 0.001f) {
                    trackHeight.start(current, target, spec->duration, spec->delay, spec->easing);
                }
            } else {
                trackHeight.finish();
                element->layout().height(DimensionConstraint::Fixed(target));
                element->markLayoutDirty();
            }
        }
    }
};

} // namespace PerfUI
