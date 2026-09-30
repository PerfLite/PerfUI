#include "ImGuiRenderBackend.h"
#include <imgui.h>
#include <algorithm>

namespace PerfUI {

static inline ImU32 ToImColor(Color c) {
    return IM_COL32(c.r, c.g, c.b, c.a);
}

static inline ImVec2 ToImVec2(Point p) {
    return ImVec2(p.x, p.y);
}

ImGuiRenderBackend::ImGuiRenderBackend() = default;

void ImGuiRenderBackend::setCustomDrawList(ImDrawList* drawList) {
    m_customDrawList = drawList;
}

ImDrawList* ImGuiRenderBackend::getDrawList() {
    if (m_customDrawList) {
        return m_customDrawList;
    }
    return ImGui::GetForegroundDrawList();
}

void ImGuiRenderBackend::beginFrame() {
    // Frame setup if needed
}

void ImGuiRenderBackend::endFrame() {
    // Frame teardown if needed
}

void ImGuiRenderBackend::pushClipRect(const Rect& rect) {
    ImDrawList* dl = getDrawList();
    if (!dl) return;

    dl->PushClipRect(
        ImVec2(rect.x, rect.y),
        ImVec2(rect.x + rect.width, rect.y + rect.height),
        true // intersect_with_current_clip_rect
    );
}

void ImGuiRenderBackend::popClipRect() {
    ImDrawList* dl = getDrawList();
    if (dl) {
        dl->PopClipRect();
    }
}

void ImGuiRenderBackend::drawRect(const Rect& rect, Color fillColor) {
    ImDrawList* dl = getDrawList();
    if (!dl || fillColor.a == 0) return;

    dl->AddRectFilled(
        ImVec2(rect.x, rect.y),
        ImVec2(rect.x + rect.width, rect.y + rect.height),
        ToImColor(fillColor)
    );
}

void ImGuiRenderBackend::drawRoundedRect(
    const Rect& rect,
    Color fillColor,
    float radius,
    Color borderColor,
    float borderWidth
) {
    ImDrawList* dl = getDrawList();
    if (!dl) return;

    ImVec2 pMin(rect.x, rect.y);
    ImVec2 pMax(rect.x + rect.width, rect.y + rect.height);

    if (fillColor.a > 0) {
        dl->AddRectFilled(pMin, pMax, ToImColor(fillColor), radius);
    }

    if (borderColor.a > 0 && borderWidth > 0.0f) {
        dl->AddRect(pMin, pMax, ToImColor(borderColor), radius, 0, borderWidth);
    }
}

void ImGuiRenderBackend::drawText(
    std::string_view text,
    const Point& position,
    const TextStyle& style
) {
    ImDrawList* dl = getDrawList();
    if (!dl || text.empty() || style.color.a == 0) return;

    // Dear ImGui accepts const char* begin and end pointers
    const char* textBegin = text.data();
    const char* textEnd = text.data() + text.size();

    dl->AddText(
        nullptr, // Default font or font sized
        style.fontSize,
        ToImVec2(position),
        ToImColor(style.color),
        textBegin,
        textEnd
    );
}

Dimensions ImGuiRenderBackend::measureText(
    std::string_view text,
    const TextStyle& style
) {
    (void)style;
    if (text.empty()) {
        return Dimensions{ 0.0f, 0.0f };
    }

    const char* textBegin = text.data();
    const char* textEnd = text.data() + text.size();

    ImVec2 size = ImGui::CalcTextSize(textBegin, textEnd);
    return Dimensions{ size.x, size.y };
}

void ImGuiRenderBackend::drawImage(
    TextureId texture,
    const Rect& rect,
    Color tint
) {
    ImDrawList* dl = getDrawList();
    if (!dl || texture == 0) return;

    ImTextureID tex = static_cast<ImTextureID>(texture);
    dl->AddImage(
        tex,
        ImVec2(rect.x, rect.y),
        ImVec2(rect.x + rect.width, rect.y + rect.height),
        ImVec2(0.0f, 0.0f),
        ImVec2(1.0f, 1.0f),
        ToImColor(tint)
    );
}

void ImGuiRenderBackend::drawShadow(
    const Rect& rect,
    float radius,
    Color shadowColor,
    float blurRadius,
    const Point& offset
) {
    ImDrawList* dl = getDrawList();
    if (!dl || shadowColor.a == 0) return;

    // Multi-pass approximation for soft drop shadow
    int passes = std::clamp(static_cast<int>(blurRadius / 2.0f), 1, 6);
    float alphaStep = static_cast<float>(shadowColor.a) / (passes * 2.0f);

    for (int i = 0; i < passes; ++i) {
        float expand = static_cast<float>(i + 1) * (blurRadius / passes);
        uint8_t a = static_cast<uint8_t>(std::max(1.0f, alphaStep * (passes - i)));
        Color passColor(shadowColor.r, shadowColor.g, shadowColor.b, a);

        ImVec2 pMin(rect.x + offset.x - expand, rect.y + offset.y - expand);
        ImVec2 pMax(rect.x + rect.width + offset.x + expand, rect.y + rect.height + offset.y + expand);

        dl->AddRectFilled(pMin, pMax, ToImColor(passColor), radius + expand);
    }
}

} // namespace PerfUI
