#pragma once

#include "UIElement.h"
#include "Types.h"
#include "UIRenderBackend.h"

namespace PerfUI {

class PERFUI_API Image : public UIElement {
public:
    explicit Image(TextureId texture = 0, std::string name = "Image");
    ~Image() override = default;

    TextureId texture() const { return m_texture; }
    Image& texture(TextureId id);

    Color tint() const { return m_tint; }
    Image& tint(Color c) { m_tint = c; return *this; }

    void render(UIRenderBackend& backend) override;

private:
    TextureId m_texture{ 0 };
    Color m_tint{ Color::White() };
};

} // namespace PerfUI
