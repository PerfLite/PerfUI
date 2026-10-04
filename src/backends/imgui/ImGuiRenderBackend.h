#pragma once

#include "PerfUI/UIRenderBackend.h"
#include <unordered_map>
#include <mutex>

// Forward declare ImDrawList and ImFont so imgui.h is NOT exposed in this header
struct ImDrawList;
struct ImFont;

namespace PerfUI {

class ImGuiRenderBackend : public UIRenderBackend {
public:
    ImGuiRenderBackend();
    ~ImGuiRenderBackend() override;

    void setCustomDrawList(ImDrawList* drawList);

    void beginFrame() override;
    void endFrame() override;

    void pushClipRect(const Rect& rect) override;
    void popClipRect() override;

    void drawRect(const Rect& rect, Color fillColor) override;
    void drawRoundedRect(
        const Rect& rect,
        Color fillColor,
        float radius,
        Color borderColor = Color::Transparent(),
        float borderWidth = 0.0f
    ) override;

    void drawText(
        std::string_view text,
        const Point& position,
        const TextStyle& style
    ) override;

    Dimensions measureText(
        std::string_view text,
        const TextStyle& style
    ) override;

    void drawImage(
        TextureId texture,
        const Rect& rect,
        Color tint = Color::White()
    ) override;

    void drawShadow(
        const Rect& rect,
        float radius,
        Color shadowColor,
        float blurRadius,
        const Point& offset
    ) override;

    void drawLine(const Point& a, const Point& b, Color color, float thickness = 1.0f) override;
    void drawPolyline(std::span<const Point> pts, Color color, float thickness = 1.0f, bool closed = false) override;
    void drawCircle(const Point& center, float radius, Color fill, Color border = Color::Transparent(), float borderWidth = 0.0f) override;
    void drawArc(const Point& center, float radius, float startAngle, float endAngle, Color color, float thickness = 1.0f) override;
    void drawTriangle(const Point& a, const Point& b, const Point& c, Color fill) override;
    void drawQuad(const Point& a, const Point& b, const Point& c, const Point& d, Color fill) override;
    void drawPolygon(std::span<const Point> pts, Color fill) override;
    void drawRectOutline(const Rect& rect, Color color, float thickness = 1.0f, float rounding = 0.0f) override;
    void drawImageRotated(TextureId tex, const Point& center, const Dimensions& size, float angleRadians, Color tint = Color::White()) override;
    void drawImageUV(TextureId tex, const Rect& rect, const Point& uv0, const Point& uv1, Color tint = Color::White()) override;
    void drawImageQuad(TextureId tex, const Point& p1, const Point& p2, const Point& p3, const Point& p4,
                       const Point& uv1 = { 0.0f, 0.0f }, const Point& uv2 = { 1.0f, 0.0f },
                       const Point& uv3 = { 1.0f, 1.0f }, const Point& uv4 = { 0.0f, 1.0f },
                       Color tint = Color::White()) override;
    void drawGradientRect(const Rect& rect, Color topLeft, Color topRight, Color bottomRight, Color bottomLeft) override;

    // Texture Management API (Stage 2 Clean Client)
    void setD3D11Device(void* device, void* context);
    TextureId loadTexture(std::string_view filePath) override;
    TextureId createDynamicTexture(uint32_t width, uint32_t height, const uint8_t* rgbaPixels) override;
    bool updateDynamicTexture(TextureId id, uint32_t width, uint32_t height, const uint8_t* rgbaPixels) override;
    void destroyTexture(TextureId id) override;
    Dimensions getTextureSize(TextureId id) override;

    void initFonts();

private:
    struct TextureRecord {
        void* srv{ nullptr };
        void* texture{ nullptr };
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        bool dynamic{ false };
    };

    uint64_t resolveTexture(TextureId id);

    void* m_d3dDevice{ nullptr };
    void* m_d3dContext{ nullptr };
    TextureId m_nextTextureId{ 1 };
    std::unordered_map<TextureId, TextureRecord> m_textures;
    mutable std::mutex m_textureMutex;

    ImDrawList* getDrawList();
    ImFont* getFontForStyle(const TextStyle& style) const;

    ImDrawList* m_customDrawList{ nullptr };
    bool m_fontsInitialized{ false };
    ImFont* m_fontSmall{ nullptr };
    ImFont* m_fontRegular{ nullptr };
    ImFont* m_fontMedium{ nullptr };
    ImFont* m_fontBold{ nullptr };
    ImFont* m_fontTitle{ nullptr };
    ImFont* m_fontHeader{ nullptr };
};

} // namespace PerfUI
