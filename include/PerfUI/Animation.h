#pragma once

#include "PerfUI/Types.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <functional>
#include <vector>
#include <string>
#include <numbers>

namespace PerfUI {

class UIElement;
class AnimationScheduler;

enum class AnimPositionMode {
    RenderOffset, // Default: Visual transform layer, 0 layout passes
    BoundsLayout  // Explicit: Modifies bounds directly
};

enum class SpringPositionMode {
    RenderOffset, // Default
    BoundsLayout
};

// ============================================================================
// 1. Easing Curves
// ============================================================================

enum class Easing {
    Linear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInCirc,
    EaseOutCirc,
    EaseInOutCirc,
    Custom
};

struct CubicBezierParams {
    float x1{ 0.25f };
    float y1{ 0.1f };
    float x2{ 0.25f };
    float y2{ 1.0f };

    bool operator==(const CubicBezierParams&) const = default;
};

inline float SolveCubicBezier(float t, float x1, float y1, float x2, float y2) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    auto sampleX = [x1, x2](float u) {
        float inv = 1.0f - u;
        return 3.0f * inv * inv * u * x1 + 3.0f * inv * u * u * x2 + u * u * u;
    };
    auto sampleXDeriv = [x1, x2](float u) {
        float inv = 1.0f - u;
        return 3.0f * inv * inv * x1 + 6.0f * inv * u * (x2 - x1) + 3.0f * u * u * (1.0f - x2);
    };
    auto sampleY = [y1, y2](float u) {
        float inv = 1.0f - u;
        return 3.0f * inv * inv * u * y1 + 3.0f * inv * u * u * y2 + u * u * u;
    };

    float u = t;
    for (int i = 0; i < 8; ++i) {
        float x = sampleX(u) - t;
        if (std::abs(x) < 1e-5f) {
            return sampleY(u);
        }
        float d = sampleXDeriv(u);
        if (std::abs(d) < 1e-6f) break;
        u -= x / d;
        u = (std::clamp)(u, 0.0f, 1.0f);
    }

    float low = 0.0f, high = 1.0f;
    u = t;
    for (int i = 0; i < 12; ++i) {
        float x = sampleX(u);
        if (std::abs(x - t) < 1e-5f) {
            return sampleY(u);
        }
        if (x > t) high = u;
        else low = u;
        u = (low + high) * 0.5f;
    }

    return sampleY(u);
}

struct EasingDef {
    Easing type{ Easing::EaseOutCubic };
    CubicBezierParams bezier{};

    EasingDef() = default;
    EasingDef(Easing e) : type(e) {}
    static EasingDef Bezier(float x1, float y1, float x2, float y2) {
        EasingDef def;
        def.type = Easing::Custom;
        def.bezier = { x1, y1, x2, y2 };
        return def;
    }

    operator Easing() const { return type; }
    bool operator==(const EasingDef&) const = default;
};

inline float ApplyEasing(float t, EasingDef easing = Easing::EaseOutCubic) {
    t = (std::clamp)(t, 0.0f, 1.0f);
    if (easing.type == Easing::Custom) {
        return SolveCubicBezier(t, easing.bezier.x1, easing.bezier.y1, easing.bezier.x2, easing.bezier.y2);
    }
    switch (easing.type) {
    case Easing::Linear:
        return t;
    case Easing::EaseInQuad:
        return t * t;
    case Easing::EaseOutQuad:
        return 1.0f - (1.0f - t) * (1.0f - t);
    case Easing::EaseInOutQuad: {
        if (t < 0.5f) {
            return 2.0f * t * t;
        }
        float p = -2.0f * t + 2.0f;
        return 1.0f - (p * p) * 0.5f;
    }
    case Easing::EaseInCubic:
        return t * t * t;
    case Easing::EaseOutCubic: {
        float inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }
    case Easing::EaseInOutCubic: {
        if (t < 0.5f) {
            return 4.0f * t * t * t;
        }
        float p = -2.0f * t + 2.0f;
        return 1.0f - (p * p * p) * 0.5f;
    }
    case Easing::EaseInQuart:
        return t * t * t * t;
    case Easing::EaseOutQuart: {
        float inv = 1.0f - t;
        return 1.0f - inv * inv * inv * inv;
    }
    case Easing::EaseInOutQuart: {
        if (t < 0.5f) {
            return 8.0f * t * t * t * t;
        }
        float p = -2.0f * t + 2.0f;
        return 1.0f - (p * p * p * p) * 0.5f;
    }
    case Easing::EaseInExpo:
        return (t == 0.0f) ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
    case Easing::EaseOutExpo:
        return (t == 1.0f) ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
    case Easing::EaseInOutExpo: {
        if (t == 0.0f) return 0.0f;
        if (t == 1.0f) return 1.0f;
        if (t < 0.5f) return std::pow(2.0f, 20.0f * t - 10.0f) * 0.5f;
        return (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) * 0.5f;
    }
    case Easing::EaseInCirc:
        return 1.0f - std::sqrt(1.0f - t * t);
    case Easing::EaseOutCirc: {
        float inv = t - 1.0f;
        return std::sqrt(1.0f - inv * inv);
    }
    case Easing::EaseInOutCirc: {
        if (t < 0.5f) {
            return (1.0f - std::sqrt(1.0f - 4.0f * t * t)) * 0.5f;
        }
        float p = -2.0f * t + 2.0f;
        return (std::sqrt(1.0f - p * p) + 1.0f) * 0.5f;
    }
    case Easing::EaseInSine:
        return 1.0f - std::cos((t * std::numbers::pi_v<float>) * 0.5f);
    case Easing::EaseOutSine:
        return std::sin((t * std::numbers::pi_v<float>) * 0.5f);
    case Easing::EaseInOutSine:
        return -(std::cos(std::numbers::pi_v<float> * t) - 1.0f) * 0.5f;
    case Easing::EaseInBack: {
        constexpr float c1 = 1.70158f;
        constexpr float c3 = c1 + 1.0f;
        return c3 * t * t * t - c1 * t * t;
    }
    case Easing::EaseOutBack: {
        constexpr float c1 = 1.70158f;
        constexpr float c3 = c1 + 1.0f;
        float inv = t - 1.0f;
        return 1.0f + c3 * inv * inv * inv + c1 * inv * inv;
    }
    case Easing::EaseInOutBack: {
        constexpr float c1 = 1.70158f;
        constexpr float c2 = c1 * 1.525f;
        if (t < 0.5f) {
            float t2 = 2.0f * t;
            return (t2 * t2 * ((c2 + 1.0f) * t2 - c2)) * 0.5f;
        } else {
            float t2m2 = 2.0f * t - 2.0f;
            return (t2m2 * t2m2 * ((c2 + 1.0f) * t2m2 + c2) + 2.0f) * 0.5f;
        }
    }
    case Easing::EaseOutBounce: {
        constexpr float n1 = 7.5625f;
        constexpr float d1 = 2.75f;
        if (t < 1.0f / d1) {
            return n1 * t * t;
        } else if (t < 2.0f / d1) {
            t -= 1.5f / d1;
            return n1 * t * t + 0.75f;
        } else if (t < 2.5f / d1) {
            t -= 2.25f / d1;
            return n1 * t * t + 0.9375f;
        } else {
            t -= 2.625f / d1;
            return n1 * t * t + 0.984375f;
        }
    }
    case Easing::EaseInBounce:
        return 1.0f - ApplyEasing(1.0f - t, Easing::EaseOutBounce);
    case Easing::EaseInOutBounce:
        return t < 0.5f
            ? (1.0f - ApplyEasing(1.0f - 2.0f * t, Easing::EaseOutBounce)) * 0.5f
            : (1.0f + ApplyEasing(2.0f * t - 1.0f, Easing::EaseOutBounce)) * 0.5f;
    case Easing::EaseInElastic: {
        constexpr float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
        return (t == 0.0f) ? 0.0f : ((t == 1.0f) ? 1.0f : -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * c4));
    }
    case Easing::EaseOutElastic: {
        constexpr float c4 = (2.0f * std::numbers::pi_v<float>) / 3.0f;
        return (t == 0.0f) ? 0.0f : ((t == 1.0f) ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f);
    }
    case Easing::EaseInOutElastic: {
        constexpr float c5 = (2.0f * std::numbers::pi_v<float>) / 4.5f;
        return (t == 0.0f) ? 0.0f : ((t == 1.0f) ? 1.0f : (t < 0.5f
            ? -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) * 0.5f
            : (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * c5)) * 0.5f + 1.0f));
    }
    default:
        return t;
    }
}

// ============================================================================
// 2. Animated Value Wrappers
// ============================================================================

class AnimatedFloat {
public:
    explicit AnimatedFloat(float initial = 0.0f, float speed = 10.0f)
        : m_current(initial), m_target(initial), m_speed(speed) {}

    void setTarget(float target) { m_target = target; }
    void snapTo(float value) { m_current = value; m_target = value; }
    void setSpeed(float speed) { m_speed = speed; }

    void update(float dt) {
        if (std::abs(m_current - m_target) < 0.0005f) {
            m_current = m_target;
        } else {
            float factor = 1.0f - std::exp(-m_speed * dt);
            m_current += (m_target - m_current) * factor;
            if (std::abs(m_current - m_target) < 0.0005f) {
                m_current = m_target;
            }
        }
    }

    float value() const { return m_current; }
    float target() const { return m_target; }
    bool isAtTarget() const { return m_current == m_target; }

private:
    float m_current{ 0.0f };
    float m_target{ 0.0f };
    float m_speed{ 10.0f };
};

class AnimatedColor {
public:
    explicit AnimatedColor(Color initial = Color::White(), float speed = 10.0f)
        : m_current(initial), m_target(initial), m_speed(speed) {}

    void setTarget(Color target) { m_target = target; }
    void snapTo(Color value) { m_current = value; m_target = value; }
    void setSpeed(float speed) { m_speed = speed; }

    void update(float dt) {
        if (isAtTarget()) return;
        float factor = 1.0f - std::exp(-m_speed * dt);
        m_current = Color::Lerp(m_current, m_target, factor);
        constexpr int kSnapThreshold = 1;
        if (std::abs(static_cast<int>(m_current.r) - static_cast<int>(m_target.r)) <= kSnapThreshold &&
            std::abs(static_cast<int>(m_current.g) - static_cast<int>(m_target.g)) <= kSnapThreshold &&
            std::abs(static_cast<int>(m_current.b) - static_cast<int>(m_target.b)) <= kSnapThreshold &&
            std::abs(static_cast<int>(m_current.a) - static_cast<int>(m_target.a)) <= kSnapThreshold)
        {
            m_current = m_target;
        }
    }

    Color value() const { return m_current; }
    Color target() const { return m_target; }
    bool isAtTarget() const {
        return m_current.r == m_target.r && m_current.g == m_target.g &&
               m_current.b == m_target.b && m_current.a == m_target.a;
    }

private:
    Color m_current{ Color::White() };
    Color m_target{ Color::White() };
    float m_speed{ 10.0f };
};

// ============================================================================
// 3. Second-Order Damped Spring Physics & Presets
// ============================================================================

namespace SpringPreset {
    struct Config {
        float stiffness;
        float damping;
        float mass{ 1.0f };
    };

    inline constexpr Config Gentle{ 120.0f, 22.0f, 1.0f };
    inline constexpr Config Snappy{ 230.0f, 28.0f, 1.0f };
    inline constexpr Config Wobbly{ 170.0f, 14.0f, 1.0f };
}

class PERFUI_API SpringFloat {
public:
    explicit SpringFloat(float initial = 0.0f, float stiffness = 170.0f, float damping = 14.0f, float mass = 1.0f)
        : m_current(initial), m_target(initial), m_stiffness(stiffness), m_damping(damping), m_mass(mass) {}

    void setTarget(float target) { m_target = target; }
    void snapTo(float val) { m_current = val; m_target = val; m_velocity = 0.0f; }
    void setStiffness(float k) { m_stiffness = k; }
    void setDamping(float c) { m_damping = c; }
    void setMass(float m) { m_mass = (std::max)(0.001f, m); }
    void setRestThreshold(float posThreshold, float velThreshold) {
        m_posThreshold = posThreshold;
        m_velThreshold = velThreshold;
    }
    void setRestThreshold(float epsilon) {
        m_posThreshold = epsilon;
        m_velThreshold = epsilon;
    }
    float restPosThreshold() const { return m_posThreshold; }
    float restVelThreshold() const { return m_velThreshold; }

    void applyPreset(const SpringPreset::Config& preset) {
        m_stiffness = preset.stiffness;
        m_damping = preset.damping;
        m_mass = preset.mass;
    }

    void update(float dt) {
        if (isResting()) return;
        if (std::isnan(m_current) || std::isinf(m_current)) {
            m_current = m_target;
            m_velocity = 0.0f;
            return;
        }

        // Substep to ensure high stability even with frame spikes
        float remaining = (std::min)(dt, 0.1f);
        constexpr float kMaxStep = 1.0f / 120.0f;
        while (remaining > 0.0f) {
            float h = (std::min)(remaining, kMaxStep);
            float force = -m_stiffness * (m_current - m_target) - m_damping * m_velocity;
            float accel = force / m_mass;
            m_velocity += accel * h;
            m_current += m_velocity * h;
            remaining -= h;
        }

        if (std::abs(m_current - m_target) < m_posThreshold && std::abs(m_velocity) < m_velThreshold) {
            m_current = m_target;
            m_velocity = 0.0f;
        }
    }

    float value() const { return m_current; }
    float target() const { return m_target; }
    float velocity() const { return m_velocity; }
    bool isResting() const {
        return m_current == m_target && m_velocity == 0.0f;
    }

private:
    float m_current{ 0.0f };
    float m_target{ 0.0f };
    float m_velocity{ 0.0f };
    float m_stiffness{ 170.0f };
    float m_damping{ 14.0f };
    float m_mass{ 1.0f };
    float m_posThreshold{ 0.01f };
    float m_velThreshold{ 0.01f };
};

// ============================================================================
// 4. Weak Element Reference (Weak Observer for Safe Cancellation)
// ============================================================================

class PERFUI_API WeakElementRef {
public:
    WeakElementRef() = default;
    explicit WeakElementRef(UIElement* element);

    UIElement* get() const;
    UIElement* lock() const { return get(); }
    bool isValid() const;
    bool wasBound() const { return m_ptr != nullptr; }
    explicit operator bool() const { return isValid(); }

private:
    UIElement* m_ptr{ nullptr };
    std::weak_ptr<bool> m_aliveToken;
};

// ============================================================================
// 5. Element Animator (Tween API for UIElement)
// ============================================================================

class PERFUI_API ElementAnimator {
public:
    template <typename T>
    struct Track {
        bool active{ false };
        bool fromCaptured{ false };
        T fromValue{};
        T toValue{};
        float duration{ 0.3f };
        float delay{ 0.0f };
        float elapsed{ 0.0f };
        Easing easing{ Easing::EaseOutCubic };
        bool finished{ false };
        bool reverse{ false };

        void setup(const T& to, float dur, Easing eas, float del = 0.0f) {
            toValue = to;
            duration = (std::max)(0.001f, dur);
            easing = eas;
            delay = (std::max)(0.0f, del);
            elapsed = 0.0f;
            active = true;
            finished = false;
            fromCaptured = false;
            reverse = false;
        }

        void reset() {
            active = false;
            fromCaptured = false;
            finished = false;
            elapsed = 0.0f;
            delay = 0.0f;
            reverse = false;
        }

        bool isFinished() const {
            return !active || finished;
        }

        float currentProgress() const {
            if (!active) return 1.0f;
            if (delay > 0.0f) return 0.0f;
            float raw = (std::clamp)(elapsed / duration, 0.0f, 1.0f);
            return reverse ? (1.0f - raw) : raw;
        }
    };

    explicit ElementAnimator(UIElement* element);
    virtual ~ElementAnimator() = default;

    ElementAnimator& positionMode(AnimPositionMode mode) { m_positionMode = mode; return *this; }
    AnimPositionMode positionMode() const { return m_positionMode; }

    ElementAnimator& position(Point to, float duration, Easing easing = Easing::EaseOutCubic);
    ElementAnimator& moveBy(Point delta, float duration, Easing easing = Easing::EaseOutCubic);
    ElementAnimator& scale(Point to, float duration, Easing easing = Easing::EaseOutCubic);
    ElementAnimator& scale(float to, float duration, Easing easing = Easing::EaseOutCubic);
    ElementAnimator& size(Dimensions to, float duration, Easing easing = Easing::EaseOutCubic);
    ElementAnimator& alpha(float to, float duration, Easing easing = Easing::Linear);
    ElementAnimator& renderOpacity(float to, float duration, Easing easing = Easing::Linear);
    ElementAnimator& color(Color to, float duration, Easing easing = Easing::Linear);

    ElementAnimator& delay(float seconds);
    ElementAnimator& repeat(int count, bool yoyo = false);
    ElementAnimator& onUpdate(std::function<void(float progress)> callback);
    ElementAnimator& onComplete(std::function<void()> callback);

    virtual void update(float dt);
    void stop();
    void finish();
    void cancelPosition() { m_trackPos.reset(); }
    void cancelScale() { m_trackScale.reset(); }
    void cancelSize() { m_trackSize.reset(); }
    void cancelAlpha() { m_trackAlpha.reset(); }
    void cancelRenderOpacity() { m_trackRenderOpacity.reset(); }
    void cancelColor() { m_trackColor.reset(); }
    virtual bool isFinished() const;

    const Track<Point>& trackPosition() const { return m_trackPos; }
    const Track<Point>& trackScale() const { return m_trackScale; }
    const Track<Dimensions>& trackSize() const { return m_trackSize; }
    const Track<float>& trackAlpha() const { return m_trackAlpha; }
    const Track<float>& trackRenderOpacity() const { return m_trackRenderOpacity; }
    const Track<Color>& trackColor() const { return m_trackColor; }

protected:
    WeakElementRef m_element;

    Track<Point> m_trackPos;
    bool m_posIsRelative{ false };
    Point m_posDelta{};
    AnimPositionMode m_positionMode{ AnimPositionMode::RenderOffset };

    Track<Point> m_trackScale;
    Track<Dimensions> m_trackSize;
    Track<float> m_trackAlpha;
    Track<float> m_trackRenderOpacity;
    Track<Color> m_trackColor;

    float m_pendingDelay{ 0.0f };
    int m_repeatCount{ 0 };
    int m_currentRepeat{ 0 };
    bool m_yoyo{ false };
    bool m_finished{ true };

    std::function<void(float)> m_onUpdate;
    std::function<void()> m_onComplete;
};

// ============================================================================
// 6. Element Spring (Spring Physics API for UIElement)
// ============================================================================

class PERFUI_API ElementSpring {
public:
    explicit ElementSpring(UIElement* element);
    ~ElementSpring() = default;

    ElementSpring& positionMode(SpringPositionMode mode) { m_posMode = mode; return *this; }
    SpringPositionMode positionMode() const { return m_posMode; }

    ElementSpring& position(Point target);
    ElementSpring& size(Dimensions target);
    ElementSpring& scale(Point target);
    ElementSpring& scale(float target);
    ElementSpring& alpha(float target);
    ElementSpring& color(Color target);

    ElementSpring& preset(const SpringPreset::Config& config);
    ElementSpring& gentle() { return preset(SpringPreset::Gentle); }
    ElementSpring& snappy() { return preset(SpringPreset::Snappy); }
    ElementSpring& wobbly() { return preset(SpringPreset::Wobbly); }

    ElementSpring& stiffness(float k);
    ElementSpring& damping(float c);
    ElementSpring& mass(float m);

    ElementSpring& positionStiffness(float k);
    ElementSpring& positionDamping(float c);
    ElementSpring& sizeStiffness(float k);
    ElementSpring& sizeDamping(float c);
    ElementSpring& scaleStiffness(float k);
    ElementSpring& scaleDamping(float c);
    ElementSpring& alphaStiffness(float k);
    ElementSpring& alphaDamping(float c);
    ElementSpring& colorStiffness(float k);
    ElementSpring& colorDamping(float c);

    ElementSpring& onRest(std::function<void()> callback);

    void update(float dt);
    bool isResting() const;
    void finish();
    void stop();
    void cancelPosition() { m_animPos = false; }
    void cancelScale() { m_animScale = false; }
    void cancelSize() { m_animSize = false; }
    void cancelAlpha() { m_animAlpha = false; }
    void cancelColor() { m_animColor = false; }

private:
    WeakElementRef m_element;

    SpringPositionMode m_posMode{ SpringPositionMode::RenderOffset };
    bool m_animPos{ false };
    bool m_animSize{ false };
    bool m_animScale{ false };
    bool m_animAlpha{ false };
    bool m_animColor{ false };

    SpringFloat m_springX;
    SpringFloat m_springY;
    SpringFloat m_springW;
    SpringFloat m_springH;
    SpringFloat m_springScaleX{ 1.0f };
    SpringFloat m_springScaleY{ 1.0f };
    SpringFloat m_springAlpha;

    SpringFloat m_springColorR{ 255.0f };
    SpringFloat m_springColorG{ 255.0f };
    SpringFloat m_springColorB{ 255.0f };
    SpringFloat m_springColorA{ 255.0f };

    std::function<void()> m_onRest;
    bool m_wasResting{ true };
};

// ============================================================================
// 7. Timeline (Sequencer and Animation Groups with Weak References)
// ============================================================================

class PERFUI_API Timeline {
public:
    enum class State {
        Stopped,
        Playing,
        Paused,
        Finished
    };

    enum class Mode {
        Sequence,
        Parallel
    };

    struct Step {
        enum class Type {
            Animation,
            Wait,
            Action,
            Group
        } type{ Type::Wait };

        WeakElementRef element;
        std::function<void(UIElement*)> onStart;
        std::function<void(UIElement*, float)> onUpdate; // progress 0..1
        std::function<void(UIElement*)> onFinish;
        float duration{ 0.0f };
        float elapsed{ 0.0f };
        float delay{ 0.0f };
        float delayRemaining{ 0.0f };
        Easing easing{ Easing::Linear };
        bool started{ false };
        bool finished{ false };
        std::shared_ptr<Timeline> subTimeline;
        std::function<void()> onReset;
    };

    Timeline();
    ~Timeline() = default;

    void setScheduler(AnimationScheduler* sched) { m_scheduler = sched; }
    void setSelf(std::weak_ptr<Timeline> self) { m_self = std::move(self); }

    Timeline& sequence();
    Timeline& parallel();

    Timeline& group(Mode mode, std::function<void(Timeline&)> setup);
    Timeline& sequence(std::function<void(Timeline&)> setup);
    Timeline& parallel(std::function<void(Timeline&)> setup);

    Timeline& stagger(const std::vector<UIElement*>& elements, float delayStep,
                      std::function<void(UIElement*, Timeline&)> fn);
    Timeline& stagger(const std::vector<UIElement*>& elements, float delayStep,
                      std::function<void(UIElement*, size_t, Timeline&)> fn);

    Timeline& delay(float seconds);
    Timeline& wait(float seconds);
    Timeline& call(std::function<void()> action);
    Timeline& animate(UIElement* element, float duration, Easing easing,
                      std::function<void(UIElement*, float t)> onUpdate,
                      std::function<void(UIElement*)> onComplete = nullptr);

    Timeline& fadeTo(UIElement* element, float targetAlpha, float duration, Easing easing = Easing::Linear);
    Timeline& moveTo(UIElement* element, Point targetPos, float duration, Easing easing = Easing::EaseOutCubic);
    Timeline& sizeTo(UIElement* element, Dimensions targetSize, float duration, Easing easing = Easing::EaseOutCubic);

    Timeline& loop(bool enable = true);
    Timeline& onComplete(std::function<void()> callback);

    Timeline& timeScale(float scale);
    float timeScale() const { return m_timeScale; }

    Timeline& reverse();
    bool isReversed() const { return m_reversed; }

    void seek(float seconds);
    float totalDuration() const;

    void play();
    void pause();
    void stop();
    void restart();

    void update(float dt);
    bool isFinished() const;
    State state() const { return m_state; }

private:
    Mode m_mode{ Mode::Sequence };
    State m_state{ State::Stopped };
    bool m_loop{ false };
    bool m_reversed{ false };
    float m_timeScale{ 1.0f };
    float m_pendingStepDelay{ 0.0f };
    size_t m_currentStep{ 0 };
    std::vector<Step> m_steps;
    std::function<void()> m_onComplete;
    AnimationScheduler* m_scheduler{ nullptr };
    std::weak_ptr<Timeline> m_self;
};

} // namespace PerfUI
