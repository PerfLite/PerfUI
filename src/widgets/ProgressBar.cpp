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
        m_animProgress.snapTo(m_progress);
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

    // 1. Soft drop shadow behind bar
    if (m_dropShadow) {
        backend.drawShadow(barRect, m_cornerRadius, Color(0, 0, 0, 140), 3.0f, Point(0.0f, 1.5f));
    }

    // 2. Track background with outer border
    backend.drawRoundedRect(barRect, m_trackColor, m_cornerRadius, m_borderColor, m_borderWidth);

    // 3. Inner fill area
    float inset = (m_borderWidth > 0.0f) ? m_borderWidth : 0.0f;
    float innerX = barRect.x + inset;
    float innerY = barRect.y + inset;
    float innerW = (std::max)(1.0f, barRect.width - inset * 2.0f);
    float innerH = (std::max)(1.0f, barRect.height - inset * 2.0f);

    float visualProgress = m_animProgress.value();
    if (visualProgress > 0.001f) {
        float fillW = innerW * visualProgress;
        Rect fillRect{ innerX, innerY, fillW, innerH };
        backend.drawRect(fillRect, m_fillColor);

        // 3D Glassy Sheen & Specular Highlight
        if (m_glossy && fillW > 1.0f && innerH > 2.0f) {
            // Glossy top sheen (top 45%)
            backend.drawRect(Rect{ innerX, innerY, fillW, innerH * 0.45f }, Color(255, 255, 255, 40));

            // Specular highlight top line
            backend.drawLine(Point{ innerX, innerY + 0.5f }, Point{ innerX + fillW, innerY + 0.5f },
                             Color(255, 255, 255, 120), 1.0f);

            // Subtle bottom shade (bottom 35%)
            backend.drawRect(Rect{ innerX, innerY + innerH * 0.65f, fillW, innerH * 0.35f }, Color(0, 0, 0, 45));
        }
    }

    // 4. Center numeric values / text (with 8-directional solid outline for crisp legibility)
    std::string textToRender = m_text;
    if (textToRender.empty() && m_showLabel) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(visualProgress * 100.0f + 0.5f));
        textToRender = buf;
    }

    if (!textToRender.empty() && innerH >= 3.0f) {
        Dimensions textDim = backend.measureText(textToRender.c_str(), m_textStyle);
        float textX = innerX + (innerW - textDim.width) * 0.5f;
        float textY = innerY + (innerH - textDim.height) * 0.5f - 0.5f;

        TextStyle shadow = m_textStyle;
        shadow.color = Color(0, 0, 0, 255);
        backend.drawText(textToRender.c_str(), Point{ textX - 1.0f, textY }, shadow);
        backend.drawText(textToRender.c_str(), Point{ textX + 1.0f, textY }, shadow);
        backend.drawText(textToRender.c_str(), Point{ textX, textY - 1.0f }, shadow);
        backend.drawText(textToRender.c_str(), Point{ textX, textY + 1.0f }, shadow);
        backend.drawText(textToRender.c_str(), Point{ textX - 1.0f, textY - 1.0f }, shadow);
        backend.drawText(textToRender.c_str(), Point{ textX + 1.0f, textY - 1.0f }, shadow);
        backend.drawText(textToRender.c_str(), Point{ textX - 1.0f, textY + 1.0f }, shadow);
        backend.drawText(textToRender.c_str(), Point{ textX + 1.0f, textY + 1.0f }, shadow);
        backend.drawText(textToRender.c_str(), Point{ textX, textY }, m_textStyle);
    }

    // 5. Secondary right-aligned label (e.g. regen rate or bonus value)
    if (!m_secondaryText.empty() && innerH >= 3.0f) {
        Dimensions secDim = backend.measureText(m_secondaryText.c_str(), m_secondaryTextStyle);
        float secX = innerX + innerW - secDim.width - 5.0f;
        float secY = innerY + (innerH - secDim.height) * 0.5f - 0.5f;

        TextStyle secShadow = m_secondaryTextStyle;
        secShadow.color = Color(0, 0, 0, 255);
        backend.drawText(m_secondaryText.c_str(), Point{ secX - 1.0f, secY }, secShadow);
        backend.drawText(m_secondaryText.c_str(), Point{ secX + 1.0f, secY }, secShadow);
        backend.drawText(m_secondaryText.c_str(), Point{ secX, secY - 1.0f }, secShadow);
        backend.drawText(m_secondaryText.c_str(), Point{ secX, secY + 1.0f }, secShadow);
        backend.drawText(m_secondaryText.c_str(), Point{ secX, secY }, m_secondaryTextStyle);
    }

    UIElement::render(backend);
}

} // namespace PerfUI
