#include "PerfUI/ProgressBar.h"
#include <algorithm>
#include <cstdio>

namespace PerfUI {

// ============================================================================
// ProgressBarAnimator
// ============================================================================

ProgressBarAnimator::ProgressBarAnimator(ProgressBar* bar)
    : ElementAnimator(bar)
    , m_bar(bar)
{
}

ProgressBarAnimator& ProgressBarAnimator::fill(float target, float duration, Easing easing) {
    if (!m_bar) return *this;
    m_fromFill = m_bar->progress();
    m_toFill = (std::clamp)(target, 0.0f, 1.0f);
    m_fillDuration = (std::max)(0.001f, duration);
    m_fillElapsed = 0.0f;
    m_fillEasing = easing;
    m_animFill = true;
    return *this;
}

ProgressBarAnimator& ProgressBarAnimator::fillColor(Color target, float duration, Easing easing) {
    if (!m_bar) return *this;
    m_fromFillColor = m_bar->m_fillColor;
    m_toFillColor = target;
    m_colorDuration = (std::max)(0.001f, duration);
    m_colorElapsed = 0.0f;
    m_colorEasing = easing;
    m_animFillColor = true;
    return *this;
}

void ProgressBarAnimator::update(float dt) {
    ElementAnimator::update(dt);

    if (m_animFill && m_bar) {
        m_fillElapsed += dt;
        float p = (std::clamp)(m_fillElapsed / m_fillDuration, 0.0f, 1.0f);
        float curved = ApplyEasing(p, m_fillEasing);
        float curVal = m_fromFill + (m_toFill - m_fromFill) * curved;
        m_bar->m_progress = curVal;
        m_bar->m_animProgress.snapTo(curVal);
        if (p >= 1.0f) {
            m_animFill = false;
        }
    }

    if (m_animFillColor && m_bar) {
        m_colorElapsed += dt;
        float p = (std::clamp)(m_colorElapsed / m_colorDuration, 0.0f, 1.0f);
        float curved = ApplyEasing(p, m_colorEasing);
        m_bar->m_fillColor = Color::Lerp(m_fromFillColor, m_toFillColor, curved);
        if (p >= 1.0f) {
            m_animFillColor = false;
        }
    }
}

bool ProgressBarAnimator::isFinished() const {
    return ElementAnimator::isFinished() && !m_animFill && !m_animFillColor;
}

// ============================================================================
// ProgressBar
// ============================================================================

ProgressBar::ProgressBar(float progress, std::string name)
    : UIElement(std::move(name))
    , m_progress((std::clamp)(progress, 0.0f, 1.0f))
    , m_animProgress(m_progress, 7.0f)
{
}

ProgressBar& ProgressBar::progress(float p) {
    float clamped = (std::clamp)(p, 0.0f, 1.0f);
    if (m_progress != clamped) {
        m_progress = clamped;
        m_animProgress.setTarget(m_progress);
        markLayoutDirty();
    }
    return *this;
}

ProgressBarAnimator& ProgressBar::animate() {
    if (!m_barAnimator) {
        m_barAnimator = std::make_unique<ProgressBarAnimator>(this);
    }
    return *m_barAnimator;
}

ProgressBar& ProgressBar::animateFill(float target, float duration, Easing easing) {
    animate().fill(target, duration, easing);
    return *this;
}

void ProgressBar::update(float deltaTime) {
    UIElement::update(deltaTime);
    if (m_barAnimator && !m_barAnimator->isFinished()) {
        m_barAnimator->update(deltaTime);
    }
    m_animProgress.update(deltaTime);
}

void ProgressBar::measure(Dimensions availableSize) {
    float w = 0.0f;
    float h = m_barHeight;

    const auto& pad = layout().padding();
    h += pad.vertical();

    if (layout().width().mode == SizeMode::Fixed) {
        w = layout().width().value;
    } else if (layout().width().mode == SizeMode::Percent) {
        w = availableSize.width * (layout().width().value / 100.0f);
    }

    if (layout().height().mode == SizeMode::Fixed) {
        h = layout().height().value;
    }

    w = (std::clamp)(w, layout().minWidth(), layout().maxWidth());
    h = (std::clamp)(h, layout().minHeight(), layout().maxHeight());

    m_desiredSize = Dimensions{ w, h };
}

void ProgressBar::render(UIRenderBackend& backend) {
    if (!isVisible() || m_bounds.width <= 0.0f || m_bounds.height <= 0.0f) return;

    const auto& pad = layout().padding();
    Rect barRect = m_bounds.inset(pad);

    // Draw track
    backend.drawRoundedRect(barRect, m_trackColor, m_cornerRadius, m_trackBorder, 1.0f);

    // Draw fill
    float visualProgress = m_animProgress.value();
    if (visualProgress > 0.001f) {
        float fillW = barRect.width * visualProgress;
        Rect fillRect{ barRect.x, barRect.y, fillW, barRect.height };
        backend.drawRoundedRect(fillRect, m_fillColor, m_cornerRadius);
    }

    // Draw percentage text
    if (m_showLabel) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(visualProgress * 100.0f + 0.5f));

        Dimensions textDim = backend.measureText(buf, m_textStyle);
        float textX = barRect.x + (std::max)(0.0f, (barRect.width - textDim.width) * 0.5f);
        float textY = barRect.y + (std::max)(0.0f, (barRect.height - textDim.height) * 0.5f);

        backend.drawText(buf, Point{ textX, textY }, m_textStyle);
    }

    UIElement::render(backend);
}

} // namespace PerfUI
