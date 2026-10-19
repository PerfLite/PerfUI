#pragma once

#include "UIElement.h"
#include "Types.h"
#include "UIRenderBackend.h"
#include <string>
#include <string_view>

namespace PerfUI {

class PERFUI_API SvgImage : public UIElement {
public:
    explicit SvgImage(std::string_view filePath = "", float targetWidth = 0.0f, float targetHeight = 0.0f, std::string name = "SvgImage");
    ~SvgImage() override;

    SvgImage& src(std::string_view path);
    const std::string& src() const { return m_filePath; }

    SvgImage& svgContent(std::string_view xml);
    const std::string& svgContent() const { return m_svgContent; }

    SvgImage& tint(Color c) { m_tint = c; return *this; }
    Color tint() const { return m_tint; }

    SvgImage& targetSize(float width, float height);
    Dimensions targetSize() const { return Dimensions{ m_targetWidth, m_targetHeight }; }

    TextureId texture() const { return m_texture; }

    void measure(Dimensions availableSize) override;
    void render(UIRenderBackend& backend) override;

private:
    void ensureTextureLoaded(UIRenderBackend& backend);

    std::string m_filePath;
    std::string m_svgContent;
    float m_targetWidth{ 0.0f };
    float m_targetHeight{ 0.0f };
    Color m_tint{ Color::White() };

    TextureId m_texture{ 0 };
    UIRenderBackend* m_loadedBackend{ nullptr };
    bool m_dirty{ true };
};

} // namespace PerfUI
