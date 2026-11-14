#pragma once

#include "PerfUI/Types.h"
#include "PerfUI/Animation.h"
#include "PerfUI/Style/StyleTypes.h"
#include "PerfUI/Style/StyleSheet.h"
#include "PerfUI/UIElement.h"
#include <cmath>
#include <vector>
#include <algorithm>

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

struct ActiveKeyframeAnimation {
    AnimationSpec spec;
    const KeyframesDefinition* def{ nullptr };
    float elapsedTime{ 0.0f };
    bool finished{ false };

    bool isInfinite() const {
        return spec.isInfinite();
    }
};

class StyleKeyframeAnimationController {
public:
    void applyAnimations(UIElement* element, const std::vector<AnimationSpec>& specs, const StyleSheet& sheet) {
        (void)element;
        std::vector<ActiveKeyframeAnimation> newInstances;
        newInstances.reserve(specs.size());

        for (const auto& sp : specs) {
            const KeyframesDefinition* kfDef = sheet.findKeyframes(sp.name);
            if (!kfDef || kfDef->steps.empty()) continue;

            ActiveKeyframeAnimation inst;
            inst.spec = sp;
            inst.def = kfDef;

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

    void clear() {
        m_instances.clear();
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
        if (!inst.def || inst.def->steps.empty() || !element) return;

        float eased = ApplyEasing((std::clamp)(rawProgress, 0.0f, 1.0f), inst.spec.easing);
        eased = (std::clamp)(eased, 0.0f, 1.0f);

        const auto& steps = inst.def->steps;
        if (steps.size() == 1) {
            applyKeyframeProperties(steps[0], steps[0], 0.0f, element);
            return;
        }

        const KeyframeStep* k0 = &steps.front();
        const KeyframeStep* k1 = &steps.back();

        for (size_t i = 0; i + 1 < steps.size(); ++i) {
            if (eased >= steps[i].offset && eased <= steps[i + 1].offset) {
                k0 = &steps[i];
                k1 = &steps[i + 1];
                break;
            }
        }

        float segSpan = k1->offset - k0->offset;
        float segT = (segSpan > 0.0001f) ? (eased - k0->offset) / segSpan : 0.0f;
        if (k0->timingFunction.has_value()) {
            segT = ApplyEasing(segT, *k0->timingFunction);
        }
        segT = (std::clamp)(segT, 0.0f, 1.0f);

        applyKeyframeProperties(*k0, *k1, segT, element);
    }

    void applyKeyframeProperties(const KeyframeStep& k0, const KeyframeStep& k1, float t, UIElement* element) {
        // 1. Opacity
        if (k0.opacity.has_value() && k1.opacity.has_value()) {
            element->setOpacity(detail::LerpValue(*k0.opacity, *k1.opacity, t));
        } else if (k1.opacity.has_value() && t >= 0.999f) {
            element->setOpacity(*k1.opacity);
        } else if (k0.opacity.has_value() && t <= 0.001f) {
            element->setOpacity(*k0.opacity);
        }

        // 2. Transform Scale
        if (k0.transformScale.has_value() && k1.transformScale.has_value()) {
            element->setRenderScale(detail::LerpValue(*k0.transformScale, *k1.transformScale, t));
        } else if (k1.transformScale.has_value() && t >= 0.999f) {
            element->setRenderScale(*k1.transformScale);
        } else if (k0.transformScale.has_value() && t <= 0.001f) {
            element->setRenderScale(*k0.transformScale);
        }

        // 3. Transform Offset
        if (k0.transformOffsetPx.has_value() || k1.transformOffsetPx.has_value() ||
            k0.transformOffsetPct.has_value() || k1.transformOffsetPct.has_value()) {
            Point px0 = k0.transformOffsetPx.value_or(Point{ 0.0f, 0.0f });
            Point px1 = k1.transformOffsetPx.value_or(Point{ 0.0f, 0.0f });
            Point pct0 = k0.transformOffsetPct.value_or(Point{ 0.0f, 0.0f });
            Point pct1 = k1.transformOffsetPct.value_or(Point{ 0.0f, 0.0f });
            Point curPx = detail::LerpValue(px0, px1, t);
            Point curPct = detail::LerpValue(pct0, pct1, t);
            element->setRenderOffset(curPx, curPct);
        }

        // 4. Background Color
        if (k0.backgroundColor.has_value() && k1.backgroundColor.has_value()) {
            Color bg = Color::Lerp(*k0.backgroundColor, *k1.backgroundColor, t);
            element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).bg(bg);
        } else if (k1.backgroundColor.has_value() && t >= 0.999f) {
            element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).bg(*k1.backgroundColor);
        }

        // 5. Border Color & Width
        if (k0.borderColor.has_value() && k1.borderColor.has_value()) {
            Color bc = Color::Lerp(*k0.borderColor, *k1.borderColor, t);
            float bw = detail::LerpValue(k0.borderWidth.value_or(1.0f), k1.borderWidth.value_or(1.0f), t);
            element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).border(bc, bw);
        }

        // 6. Corner Radius
        if (k0.cornerRadius.has_value() && k1.cornerRadius.has_value()) {
            float r = detail::LerpValue(*k0.cornerRadius, *k1.cornerRadius, t);
            element->style().box.normal = element->style().box.normal.value_or(BoxStyle{}).radius(r);
        }

        // 7. Outline
        if (k0.outlineColor.has_value() && k1.outlineColor.has_value()) {
            element->setOutlineColor(Color::Lerp(*k0.outlineColor, *k1.outlineColor, t));
        }
        if (k0.outlineWidth.has_value() && k1.outlineWidth.has_value()) {
            element->setOutlineWidth(detail::LerpValue(*k0.outlineWidth, *k1.outlineWidth, t));
        }
        if (k0.outlineOffset.has_value() && k1.outlineOffset.has_value()) {
            element->setOutlineOffset(detail::LerpValue(*k0.outlineOffset, *k1.outlineOffset, t));
        }

        // 8. Text Color
        if (k0.color.has_value() && k1.color.has_value()) {
            Color tc = Color::Lerp(*k0.color, *k1.color, t);
            element->style().text.normal = element->style().text.normal.value_or(TextStyleOverride{}).col(tc);
        }

        // 9. Width & Height
        if (k0.width.has_value() && k1.width.has_value()) {
            float w = detail::LerpValue(*k0.width, *k1.width, t);
            element->layout().width(DimensionConstraint::Fixed(w));
            element->markLayoutDirty();
        }
        if (k0.height.has_value() && k1.height.has_value()) {
            float h = detail::LerpValue(*k0.height, *k1.height, t);
            element->layout().height(DimensionConstraint::Fixed(h));
            element->markLayoutDirty();
        }
    }

    std::vector<ActiveKeyframeAnimation> m_instances;
};

} // namespace PerfUI
