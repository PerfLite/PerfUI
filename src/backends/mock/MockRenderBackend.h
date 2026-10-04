#pragma once

#include "PerfUI/UIRenderBackend.h"
#include <vector>
#include <string>
#include <cstdint>

namespace PerfUI {

/**
 * @brief Headless, GPU-independent mock rendering backend for unit testing and CI.
 * 
 * Records all render commands and layout measurements without any DirectX,
 * OpenGL, Vulkan, or ImGui dependencies.
 */
class MockRenderBackend : public UIRenderBackend {
public:
    struct DrawCommand {
        enum class Type {
            Rect,
            RoundedRect,
            Text,
            Shadow,
            Image,
            PushClip,
            PopClip,
            Line,
            Polyline,
            Circle,
            Arc,
            Triangle,
            Quad,
            Polygon,
            RectOutline,
            ImageRotated,
            ImageUV,
            ImageQuad,
            GradientRect
        };

        Type type;
        Rect bounds;
        Color color;
        std::string text;
        float radius{ 0.0f };
        float thickness{ 1.0f };
        float angle{ 0.0f };
        std::vector<Point> points;
    };

    MockRenderBackend() = default;
    ~MockRenderBackend() override = default;

    void beginFrame() override {
        m_frameActive = true;
        m_commands.clear();
        m_drawCallCount = 0;
        m_textMeasureCount = 0;
    }

    void endFrame() override {
        m_frameActive = false;
    }

    void pushClipRect(const Rect& rect) override {
        m_clipStack.push_back(rect);
        m_commands.push_back({ DrawCommand::Type::PushClip, rect, Color(), "", 0.0f });
    }

    void popClipRect() override {
        if (!m_clipStack.empty()) {
            m_clipStack.pop_back();
        }
        m_commands.push_back({ DrawCommand::Type::PopClip, Rect(), Color(), "", 0.0f });
    }

    void drawRect(const Rect& rect, Color fillColor) override {
        ++m_drawCallCount;
        m_commands.push_back({ DrawCommand::Type::Rect, rect, fillColor, "", 0.0f });
    }

    void drawRoundedRect(const Rect& rect, Color fillColor, float cornerRadius, Color borderColor = Color::Transparent(), float borderWidth = 0.0f) override {
        (void)borderColor;
        (void)borderWidth;
        ++m_drawCallCount;
        m_commands.push_back({ DrawCommand::Type::RoundedRect, rect, fillColor, "", cornerRadius });
    }

    void drawText(std::string_view text, const Point& position, const TextStyle& style) override {
        ++m_drawCallCount;
        Rect textRect{ position.x, position.y, static_cast<float>(text.length()) * (style.fontSize * 0.58f), style.fontSize * 1.25f };
        m_commands.push_back({ DrawCommand::Type::Text, textRect, style.color, std::string(text), 0.0f });
    }

    Dimensions measureText(std::string_view text, const TextStyle& style) override {
        ++m_textMeasureCount;
        float w = static_cast<float>(text.length()) * (style.fontSize * 0.58f);
        float h = style.fontSize * 1.25f;
        return Dimensions{ w, h };
    }

    void drawShadow(const Rect& rect, float cornerRadius, Color shadowColor, float blurRadius, const Point& offset) override {
        (void)blurRadius;
        (void)offset;
        ++m_drawCallCount;
        m_commands.push_back({ DrawCommand::Type::Shadow, rect, shadowColor, "", cornerRadius });
    }

    void drawImage(TextureId texture, const Rect& rect, Color tint = Color::White()) override {
        (void)texture;
        ++m_drawCallCount;
        m_commands.push_back({ DrawCommand::Type::Image, rect, tint, "", 0.0f });
    }

    void drawLine(const Point& a, const Point& b, Color color, float thickness = 1.0f) override {
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::Line, Rect{ a.x, a.y, b.x - a.x, b.y - a.y }, color, "", 0.0f };
        cmd.thickness = thickness;
        m_commands.push_back(cmd);
    }

    void drawPolyline(std::span<const Point> pts, Color color, float thickness = 1.0f, bool closed = false) override {
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::Polyline, Rect(), color, closed ? "closed" : "open", 0.0f };
        cmd.thickness = thickness;
        cmd.points.assign(pts.begin(), pts.end());
        m_commands.push_back(cmd);
    }

    void drawCircle(const Point& center, float radius, Color fill, Color border = Color::Transparent(), float borderWidth = 0.0f) override {
        (void)border;
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::Circle, Rect{ center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f }, fill, "", radius };
        cmd.thickness = borderWidth;
        m_commands.push_back(cmd);
    }

    void drawArc(const Point& center, float radius, float startAngle, float endAngle, Color color, float thickness = 1.0f) override {
        (void)startAngle; (void)endAngle;
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::Arc, Rect{ center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f }, color, "", radius };
        cmd.thickness = thickness;
        m_commands.push_back(cmd);
    }

    void drawTriangle(const Point& a, const Point& b, const Point& c, Color fill) override {
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::Triangle, Rect(), fill, "", 0.0f };
        cmd.points = { a, b, c };
        m_commands.push_back(cmd);
    }

    void drawQuad(const Point& a, const Point& b, const Point& c, const Point& d, Color fill) override {
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::Quad, Rect(), fill, "", 0.0f };
        cmd.points = { a, b, c, d };
        m_commands.push_back(cmd);
    }

    void drawPolygon(std::span<const Point> pts, Color fill) override {
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::Polygon, Rect(), fill, "", 0.0f };
        cmd.points.assign(pts.begin(), pts.end());
        m_commands.push_back(cmd);
    }

    void drawRectOutline(const Rect& rect, Color color, float thickness = 1.0f, float rounding = 0.0f) override {
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::RectOutline, rect, color, "", rounding };
        cmd.thickness = thickness;
        m_commands.push_back(cmd);
    }

    void drawImageRotated(TextureId tex, const Point& center, const Dimensions& size, float angleRadians, Color tint = Color::White()) override {
        (void)tex;
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::ImageRotated, Rect{ center.x - size.width * 0.5f, center.y - size.height * 0.5f, size.width, size.height }, tint, "", 0.0f };
        cmd.angle = angleRadians;
        m_commands.push_back(cmd);
    }

    void drawImageUV(TextureId tex, const Rect& rect, const Point& uv0, const Point& uv1, Color tint = Color::White()) override {
        (void)tex; (void)uv0; (void)uv1;
        ++m_drawCallCount;
        m_commands.push_back({ DrawCommand::Type::ImageUV, rect, tint, "", 0.0f });
    }

    void drawImageQuad(TextureId tex, const Point& p1, const Point& p2, const Point& p3, const Point& p4,
                       const Point& uv1 = { 0, 0 }, const Point& uv2 = { 1, 0 },
                       const Point& uv3 = { 1, 1 }, const Point& uv4 = { 0, 1 },
                       Color tint = Color::White()) override {
        (void)tex; (void)uv1; (void)uv2; (void)uv3; (void)uv4;
        ++m_drawCallCount;
        DrawCommand cmd{ DrawCommand::Type::ImageQuad, Rect(), tint, "", 0.0f };
        cmd.points = { p1, p2, p3, p4 };
        m_commands.push_back(cmd);
    }

    void drawGradientRect(const Rect& rect, Color topLeft, Color topRight, Color bottomRight, Color bottomLeft) override {
        (void)topRight; (void)bottomRight; (void)bottomLeft;
        ++m_drawCallCount;
        m_commands.push_back({ DrawCommand::Type::GradientRect, rect, topLeft, "", 0.0f });
    }

    struct MockTexture {
        std::string filePath;
        uint32_t width{ 0 };
        uint32_t height{ 0 };
        std::vector<uint8_t> data;
    };

    TextureId loadTexture(std::string_view filePath) override {
        TextureId id = m_nextTextureId++;
        MockTexture tex;
        tex.filePath = std::string(filePath);
        tex.width = 64;
        tex.height = 64;
        m_mockTextures[id] = tex;
        return id;
    }

    TextureId createDynamicTexture(uint32_t width, uint32_t height, const uint8_t* rgbaPixels) override {
        TextureId id = m_nextTextureId++;
        MockTexture tex;
        tex.width = width;
        tex.height = height;
        if (rgbaPixels) {
            tex.data.assign(rgbaPixels, rgbaPixels + (width * height * 4));
        }
        m_mockTextures[id] = tex;
        return id;
    }

    bool updateDynamicTexture(TextureId id, uint32_t width, uint32_t height, const uint8_t* rgbaPixels) override {
        auto it = m_mockTextures.find(id);
        if (it == m_mockTextures.end()) return false;
        it->second.width = width;
        it->second.height = height;
        if (rgbaPixels) {
            it->second.data.assign(rgbaPixels, rgbaPixels + (width * height * 4));
        }
        return true;
    }

    void destroyTexture(TextureId id) override {
        m_mockTextures.erase(id);
    }

    Dimensions getTextureSize(TextureId id) override {
        auto it = m_mockTextures.find(id);
        if (it != m_mockTextures.end()) {
            return Dimensions{ static_cast<float>(it->second.width), static_cast<float>(it->second.height) };
        }
        return Dimensions{ 0.0f, 0.0f };
    }

    // Telemetry & Test Assertions
    bool isFrameActive() const { return m_frameActive; }
    size_t drawCallCount() const { return m_drawCallCount; }
    size_t textMeasureCount() const { return m_textMeasureCount; }
    size_t clipStackDepth() const { return m_clipStack.size(); }
    const std::vector<DrawCommand>& commands() const { return m_commands; }
    const std::unordered_map<TextureId, MockTexture>& mockTextures() const { return m_mockTextures; }
    size_t loadedTextureCount() const { return m_mockTextures.size(); }

private:
    bool m_frameActive{ false };
    size_t m_drawCallCount{ 0 };
    size_t m_textMeasureCount{ 0 };
    TextureId m_nextTextureId{ 1001 };
    std::vector<Rect> m_clipStack;
    std::vector<DrawCommand> m_commands;
    std::unordered_map<TextureId, MockTexture> m_mockTextures;
};

} // namespace PerfUI
