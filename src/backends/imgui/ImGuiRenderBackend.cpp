#include "ImGuiRenderBackend.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#endif
#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <cmath>

#pragma warning(push)
#pragma warning(disable: 4244 4996)
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb/stb_image.h"
#pragma warning(pop)

namespace PerfUI {

static inline ImU32 ToImColor(Color c) {
    return IM_COL32(c.r, c.g, c.b, c.a);
}

static inline ImVec2 ToImVec2(Point p) {
    return ImVec2(p.x, p.y);
}

ImGuiRenderBackend::ImGuiRenderBackend() = default;

ImGuiRenderBackend::~ImGuiRenderBackend() {
#if defined(_WIN32)
    std::lock_guard<std::mutex> lock(m_textureMutex);
    for (auto& [id, record] : m_textures) {
        if (record.srv) static_cast<ID3D11ShaderResourceView*>(record.srv)->Release();
        if (record.texture) static_cast<ID3D11Texture2D*>(record.texture)->Release();
    }
    m_textures.clear();
#endif
}

uint64_t ImGuiRenderBackend::resolveTexture(TextureId id) {
    if (id == 0) return 0;
    std::lock_guard<std::mutex> lock(m_textureMutex);
    auto it = m_textures.find(id);
    if (it != m_textures.end() && it->second.srv) {
        return reinterpret_cast<uintptr_t>(it->second.srv);
    }
    return id;
}

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

void ImGuiRenderBackend::initFonts() {
    if (m_fontsInitialized) return;

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    const char* regularPath = "C:\\Windows\\Fonts\\segoeui.ttf";
    const char* boldPath = "C:\\Windows\\Fonts\\segoeuib.ttf";

    // Fallback to Arial if Segoe UI is not present
    if (!std::filesystem::exists(regularPath)) {
        regularPath = "C:\\Windows\\Fonts\\arial.ttf";
        boldPath = "C:\\Windows\\Fonts\\arialbd.ttf";
    }

    if (std::filesystem::exists(regularPath)) {
        ImFontConfig cfg;
        cfg.OversampleH = 3;
        cfg.OversampleV = 2;
        cfg.PixelSnapH = true;

        const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesCyrillic();

        m_fontSmall   = io.Fonts->AddFontFromFileTTF(regularPath, 13.0f, &cfg, glyphRanges);
        m_fontRegular = io.Fonts->AddFontFromFileTTF(regularPath, 16.0f, &cfg, glyphRanges);
        m_fontMedium  = io.Fonts->AddFontFromFileTTF(regularPath, 19.0f, &cfg, glyphRanges);
        m_fontBold    = io.Fonts->AddFontFromFileTTF(boldPath,    17.0f, &cfg, glyphRanges);
        m_fontTitle   = io.Fonts->AddFontFromFileTTF(boldPath,    22.0f, &cfg, glyphRanges);
        m_fontHeader  = io.Fonts->AddFontFromFileTTF(boldPath,    28.0f, &cfg, glyphRanges);
    }

    if (!m_fontRegular) {
        io.Fonts->AddFontDefault();
    }

    m_fontsInitialized = true;
}

ImFont* ImGuiRenderBackend::getFontForStyle(const TextStyle& style) const {
    if (!m_fontsInitialized || !m_fontRegular) {
        return nullptr;
    }

    if (style.bold) {
        if (style.fontSize >= 25.0f) return m_fontHeader ? m_fontHeader : m_fontTitle;
        if (style.fontSize >= 20.0f) return m_fontTitle ? m_fontTitle : m_fontBold;
        return m_fontBold ? m_fontBold : m_fontRegular;
    } else {
        if (style.fontSize >= 24.0f) return m_fontHeader ? m_fontHeader : m_fontMedium;
        if (style.fontSize >= 18.0f) return m_fontMedium ? m_fontMedium : m_fontRegular;
        if (style.fontSize <= 13.0f) return m_fontSmall ? m_fontSmall : m_fontRegular;
        return m_fontRegular;
    }
}

void ImGuiRenderBackend::drawText(
    std::string_view text,
    const Point& position,
    const TextStyle& style
) {
    ImDrawList* dl = getDrawList();
    if (!dl || text.empty() || style.color.a == 0) return;

    const char* textBegin = text.data();
    const char* textEnd = text.data() + text.size();
    ImFont* font = getFontForStyle(style);
    float fontSize = style.fontSize;
    float wrapWidth = style.wrapWidth > 0.0f ? style.wrapWidth : 0.0f;

    dl->AddText(
        font,
        fontSize,
        ToImVec2(position),
        ToImColor(style.color),
        textBegin,
        textEnd,
        wrapWidth
    );
}

Dimensions ImGuiRenderBackend::measureText(
    std::string_view text,
    const TextStyle& style
) {
    if (text.empty()) {
        return Dimensions{ 0.0f, 0.0f };
    }

    const char* textBegin = text.data();
    const char* textEnd = text.data() + text.size();
    ImFont* font = getFontForStyle(style);
    float fontSize = style.fontSize;
    float wrapWidth = style.wrapWidth > 0.0f ? style.wrapWidth : 0.0f;

    if (font) {
        ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, wrapWidth, textBegin, textEnd);
        return Dimensions{ size.x, size.y };
    }

    ImVec2 size = ImGui::CalcTextSize(textBegin, textEnd, false, wrapWidth > 0.0f ? wrapWidth : -1.0f);
    return Dimensions{ size.x, size.y };
}

void ImGuiRenderBackend::drawImage(
    TextureId texture,
    const Rect& rect,
    Color tint
) {
    ImDrawList* dl = getDrawList();
    if (!dl || texture == 0) return;

    ImTextureID tex = resolveTexture(texture);
    if (!tex) return;
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
    int passes = (std::clamp)(static_cast<int>(blurRadius / 2.0f), 1, 6);
    float alphaStep = static_cast<float>(shadowColor.a) / (passes * 2.0f);

    for (int i = 0; i < passes; ++i) {
        float expand = static_cast<float>(i + 1) * (blurRadius / passes);
        uint8_t a = static_cast<uint8_t>((std::max)(1.0f, alphaStep * (passes - i)));
        Color passColor(shadowColor.r, shadowColor.g, shadowColor.b, a);

        ImVec2 pMin(rect.x + offset.x - expand, rect.y + offset.y - expand);
        ImVec2 pMax(rect.x + rect.width + offset.x + expand, rect.y + rect.height + offset.y + expand);

        dl->AddRectFilled(pMin, pMax, ToImColor(passColor), radius + expand);
    }
}

void ImGuiRenderBackend::drawLine(const Point& a, const Point& b, Color color, float thickness) {
    ImDrawList* dl = getDrawList();
    if (!dl || color.a == 0) return;
    dl->AddLine(ToImVec2(a), ToImVec2(b), ToImColor(color), thickness);
}

void ImGuiRenderBackend::drawPolyline(std::span<const Point> pts, Color color, float thickness, bool closed) {
    ImDrawList* dl = getDrawList();
    if (!dl || color.a == 0 || pts.size() < 2) return;

    std::vector<ImVec2> imPts;
    imPts.reserve(pts.size());
    for (const auto& p : pts) {
        imPts.emplace_back(p.x, p.y);
    }
    ImDrawFlags flags = closed ? ImDrawFlags_Closed : ImDrawFlags_None;
    dl->AddPolyline(imPts.data(), static_cast<int>(imPts.size()), ToImColor(color), flags, thickness);
}

void ImGuiRenderBackend::drawCircle(const Point& center, float radius, Color fill, Color border, float borderWidth) {
    ImDrawList* dl = getDrawList();
    if (!dl || radius <= 0.0f) return;

    ImVec2 c = ToImVec2(center);
    if (fill.a > 0) {
        dl->AddCircleFilled(c, radius, ToImColor(fill));
    }
    if (border.a > 0 && borderWidth > 0.0f) {
        dl->AddCircle(c, radius, ToImColor(border), 0, borderWidth);
    }
}

void ImGuiRenderBackend::drawArc(const Point& center, float radius, float startAngle, float endAngle, Color color, float thickness) {
    ImDrawList* dl = getDrawList();
    if (!dl || color.a == 0 || radius <= 0.0f) return;

    dl->PathClear();
    dl->PathArcTo(ToImVec2(center), radius, startAngle, endAngle);
    dl->PathStroke(ToImColor(color), ImDrawFlags_None, thickness);
}

void ImGuiRenderBackend::drawTriangle(const Point& a, const Point& b, const Point& c, Color fill) {
    ImDrawList* dl = getDrawList();
    if (!dl || fill.a == 0) return;
    dl->AddTriangleFilled(ToImVec2(a), ToImVec2(b), ToImVec2(c), ToImColor(fill));
}

void ImGuiRenderBackend::drawQuad(const Point& a, const Point& b, const Point& c, const Point& d, Color fill) {
    ImDrawList* dl = getDrawList();
    if (!dl || fill.a == 0) return;
    dl->AddQuadFilled(ToImVec2(a), ToImVec2(b), ToImVec2(c), ToImVec2(d), ToImColor(fill));
}

void ImGuiRenderBackend::drawPolygon(std::span<const Point> pts, Color fill) {
    ImDrawList* dl = getDrawList();
    if (!dl || fill.a == 0 || pts.size() < 3) return;

    std::vector<ImVec2> imPts;
    imPts.reserve(pts.size());
    for (const auto& p : pts) {
        imPts.emplace_back(p.x, p.y);
    }
    dl->AddConvexPolyFilled(imPts.data(), static_cast<int>(imPts.size()), ToImColor(fill));
}

void ImGuiRenderBackend::drawRectOutline(const Rect& rect, Color color, float thickness, float rounding) {
    ImDrawList* dl = getDrawList();
    if (!dl || color.a == 0) return;
    dl->AddRect(
        ImVec2(rect.x, rect.y),
        ImVec2(rect.x + rect.width, rect.y + rect.height),
        ToImColor(color),
        rounding,
        ImDrawFlags_None,
        thickness
    );
}

void ImGuiRenderBackend::drawImageRotated(TextureId tex, const Point& center, const Dimensions& size, float angleRadians, Color tint) {
    ImDrawList* dl = getDrawList();
    if (!dl || tex == 0 || tint.a == 0) return;

    float hw = size.width * 0.5f;
    float hh = size.height * 0.5f;
    float cosA = std::cos(angleRadians);
    float sinA = std::sin(angleRadians);

    auto rotate = [&](float x, float y) -> ImVec2 {
        return ImVec2(
            center.x + (x * cosA - y * sinA),
            center.y + (x * sinA + y * cosA)
        );
    };

    ImVec2 p1 = rotate(-hw, -hh);
    ImVec2 p2 = rotate(hw, -hh);
    ImVec2 p3 = rotate(hw, hh);
    ImVec2 p4 = rotate(-hw, hh);

    ImTextureID texId = resolveTexture(tex);
    if (!texId) return;

    dl->AddImageQuad(
        texId,
        p1, p2, p3, p4,
        ImVec2(0.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec2(0.0f, 1.0f),
        ToImColor(tint)
    );
}

void ImGuiRenderBackend::drawImageUV(TextureId tex, const Rect& rect, const Point& uv0, const Point& uv1, Color tint) {
    ImDrawList* dl = getDrawList();
    if (!dl || tex == 0 || tint.a == 0) return;

    ImTextureID texId = resolveTexture(tex);
    if (!texId) return;

    dl->AddImage(
        texId,
        ImVec2(rect.x, rect.y),
        ImVec2(rect.x + rect.width, rect.y + rect.height),
        ImVec2(uv0.x, uv0.y),
        ImVec2(uv1.x, uv1.y),
        ToImColor(tint)
    );
}

void ImGuiRenderBackend::drawImageQuad(TextureId tex, const Point& p1, const Point& p2, const Point& p3, const Point& p4,
                                      const Point& uv1, const Point& uv2, const Point& uv3, const Point& uv4,
                                      Color tint) {
    ImDrawList* dl = getDrawList();
    if (!dl || tex == 0 || tint.a == 0) return;

    ImTextureID texId = resolveTexture(tex);
    if (!texId) return;

    dl->AddImageQuad(
        texId,
        ToImVec2(p1), ToImVec2(p2), ToImVec2(p3), ToImVec2(p4),
        ToImVec2(uv1), ToImVec2(uv2), ToImVec2(uv3), ToImVec2(uv4),
        ToImColor(tint)
    );
}

void ImGuiRenderBackend::drawGradientRect(const Rect& rect, Color topLeft, Color topRight, Color bottomRight, Color bottomLeft) {
    ImDrawList* dl = getDrawList();
    if (!dl) return;

    dl->AddRectFilledMultiColor(
        ImVec2(rect.x, rect.y),
        ImVec2(rect.x + rect.width, rect.y + rect.height),
        ToImColor(topLeft),
        ToImColor(topRight),
        ToImColor(bottomRight),
        ToImColor(bottomLeft)
    );
}

void ImGuiRenderBackend::setD3D11Device(void* device, void* context) {
    std::lock_guard<std::mutex> lock(m_textureMutex);
    m_d3dDevice = device;
    m_d3dContext = context;
}

TextureId ImGuiRenderBackend::loadTexture(std::string_view filePath) {
#if defined(_WIN32)
    ID3D11Device* device = static_cast<ID3D11Device*>(m_d3dDevice);
    if (!device) return 0;

    std::string pathStr(filePath);
    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* data = stbi_load(pathStr.c_str(), &width, &height, &channels, 4);
    if (!data) return 0;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subData{};
    subData.pSysMem = data;
    subData.SysMemPitch = static_cast<UINT>(width * 4);

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &subData, &texture);
    stbi_image_free(data);

    if (FAILED(hr) || !texture) return 0;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = device->CreateShaderResourceView(texture, nullptr, &srv);
    if (FAILED(hr) || !srv) {
        texture->Release();
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_textureMutex);
    TextureId id = m_nextTextureId++;
    m_textures[id] = TextureRecord{ srv, texture, static_cast<uint32_t>(width), static_cast<uint32_t>(height), false };
    return id;
#else
    (void)filePath;
    return 0;
#endif
}

TextureId ImGuiRenderBackend::createDynamicTexture(uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
#if defined(_WIN32)
    ID3D11Device* device = static_cast<ID3D11Device*>(m_d3dDevice);
    if (!device || width == 0 || height == 0) return 0;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subData{};
    subData.pSysMem = rgbaPixels;
    subData.SysMemPitch = width * 4;

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, rgbaPixels ? &subData : nullptr, &texture);
    if (FAILED(hr) || !texture) return 0;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = device->CreateShaderResourceView(texture, nullptr, &srv);
    if (FAILED(hr) || !srv) {
        texture->Release();
        return 0;
    }

    std::lock_guard<std::mutex> lock(m_textureMutex);
    TextureId id = m_nextTextureId++;
    m_textures[id] = TextureRecord{ srv, texture, width, height, true };
    return id;
#else
    (void)width; (void)height; (void)rgbaPixels;
    return 0;
#endif
}

bool ImGuiRenderBackend::updateDynamicTexture(TextureId id, uint32_t width, uint32_t height, const uint8_t* rgbaPixels) {
#if defined(_WIN32)
    if (!m_d3dContext || !rgbaPixels) return false;
    ID3D11DeviceContext* ctx = static_cast<ID3D11DeviceContext*>(m_d3dContext);

    std::lock_guard<std::mutex> lock(m_textureMutex);
    auto it = m_textures.find(id);
    if (it == m_textures.end() || !it->second.texture) return false;

    ID3D11Texture2D* tex = static_cast<ID3D11Texture2D*>(it->second.texture);
    ctx->UpdateSubresource(tex, 0, nullptr, rgbaPixels, width * 4, 0);
    it->second.width = width;
    it->second.height = height;
    return true;
#else
    (void)id; (void)width; (void)height; (void)rgbaPixels;
    return false;
#endif
}

void ImGuiRenderBackend::destroyTexture(TextureId id) {
#if defined(_WIN32)
    std::lock_guard<std::mutex> lock(m_textureMutex);
    auto it = m_textures.find(id);
    if (it != m_textures.end()) {
        if (it->second.srv) {
            static_cast<ID3D11ShaderResourceView*>(it->second.srv)->Release();
        }
        if (it->second.texture) {
            static_cast<ID3D11Texture2D*>(it->second.texture)->Release();
        }
        m_textures.erase(it);
    }
#else
    (void)id;
#endif
}

Dimensions ImGuiRenderBackend::getTextureSize(TextureId id) {
    std::lock_guard<std::mutex> lock(m_textureMutex);
    auto it = m_textures.find(id);
    if (it != m_textures.end()) {
        return Dimensions{ static_cast<float>(it->second.width), static_cast<float>(it->second.height) };
    }
    return Dimensions{ 0.0f, 0.0f };
}

} // namespace PerfUI
