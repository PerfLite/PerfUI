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

    Point uv0() const { return m_uv0; }
    Point uv1() const { return m_uv1; }
    Image& uv(Point uv0, Point uv1);

    bool flipY() const { return m_flipY; }
    Image& flipY(bool flip);

    void render(UIRenderBackend& backend) override;

private:
    TextureId m_texture{ 0 };
    Color m_tint{ Color::White() };
    Point m_uv0{ 0.0f, 0.0f };
    Point m_uv1{ 1.0f, 1.0f };
    bool m_flipY{ false };
};

} // namespace PerfUI
