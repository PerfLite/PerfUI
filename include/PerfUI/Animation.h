#pragma once

#include "Types.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace PerfUI {

class UIElement;

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
    EaseOutElastic
};

inline float ApplyEasing(float t, Easing easing = Easing::EaseOutCubic) {
    t = (std::clamp)(t, 0.0f, 1.0f);
    switch (easing) {
    case Easing::Linear:
        return t;
    case Easing::EaseInQuad:
        return t * t;
    case Easing::EaseOutQuad:
        return 1.0f - (1.0f - t) * (1.0f - t);
    case Easing::EaseInOutQuad:
        return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
    case Easing::EaseInCubic:
        return t * t * t;
    case Easing::EaseOutCubic: {
        float inv = 1.0f - t;
        return 1.0f - inv * inv * inv;
    }
    case Easing::EaseInOutCubic:
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;

    case Easing::EaseInSine:
        return 1.0f - std::cos((t * 3.14159265f) * 0.5f);
    case Easing::EaseOutSine:
        return std::sin((t * 3.14159265f) * 0.5f);
    case Easing::EaseInOutSine:
        return -(std::cos(3.14159265f * t) - 1.0f) * 0.5f;

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
        return t < 0.5f
            ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) * 0.5f
            : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) * 0.5f;
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

    case Easing::EaseOutElastic: {
        constexpr float c4 = (2.0f * 3.14159265f) / 3.0f;
        return t == 0.0f ? 0.0f : (t == 1.0f ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f);
    }
    }
    return t;
}

// ============================================================================
// 2. Exponential Lerp Helpers (Legacy Compat)
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
            float factor = (std::min)(1.0f, dt * m_speed);
            m_current += (m_target - m_current) * factor;
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
        if (m_current.r == m_target.r && m_current.g == m_target.g &&
            m_current.b == m_target.b && m_current.a == m_target.a) {
            return;
        }
        float factor = (std::min)(1.0f, dt * m_speed);
        m_current = Color::Lerp(m_current, m_target, factor);
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
// 3. Second-Order Damped Spring Physics
// ============================================================================

class PERFUI_API SpringFloat {
public:
    explicit SpringFloat(float initial = 0.0f, float stiffness = 170.0f, float damping = 14.0f, float mass = 1.0f)
        : m_current(initial), m_target(initial), m_stiffness(stiffness), m_damping(damping), m_mass(mass) {}

    void setTarget(float target) { m_target = target; }
    void snapTo(float val) { m_current = val; m_target = val; m_velocity = 0.0f; }
    void setStiffness(float k) { m_stiffness = k; }
    void setDamping(float c) { m_damping = c; }
    void setMass(float m) { m_mass = (std::max)(0.001f, m); }

    void update(float dt) {
        if (isResting()) return;

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

        if (std::abs(m_current - m_target) < 0.01f && std::abs(m_velocity) < 0.01f) {
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
    explicit ElementAnimator(UIElement* element);
    virtual ~ElementAnimator() = default;

    ElementAnimator& position(Point to, float duration, Easing easing = Easing::EaseOutCubic);
    ElementAnimator& moveBy(Point delta, float duration, Easing easing = Easing::EaseOutCubic);
    ElementAnimator& size(Dimensions to, float duration, Easing easing = Easing::EaseOutCubic);
    ElementAnimator& alpha(float to, float duration, Easing easing = Easing::Linear);
    ElementAnimator& color(Color to, float duration, Easing easing = Easing::Linear);

    ElementAnimator& delay(float seconds);
    ElementAnimator& repeat(int count, bool yoyo = false);
    ElementAnimator& onUpdate(std::function<void(float progress)> callback);
    ElementAnimator& onComplete(std::function<void()> callback);

    void update(float dt);
    void stop();
    bool isFinished() const;

protected:
    WeakElementRef m_element;

    bool m_hasPosition{ false };
    Point m_fromPos{};
    Point m_toPos{};

    bool m_hasSize{ false };
    Dimensions m_fromSize{};
    Dimensions m_toSize{};

    bool m_hasAlpha{ false };
    float m_fromAlpha{ 1.0f };
    float m_toAlpha{ 1.0f };

    bool m_hasColor{ false };
    Color m_fromColor{};
    Color m_toColor{};

    float m_duration{ 0.3f };
    float m_delay{ 0.0f };
    float m_elapsed{ 0.0f };
    Easing m_easing{ Easing::EaseOutCubic };

    int m_repeatCount{ 0 };
    int m_currentRepeat{ 0 };
    bool m_yoyo{ false };
    bool m_reverse{ false };

    bool m_started{ false };
    bool m_finished{ false };

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

    ElementSpring& position(Point target);
    ElementSpring& size(Dimensions target);
    ElementSpring& alpha(float target);

    ElementSpring& stiffness(float k);
    ElementSpring& damping(float c);
    ElementSpring& mass(float m);

    void update(float dt);
    bool isResting() const;
    void stop();

private:
    WeakElementRef m_element;

    bool m_animPos{ false };
    bool m_animSize{ false };
    bool m_animAlpha{ false };

    SpringFloat m_springX;
    SpringFloat m_springY;
    SpringFloat m_springW;
    SpringFloat m_springH;
    SpringFloat m_springAlpha;
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
            Action
        } type{ Type::Wait };

        WeakElementRef element;
        std::function<void(UIElement*)> onStart;
        std::function<void(UIElement*, float)> onUpdate; // progress 0..1
        std::function<void(UIElement*)> onFinish;
        float duration{ 0.0f };
        float elapsed{ 0.0f };
        Easing easing{ Easing::Linear };
        bool started{ false };
        bool finished{ false };
    };

    Timeline();
    ~Timeline() = default;

    Timeline& sequence();
    Timeline& parallel();

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
    size_t m_currentStep{ 0 };
    std::vector<Step> m_steps;
    std::function<void()> m_onComplete;
};

} // namespace PerfUI
