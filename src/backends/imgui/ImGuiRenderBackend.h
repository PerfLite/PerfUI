#pragma once

#include "PerfUI/UIRenderBackend.h"

// Forward declare ImDrawList and ImFont so imgui.h is NOT exposed in this header
struct ImDrawList;
struct ImFont;

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

    void initFonts();

private:
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
