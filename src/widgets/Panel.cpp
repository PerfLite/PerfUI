#include "PerfUI/Panel.h"
#include "PerfUI/Theme.h"
#include "PerfUI/UIContext.h"
#include <mutex>
#include <unordered_map>

namespace {
struct PanelImageState {
    PerfUI::TextureId texture{ 0 };
    PerfUI::Point uv0{ 0.0f, 0.0f };
    PerfUI::Point uv1{ 1.0f, 1.0f };
    PerfUI::Insets sourceSlice{};
    PerfUI::Insets destinationSlice{};
};
std::mutex g_panelImageMutex;
std::unordered_map<const PerfUI::Panel*, PanelImageState> g_panelImages;

PanelImageState panelImage(const PerfUI::Panel* panel) {
    std::lock_guard lock(g_panelImageMutex);
    auto it = g_panelImages.find(panel);
    return it == g_panelImages.end() ? PanelImageState{} : it->second;
}
}

namespace PerfUI {

Panel::Panel(std::string name)
    : UIElement(std::move(name))
{
}

Panel::~Panel() {
    std::lock_guard lock(g_panelImageMutex);
    g_panelImages.erase(this);
}

TextureId Panel::backgroundImage() const { return panelImage(this).texture; }
Panel& Panel::backgroundImage(TextureId texture) {
    std::lock_guard lock(g_panelImageMutex);
    g_panelImages[this].texture = texture;
    return *this;
}
Panel& Panel::backgroundImageUV(Point uv0, Point uv1) {
    std::lock_guard lock(g_panelImageMutex);
    auto& image = g_panelImages[this];
    image.uv0 = uv0;
    image.uv1 = uv1;
    return *this;
}
Panel& Panel::backgroundImageSlice(Insets sourcePixels, Insets destinationPixels) {
    std::lock_guard lock(g_panelImageMutex);
    auto& image = g_panelImages[this];
    image.sourceSlice = sourcePixels;
    image.destinationSlice = destinationPixels;
    return *this;
}

void Panel::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    float effAlpha = effectiveOpacity();
    if (effAlpha <= 0.001f) return;

    auto applyAlpha = [effAlpha](Color c) -> Color {
        return Color(c.r, c.g, c.b, static_cast<uint8_t>(std::clamp(static_cast<float>(c.a) * effAlpha, 0.0f, 255.0f)));
    };

    WidgetState state = currentState();
    const auto& themePanel = Theme::Current().components.panel;
    BoxStyle themeBox = themePanel.resolveBox(state);
    BoxStyle resolvedBox = resolveBoxStyle(state, themeBox);

    Color bg = m_backgroundColor.has_value() 
        ? *m_backgroundColor 
        : resolvedBox.backgroundColor.value_or(Color::Transparent());
    Color border = m_borderColor.has_value() 
        ? *m_borderColor 
        : resolvedBox.borderColor.value_or(Color::Transparent());
    float borderW = m_borderWidth.has_value() 
        ? *m_borderWidth 
        : resolvedBox.borderWidth.value_or(0.0f);
    float radius = m_cornerRadius.has_value() 
        ? *m_cornerRadius 
        : resolvedBox.cornerRadius.value_or(0.0f);

    if (m_hasShadow && m_shadowColor.a > 0) {
        backend.drawShadow(m_bounds, radius, applyAlpha(m_shadowColor), m_shadowBlur, m_shadowOffset);
    } else if (resolvedBox.shadow.has_value() && !m_backgroundColor.has_value()) {
        backend.drawShadow(m_bounds, radius, applyAlpha(resolvedBox.shadow->color), resolvedBox.shadow->blur, resolvedBox.shadow->offset);
    }

    Color effBg = applyAlpha(bg);
    Color effBorder = applyAlpha(border);
    if (m_backdropBlur) {
        backend.drawBackdropBlur(m_bounds, radius, effBg, m_blurRadius);
        if (effBorder.a > 0 && borderW > 0.0f) {
            backend.drawRectOutline(m_bounds, effBorder, borderW, radius);
        }
    } else if (effBg.a > 0 || (effBorder.a > 0 && borderW > 0.0f)) {
        backend.drawRoundedRect(
            m_bounds,
            effBg,
            radius,
            effBorder,
            borderW
        );
    }

    const auto image = panelImage(this);
    if (image.texture != 0) {
        const Color imageTint{ 255, 255, 255, static_cast<uint8_t>(255.0f * effAlpha) };
        const auto textureSize = context() ? context()->getTextureSize(image.texture) : Dimensions{};
        const bool sliced = textureSize.width > 0.0f && textureSize.height > 0.0f &&
                            image.sourceSlice.horizontal() > 0.0f && image.sourceSlice.vertical() > 0.0f;
        if (!sliced) {
            backend.drawImageUV(image.texture, m_bounds, image.uv0, image.uv1, imageTint);
        } else {
            const float sourceW = textureSize.width;
            const float sourceH = textureSize.height;
            const float leftSrc = (std::clamp)(image.sourceSlice.left, 0.0f, sourceW * 0.5f);
            const float rightSrc = (std::clamp)(image.sourceSlice.right, 0.0f, sourceW * 0.5f);
            const float topSrc = (std::clamp)(image.sourceSlice.top, 0.0f, sourceH * 0.5f);
            const float bottomSrc = (std::clamp)(image.sourceSlice.bottom, 0.0f, sourceH * 0.5f);
            const float uSpan = image.uv1.x - image.uv0.x;
            const float vSpan = image.uv1.y - image.uv0.y;
            const float u[4]{ image.uv0.x, image.uv0.x + uSpan * leftSrc / sourceW,
                              image.uv1.x - uSpan * rightSrc / sourceW, image.uv1.x };
            const float v[4]{ image.uv0.y, image.uv0.y + vSpan * topSrc / sourceH,
                              image.uv1.y - vSpan * bottomSrc / sourceH, image.uv1.y };
            const float horizontalScale = (std::min)(1.0f, m_bounds.width /
                (std::max)(1.0f, image.destinationSlice.horizontal()));
            const float verticalScale = (std::min)(1.0f, m_bounds.height /
                (std::max)(1.0f, image.destinationSlice.vertical()));
            const float x[4]{ m_bounds.x, m_bounds.x + image.destinationSlice.left * horizontalScale,
                              m_bounds.x + m_bounds.width - image.destinationSlice.right * horizontalScale,
                              m_bounds.x + m_bounds.width };
            const float y[4]{ m_bounds.y, m_bounds.y + image.destinationSlice.top * verticalScale,
                              m_bounds.y + m_bounds.height - image.destinationSlice.bottom * verticalScale,
                              m_bounds.y + m_bounds.height };
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    const Rect part{ x[col], y[row], x[col + 1] - x[col], y[row + 1] - y[row] };
                    if (part.width > 0.0f && part.height > 0.0f) {
                        backend.drawImageUV(image.texture, part,
                                            { u[col], v[row] }, { u[col + 1], v[row + 1] }, imageTint);
                    }
                }
            }
        }
    }

    if (m_clipChildren) {
        backend.pushClipRect(m_bounds);
    }

    UIElement::render(backend);

    if (m_clipChildren) {
        backend.popClipRect();
    }
}

} // namespace PerfUI
