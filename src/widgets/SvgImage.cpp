#include "PerfUI/SvgImage.h"

namespace PerfUI {

SvgImage::SvgImage(std::string_view filePath, float targetWidth, float targetHeight, std::string name)
    : UIElement(std::move(name))
    , m_filePath(filePath)
    , m_targetWidth(targetWidth)
    , m_targetHeight(targetHeight)
{
    if (m_targetWidth > 0.0f && m_targetHeight > 0.0f) {
        setDesiredSize({ m_targetWidth, m_targetHeight });
    }
}

SvgImage::~SvgImage() {
    if (m_texture != 0 && m_loadedBackend) {
        m_loadedBackend->destroyTexture(m_texture);
        m_texture = 0;
    }
}

SvgImage& SvgImage::src(std::string_view path) {
    if (m_filePath != path) {
        m_filePath = std::string(path);
        m_svgContent.clear();
        m_dirty = true;
        markLayoutDirty();
    }
    return *this;
}

SvgImage& SvgImage::svgContent(std::string_view xml) {
    if (m_svgContent != xml) {
        m_svgContent = std::string(xml);
        m_filePath.clear();
        m_dirty = true;
        markLayoutDirty();
    }
    return *this;
}

SvgImage& SvgImage::targetSize(float width, float height) {
    if (m_targetWidth != width || m_targetHeight != height) {
        m_targetWidth = width;
        m_targetHeight = height;
        if (m_targetWidth > 0.0f && m_targetHeight > 0.0f) {
            setDesiredSize({ m_targetWidth, m_targetHeight });
        }
        m_dirty = true;
        markLayoutDirty();
    }
    return *this;
}

void SvgImage::ensureTextureLoaded(UIRenderBackend& backend) {
    if (!m_dirty && m_texture != 0) return;

    if (m_texture != 0 && m_loadedBackend) {
        m_loadedBackend->destroyTexture(m_texture);
        m_texture = 0;
    }

    float tw = m_targetWidth;
    float th = m_targetHeight;
    if (tw <= 0.0f && th <= 0.0f) {
        if (m_bounds.width > 0.0f && m_bounds.height > 0.0f) {
            tw = m_bounds.width;
            th = m_bounds.height;
        }
    }

    if (!m_filePath.empty()) {
        m_texture = backend.loadSvg(m_filePath, tw, th);
    } else if (!m_svgContent.empty()) {
        m_texture = backend.loadSvgFromMemory(m_svgContent, tw, th);
    }

    if (m_texture != 0) {
        m_loadedBackend = &backend;
        m_dirty = false;
        if (m_targetWidth <= 0.0f || m_targetHeight <= 0.0f) {
            Dimensions texSize = backend.getTextureSize(m_texture);
            if (texSize.width > 0.0f && texSize.height > 0.0f && desiredSize().width <= 0.0f) {
                setDesiredSize(texSize);
            }
        }
    }
}

void SvgImage::measure(Dimensions availableSize) {
    if (m_targetWidth > 0.0f && m_targetHeight > 0.0f) {
        setDesiredSize({ m_targetWidth, m_targetHeight });
    } else {
        UIElement::measure(availableSize);
    }
}

void SvgImage::render(UIRenderBackend& backend) {
    if (!isVisible()) return;

    ensureTextureLoaded(backend);

    float effAlpha = effectiveOpacity();
    if (m_texture != 0 && effAlpha > 0.001f) {
        Color effTint(m_tint.r, m_tint.g, m_tint.b, static_cast<uint8_t>(std::clamp(static_cast<float>(m_tint.a) * effAlpha, 0.0f, 255.0f)));
        backend.drawImage(m_texture, m_bounds, effTint);
    }

    UIElement::render(backend);
}

} // namespace PerfUI
