#pragma once

#include "PerfUI/UIRenderBackend.h"

// Forward declare ImDrawList so imgui.h is NOT exposed in this header
struct ImDrawList;

namespace PerfUI {

class ImGuiRenderBackend : public UIRenderBackend {
public:
    ImGuiRenderBackend();
    ~ImGuiRenderBackend() override = default;

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

private:
    ImDrawList* getDrawList();

    ImDrawList* m_customDrawList{ nullptr };
};

} // namespace PerfUI
