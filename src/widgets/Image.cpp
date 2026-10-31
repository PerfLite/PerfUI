#include "PerfUI/Image.h"

namespace PerfUI {

Image::Image(TextureId texture, std::string name)
    : UIElement(std::move(name))
    , m_texture(texture)
{
}

Image& Image::texture(TextureId id) {
    if (m_texture != id) {
        m_texture = id;
        markLayoutDirty();
    }
    return *this;
}

Image& Image::uv(Point uv0, Point uv1) {
    m_uv0 = uv0;
    m_uv1 = uv1;
    return *this;
}

Image& Image::flipY(bool flip) {
    m_flipY = flip;
    return *this;
}

void Image::render(UIRenderBackend& backend) {
    if (!isVisible() || m_texture == 0) return;

    Point p0 = m_uv0;
    Point p1 = m_uv1;
    if (m_flipY) {
        std::swap(p0.y, p1.y);
    }

    if (p0 == Point(0.0f, 0.0f) && p1 == Point(1.0f, 1.0f)) {
        backend.drawImage(m_texture, m_bounds, m_tint);
    } else {
        backend.drawImageUV(m_texture, m_bounds, p0, p1, m_tint);
    }
    UIElement::render(backend);
}

} // namespace PerfUI
