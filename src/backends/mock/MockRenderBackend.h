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
            PopClip
        };

        Type type;
        Rect bounds;
        Color color;
        std::string text;
        float radius{ 0.0f };
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

    // Telemetry & Test Assertions
    bool isFrameActive() const { return m_frameActive; }
    size_t drawCallCount() const { return m_drawCallCount; }
    size_t textMeasureCount() const { return m_textMeasureCount; }
    size_t clipStackDepth() const { return m_clipStack.size(); }
    const std::vector<DrawCommand>& commands() const { return m_commands; }

private:
    bool m_frameActive{ false };
    size_t m_drawCallCount{ 0 };
    size_t m_textMeasureCount{ 0 };
    std::vector<Rect> m_clipStack;
    std::vector<DrawCommand> m_commands;
};

} // namespace PerfUI
