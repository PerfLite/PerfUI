#pragma once

#include "Types.h"
#include <algorithm>
#include <cmath>

namespace PerfUI {

enum class Easing {
    Linear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
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
    case Easing::EaseOutElastic: {
        constexpr float c4 = (2.0f * 3.14159265f) / 3.0f;
        return t == 0.0f ? 0.0f : (t == 1.0f ? 1.0f : std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f);
    }
    }
    return t;
}

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

} // namespace PerfUI
