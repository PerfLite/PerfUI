#include "PerfUI/Animation.h"
#include "PerfUI/UIElement.h"

namespace PerfUI {

// ============================================================================
// WeakElementRef
// ============================================================================

WeakElementRef::WeakElementRef(UIElement* element)
    : m_ptr(element)
    , m_aliveToken(element ? element->aliveToken() : std::weak_ptr<bool>())
{
}

UIElement* WeakElementRef::get() const {
    if (!m_ptr) return nullptr;
    auto token = m_aliveToken.lock();
    if (token && *token) {
        return m_ptr;
    }
    return nullptr;
}

bool WeakElementRef::isValid() const {
    return get() != nullptr;
}

// ============================================================================
// ElementAnimator
// ============================================================================

ElementAnimator::ElementAnimator(UIElement* element)
    : m_element(element)
{
}

ElementAnimator& ElementAnimator::position(Point to, float duration, Easing easing) {
    UIElement* el = m_element.lock();
    if (el) {
        m_fromPos = el->bounds().topLeft();
    }
    m_toPos = to;
    m_duration = (std::max)(0.001f, duration);
    m_easing = easing;
    m_hasPosition = true;
    m_started = true;
    m_finished = false;
    m_elapsed = 0.0f;
    return *this;
}

ElementAnimator& ElementAnimator::moveBy(Point delta, float duration, Easing easing) {
    UIElement* el = m_element.lock();
    Point current = el ? el->bounds().topLeft() : Point{ 0.0f, 0.0f };
    return position(current + delta, duration, easing);
}

ElementAnimator& ElementAnimator::size(Dimensions to, float duration, Easing easing) {
    UIElement* el = m_element.lock();
    if (el) {
        m_fromSize = el->bounds().size();
    }
    m_toSize = to;
    m_duration = (std::max)(0.001f, duration);
    m_easing = easing;
    m_hasSize = true;
    m_started = true;
    m_finished = false;
    m_elapsed = 0.0f;
    return *this;
}

ElementAnimator& ElementAnimator::alpha(float to, float duration, Easing easing) {
    UIElement* el = m_element.lock();
    if (el) {
        m_fromAlpha = el->opacity();
    }
    m_toAlpha = (std::clamp)(to, 0.0f, 1.0f);
    m_duration = (std::max)(0.001f, duration);
    m_easing = easing;
    m_hasAlpha = true;
    m_started = true;
    m_finished = false;
    m_elapsed = 0.0f;
    return *this;
}

ElementAnimator& ElementAnimator::color(Color to, float duration, Easing easing) {
    UIElement* el = m_element.lock();
    if (el) {
        if (el->style().box.normal.has_value() && el->style().box.normal->backgroundColor.has_value()) {
            m_fromColor = *el->style().box.normal->backgroundColor;
        } else {
            m_fromColor = Color::White();
        }
    }
    m_toColor = to;
    m_duration = (std::max)(0.001f, duration);
    m_easing = easing;
    m_hasColor = true;
    m_started = true;
    m_finished = false;
    m_elapsed = 0.0f;
    return *this;
}

ElementAnimator& ElementAnimator::delay(float seconds) {
    m_delay = (std::max)(0.0f, seconds);
    return *this;
}

ElementAnimator& ElementAnimator::repeat(int count, bool yoyo) {
    m_repeatCount = count;
    m_yoyo = yoyo;
    return *this;
}

ElementAnimator& ElementAnimator::onUpdate(std::function<void(float)> callback) {
    m_onUpdate = std::move(callback);
    return *this;
}

ElementAnimator& ElementAnimator::onComplete(std::function<void()> callback) {
    m_onComplete = std::move(callback);
    return *this;
}

void ElementAnimator::stop() {
    m_finished = true;
}

bool ElementAnimator::isFinished() const {
    return m_finished;
}

void ElementAnimator::update(float dt) {
    if (m_finished) return;

    UIElement* el = m_element.lock();
    if (!el) {
        m_finished = true;
        return;
    }

    if (m_delay > 0.0f) {
        m_delay -= dt;
        if (m_delay > 0.0f) return;
        dt = -m_delay;
        m_delay = 0.0f;
    }

    if (!m_started) {
        m_started = true;
        const auto& b = el->bounds();
        if (m_hasPosition) m_fromPos = b.topLeft();
        if (m_hasSize) m_fromSize = b.size();
        if (m_hasAlpha) m_fromAlpha = el->opacity();
        if (m_hasColor) {
            if (el->style().box.normal.has_value() && el->style().box.normal->backgroundColor.has_value()) {
                m_fromColor = *el->style().box.normal->backgroundColor;
            } else {
                m_fromColor = Color::White();
            }
        }
    }

    m_elapsed += dt;
    float rawProgress = (std::clamp)(m_elapsed / m_duration, 0.0f, 1.0f);
    float animT = m_reverse ? (1.0f - rawProgress) : rawProgress;
    float curvedT = ApplyEasing(animT, m_easing);

    // Apply animated properties
    Rect b = el->bounds();
    bool boundsChanged = false;

    if (m_hasPosition) {
        b.x = m_fromPos.x + (m_toPos.x - m_fromPos.x) * curvedT;
        b.y = m_fromPos.y + (m_toPos.y - m_fromPos.y) * curvedT;
        boundsChanged = true;
    }

    if (m_hasSize) {
        b.width = m_fromSize.width + (m_toSize.width - m_fromSize.width) * curvedT;
        b.height = m_fromSize.height + (m_toSize.height - m_fromSize.height) * curvedT;
        boundsChanged = true;
    }

    if (boundsChanged) {
        el->setBounds(b);
    }

    if (m_hasAlpha) {
        float curAlpha = m_fromAlpha + (m_toAlpha - m_fromAlpha) * curvedT;
        el->setOpacity(curAlpha);
    }

    if (m_hasColor) {
        Color curColor = Color::Lerp(m_fromColor, m_toColor, curvedT);
        if (!el->style().box.normal.has_value()) {
            el->style().box.normal = BoxStyle{};
        }
        el->style().box.normal->backgroundColor = curColor;
    }

    if (m_onUpdate) {
        m_onUpdate(curvedT);
    }

    if (rawProgress >= 1.0f) {
        if (m_repeatCount > 0 && m_currentRepeat < m_repeatCount) {
            m_currentRepeat++;
            m_elapsed = 0.0f;
            if (m_yoyo) {
                m_reverse = !m_reverse;
            }
        } else {
            m_finished = true;
            if (m_onComplete) {
                m_onComplete();
            }
        }
    }
}

// ============================================================================
// ElementSpring
// ============================================================================

ElementSpring::ElementSpring(UIElement* element)
    : m_element(element)
    , m_springX(0.0f)
    , m_springY(0.0f)
    , m_springW(0.0f)
    , m_springH(0.0f)
    , m_springAlpha(1.0f)
{
}

ElementSpring& ElementSpring::position(Point target) {
    UIElement* el = m_element.lock();
    if (el) {
        if (!m_animPos) {
            m_springX.snapTo(el->bounds().x);
            m_springY.snapTo(el->bounds().y);
        }
    }
    m_springX.setTarget(target.x);
    m_springY.setTarget(target.y);
    m_animPos = true;
    return *this;
}

ElementSpring& ElementSpring::size(Dimensions target) {
    UIElement* el = m_element.lock();
    if (el) {
        if (!m_animSize) {
            m_springW.snapTo(el->bounds().width);
            m_springH.snapTo(el->bounds().height);
        }
    }
    m_springW.setTarget(target.width);
    m_springH.setTarget(target.height);
    m_animSize = true;
    return *this;
}

ElementSpring& ElementSpring::alpha(float target) {
    UIElement* el = m_element.lock();
    if (el) {
        if (!m_animAlpha) {
            m_springAlpha.snapTo(el->opacity());
        }
    }
    m_springAlpha.setTarget(target);
    m_animAlpha = true;
    return *this;
}

ElementSpring& ElementSpring::stiffness(float k) {
    m_springX.setStiffness(k);
    m_springY.setStiffness(k);
    m_springW.setStiffness(k);
    m_springH.setStiffness(k);
    m_springAlpha.setStiffness(k);
    return *this;
}

ElementSpring& ElementSpring::damping(float c) {
    m_springX.setDamping(c);
    m_springY.setDamping(c);
    m_springW.setDamping(c);
    m_springH.setDamping(c);
    m_springAlpha.setDamping(c);
    return *this;
}

ElementSpring& ElementSpring::mass(float m) {
    m_springX.setMass(m);
    m_springY.setMass(m);
    m_springW.setMass(m);
    m_springH.setMass(m);
    m_springAlpha.setMass(m);
    return *this;
}

void ElementSpring::update(float dt) {
    if (isResting()) return;

    UIElement* el = m_element.lock();
    if (!el) {
        stop();
        return;
    }

    Rect b = el->bounds();
    bool boundsChanged = false;

    if (m_animPos) {
        m_springX.update(dt);
        m_springY.update(dt);
        b.x = m_springX.value();
        b.y = m_springY.value();
        boundsChanged = true;
        if (m_springX.isResting() && m_springY.isResting()) {
            m_animPos = false;
        }
    }

    if (m_animSize) {
        m_springW.update(dt);
        m_springH.update(dt);
        b.width = m_springW.value();
        b.height = m_springH.value();
        boundsChanged = true;
        if (m_springW.isResting() && m_springH.isResting()) {
            m_animSize = false;
        }
    }

    if (boundsChanged) {
        el->setBounds(b);
    }

    if (m_animAlpha) {
        m_springAlpha.update(dt);
        el->setOpacity(m_springAlpha.value());
        if (m_springAlpha.isResting()) {
            m_animAlpha = false;
        }
    }
}

bool ElementSpring::isResting() const {
    return (!m_animPos || (m_springX.isResting() && m_springY.isResting())) &&
           (!m_animSize || (m_springW.isResting() && m_springH.isResting())) &&
           (!m_animAlpha || m_springAlpha.isResting());
}

void ElementSpring::stop() {
    m_animPos = false;
    m_animSize = false;
    m_animAlpha = false;
}

// ============================================================================
// Timeline
// ============================================================================

Timeline::Timeline() = default;

Timeline& Timeline::sequence() {
    m_mode = Mode::Sequence;
    return *this;
}

Timeline& Timeline::parallel() {
    m_mode = Mode::Parallel;
    return *this;
}

Timeline& Timeline::wait(float seconds) {
    Step step;
    step.type = Step::Type::Wait;
    step.duration = (std::max)(0.0f, seconds);
    m_steps.push_back(std::move(step));
    return *this;
}

Timeline& Timeline::call(std::function<void()> action) {
    Step step;
    step.type = Step::Type::Action;
    step.duration = 0.0f;
    step.onStart = [act = std::move(action)](UIElement*) {
        if (act) act();
    };
    m_steps.push_back(std::move(step));
    return *this;
}

Timeline& Timeline::animate(UIElement* element, float duration, Easing easing,
                            std::function<void(UIElement*, float t)> onUpdate,
                            std::function<void(UIElement*)> onComplete)
{
    Step step;
    step.type = Step::Type::Animation;
    step.element = WeakElementRef(element);
    step.duration = (std::max)(0.001f, duration);
    step.easing = easing;
    step.onUpdate = std::move(onUpdate);
    step.onFinish = std::move(onComplete);
    m_steps.push_back(std::move(step));
    return *this;
}

Timeline& Timeline::fadeTo(UIElement* element, float targetAlpha, float duration, Easing easing) {
    auto startAlpha = std::make_shared<float>(-1.0f);
    Step step;
    step.type = Step::Type::Animation;
    step.element = WeakElementRef(element);
    step.duration = (std::max)(0.001f, duration);
    step.easing = easing;
    step.onStart = [startAlpha](UIElement* el) {
        if (el) *startAlpha = el->opacity();
    };
    step.onUpdate = [targetAlpha, startAlpha](UIElement* el, float t) {
        if (!el) return;
        if (*startAlpha < 0.0f) *startAlpha = el->opacity();
        float a = *startAlpha + (targetAlpha - *startAlpha) * t;
        el->setOpacity(a);
    };
    m_steps.push_back(std::move(step));
    return *this;
}

Timeline& Timeline::moveTo(UIElement* element, Point targetPos, float duration, Easing easing) {
    auto startPos = std::make_shared<Point>(-9999.0f, -9999.0f);
    Step step;
    step.type = Step::Type::Animation;
    step.element = WeakElementRef(element);
    step.duration = (std::max)(0.001f, duration);
    step.easing = easing;
    step.onStart = [startPos](UIElement* el) {
        if (el) *startPos = el->bounds().topLeft();
    };
    step.onUpdate = [targetPos, startPos](UIElement* el, float t) {
        if (!el) return;
        if (startPos->x < -9000.0f) *startPos = el->bounds().topLeft();
        Point cur = *startPos + (targetPos - *startPos) * t;
        el->setBounds(Rect{ cur.x, cur.y, el->bounds().width, el->bounds().height });
    };
    m_steps.push_back(std::move(step));
    return *this;
}

Timeline& Timeline::sizeTo(UIElement* element, Dimensions targetSize, float duration, Easing easing) {
    auto startSize = std::make_shared<Dimensions>(-1.0f, -1.0f);
    Step step;
    step.type = Step::Type::Animation;
    step.element = WeakElementRef(element);
    step.duration = (std::max)(0.001f, duration);
    step.easing = easing;
    step.onStart = [startSize](UIElement* el) {
        if (el) *startSize = el->bounds().size();
    };
    step.onUpdate = [targetSize, startSize](UIElement* el, float t) {
        if (!el) return;
        if (startSize->width < 0.0f) *startSize = el->bounds().size();
        float w = startSize->width + (targetSize.width - startSize->width) * t;
        float h = startSize->height + (targetSize.height - startSize->height) * t;
        el->setBounds(Rect{ el->bounds().x, el->bounds().y, w, h });
    };
    m_steps.push_back(std::move(step));
    return *this;
}

Timeline& Timeline::loop(bool enable) {
    m_loop = enable;
    return *this;
}

Timeline& Timeline::onComplete(std::function<void()> callback) {
    m_onComplete = std::move(callback);
    return *this;
}

void Timeline::play() {
    m_state = State::Playing;
}

void Timeline::pause() {
    if (m_state == State::Playing) {
        m_state = State::Paused;
    }
}

void Timeline::stop() {
    m_state = State::Stopped;
    m_currentStep = 0;
    for (auto& step : m_steps) {
        step.elapsed = 0.0f;
        step.started = false;
        step.finished = false;
    }
}

void Timeline::restart() {
    stop();
    play();
}

bool Timeline::isFinished() const {
    return m_state == State::Finished;
}

void Timeline::update(float dt) {
    if (m_state != State::Playing || m_steps.empty()) return;

    if (m_mode == Mode::Sequence) {
        while (m_currentStep < m_steps.size() && dt > 0.0f) {
            auto& step = m_steps[m_currentStep];
            UIElement* el = step.element.lock();

            // Weak reference check: if an element was bound but is now destroyed, skip step cleanly
            if (step.element.isValid() && !el) {
                step.finished = true;
                m_currentStep++;
                continue;
            }

            if (!step.started) {
                step.started = true;
                if (step.onStart) {
                    step.onStart(el);
                }
            }

            if (step.duration <= 0.0f) {
                step.finished = true;
                if (step.onFinish) {
                    step.onFinish(el);
                }
                m_currentStep++;
                continue;
            }

            step.elapsed += dt;
            float progress = (std::clamp)(step.elapsed / step.duration, 0.0f, 1.0f);
            float curvedT = ApplyEasing(progress, step.easing);

            if (step.onUpdate) {
                step.onUpdate(el, curvedT);
            }

            if (progress >= 1.0f) {
                step.finished = true;
                if (step.onFinish) {
                    step.onFinish(el);
                }
                dt = step.elapsed - step.duration; // Carry remainder to next step
                m_currentStep++;
            } else {
                break; // Current step still progressing
            }
        }

        if (m_currentStep >= m_steps.size()) {
            if (m_loop) {
                restart();
            } else {
                m_state = State::Finished;
                if (m_onComplete) {
                    m_onComplete();
                }
            }
        }
    } else { // Mode::Parallel
        bool allDone = true;
        for (auto& step : m_steps) {
            if (step.finished) continue;

            UIElement* el = step.element.lock();
            if (step.element.isValid() && !el) {
                step.finished = true;
                continue;
            }

            if (!step.started) {
                step.started = true;
                if (step.onStart) {
                    step.onStart(el);
                }
            }

            if (step.duration <= 0.0f) {
                step.finished = true;
                if (step.onFinish) {
                    step.onFinish(el);
                }
                continue;
            }

            step.elapsed += dt;
            float progress = (std::clamp)(step.elapsed / step.duration, 0.0f, 1.0f);
            float curvedT = ApplyEasing(progress, step.easing);

            if (step.onUpdate) {
                step.onUpdate(el, curvedT);
            }

            if (progress >= 1.0f) {
                step.finished = true;
                if (step.onFinish) {
                    step.onFinish(el);
                }
            } else {
                allDone = false;
            }
        }

        if (allDone) {
            if (m_loop) {
                restart();
            } else {
                m_state = State::Finished;
                if (m_onComplete) {
                    m_onComplete();
                }
            }
        }
    }
}

} // namespace PerfUI
