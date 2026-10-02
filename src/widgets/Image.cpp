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

void Image::render(UIRenderBackend& backend) {
    if (!isVisible() || m_texture == 0) return;

    backend.drawImage(m_texture, m_bounds, m_tint);
    UIElement::render(backend);
}

} // namespace PerfUI
