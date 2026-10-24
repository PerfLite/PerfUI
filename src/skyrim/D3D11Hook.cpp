#include "D3D11Hook.h"
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include "InputHook.h"
#include "PerfUI/JournalWindow.h"
#include "SkyrimQuestService.h"
#include "SkyrimSoundService.h"
#include "WorldTrackingService.h"

namespace PerfUI::Skyrim {

bool D3D11Hook::IsCursorNeeded() const {
    if (InputHook::GetSingleton().IsCaptureInput()) return true;
    if (HasVisibleRootWindows()) return true;

    auto* mc = RE::MenuCursor::GetSingleton();
    if (mc && mc->showCursorCount > 0) return true;

    auto* ui = RE::UI::GetSingleton();
    if (!ui) return false;

    for (const auto& menu : ui->menuStack) {
        if (menu && menu->UsesCursor()) {
            return true;
        }
    }

    if (ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME)) {
        return true;
    }

    if (ui->IsMenuOpen(RE::Console::MENU_NAME)) {
        return true;
    }

    return false;
}

namespace {

void DrawSkyrimSwordCursor(ImDrawList* fg, ImVec2 pos) {
    if (!fg) return;

    constexpr float cosA = 0.70710678f;
    constexpr float sinA = 0.70710678f;

    auto pt = [&](float t, float s, float ox = 0.0f, float oy = 0.0f) -> ImVec2 {
        return ImVec2(
            pos.x + ox + t * cosA - s * sinA,
            pos.y + oy + t * sinA + s * cosA
        );
    };

    // 1. Double-layer smooth drop shadow (decomposed into convex polygons to avoid concave triangle-fan artifacts)
    auto drawShadowParts = [&](float ox, float oy, ImU32 col) {
        // Blade shadow (split into upper & lower convex halves)
        ImVec2 s_tip = pt(0.0f, 0.0f, ox, oy);
        ImVec2 s_b1L = pt(2.5f, -1.0f, ox, oy);
        ImVec2 s_b1R = pt(2.5f, 1.0f, ox, oy);
        ImVec2 s_b1C = pt(2.5f, 0.0f, ox, oy);
        ImVec2 s_b2L = pt(9.0f, -1.9f, ox, oy);
        ImVec2 s_b2R = pt(9.0f, 1.9f, ox, oy);
        ImVec2 s_b2C = pt(9.0f, 0.0f, ox, oy);
        ImVec2 s_b3L = pt(17.5f, -2.4f, ox, oy);
        ImVec2 s_b3R = pt(17.5f, 2.4f, ox, oy);
        ImVec2 s_b3C = pt(17.5f, 0.0f, ox, oy);

        fg->AddTriangleFilled(s_tip, s_b1L, s_b1C, col);
        fg->AddQuadFilled(s_b1L, s_b2L, s_b2C, s_b1C, col);
        fg->AddQuadFilled(s_b2L, s_b3L, s_b3C, s_b2C, col);

        fg->AddTriangleFilled(s_tip, s_b1C, s_b1R, col);
        fg->AddQuadFilled(s_b1C, s_b2C, s_b2R, s_b1R, col);
        fg->AddQuadFilled(s_b2C, s_b3C, s_b3R, s_b2R, col);

        // Guard shadow
        ImVec2 s_gMid  = pt(17.6f, 0.0f, ox, oy);
        ImVec2 s_qUK   = pt(17.2f, -5.2f, ox, oy);
        ImVec2 s_qUTip = pt(15.8f, -9.0f, ox, oy);
        ImVec2 s_qUFlr = pt(18.8f, -9.5f, ox, oy);
        ImVec2 s_qURr  = pt(20.0f, -7.8f, ox, oy);
        ImVec2 s_qUBs  = pt(20.0f, -1.6f, ox, oy);

        ImVec2 s_qLK   = pt(17.2f, 5.2f, ox, oy);
        ImVec2 s_qLTip = pt(15.8f, 9.0f, ox, oy);
        ImVec2 s_qLFlr = pt(18.8f, 9.5f, ox, oy);
        ImVec2 s_qLRr  = pt(20.0f, 7.8f, ox, oy);
        ImVec2 s_qLBs  = pt(20.0f, 1.6f, ox, oy);

        fg->AddTriangleFilled(s_gMid, s_qUK, s_qUBs, col);
        fg->AddQuadFilled(s_qUK, s_qUTip, s_qUFlr, s_qURr, col);
        fg->AddTriangleFilled(s_qUK, s_qURr, s_qUBs, col);

        fg->AddTriangleFilled(s_gMid, s_qLBs, s_qLK, col);
        fg->AddQuadFilled(s_qLK, s_qLTip, s_qLFlr, s_qLRr, col);
        fg->AddTriangleFilled(s_qLK, s_qLRr, s_qLBs, col);

        fg->AddQuadFilled(s_b3L, s_b3R, s_qLBs, s_qUBs, col);

        // Grip shadow
        ImVec2 s_gTL = pt(20.2f, -1.6f, ox, oy);
        ImVec2 s_gTR = pt(20.2f, 1.6f, ox, oy);
        ImVec2 s_gML = pt(23.5f, -1.4f, ox, oy);
        ImVec2 s_gMR = pt(23.5f, 1.4f, ox, oy);
        ImVec2 s_gBL = pt(26.8f, -1.6f, ox, oy);
        ImVec2 s_gBR = pt(26.8f, 1.6f, ox, oy);
        fg->AddQuadFilled(s_gTL, s_gTR, s_gMR, s_gML, col);
        fg->AddQuadFilled(s_gML, s_gMR, s_gBR, s_gBL, col);

        // Pommel shadow
        ImVec2 s_pF = pt(26.8f, 0.0f, ox, oy);
        ImVec2 s_pT = pt(27.3f, -3.0f, ox, oy);
        ImVec2 s_pE = pt(30.6f, 0.0f, ox, oy);
        ImVec2 s_pB = pt(27.3f, 3.0f, ox, oy);
        fg->AddTriangleFilled(s_pF, s_pT, s_pE, col);
        fg->AddTriangleFilled(s_pF, s_pE, s_pB, col);

        // Feathered silhouette outline for soft contact shadow
        ImVec2 s_poly[] = {
            s_tip, s_b1L, s_b2L, s_b3L,
            s_qUK, s_qUTip, s_qUFlr, s_qURr, s_qUBs,
            s_gTL, s_gML, s_gBL,
            s_pT, s_pE, s_pB,
            s_gBR, s_gMR, s_gTR,
            s_qLBs, s_qLRr, s_qLFlr, s_qLTip, s_qLK,
            s_b3R, s_b2R, s_b1R
        };
        constexpr int nShadowPts = sizeof(s_poly) / sizeof(s_poly[0]);
        fg->AddPolyline(s_poly, nShadowPts, col, ImDrawFlags_Closed, 1.5f);
    };

    // Layer 1: Ambient soft blur shadow
    drawShadowParts(2.0f, 2.5f, IM_COL32(0, 0, 0, 50));
    // Layer 2: Focused contact shadow
    drawShadowParts(1.0f, 1.2f, IM_COL32(0, 0, 0, 110));

    // 2. Ergonomic Dark Leather Grip
    ImVec2 gripTopL = pt(20.2f, -1.6f);
    ImVec2 gripTopR = pt(20.2f, 1.6f);
    ImVec2 gripMidL = pt(23.5f, -1.4f);
    ImVec2 gripMidR = pt(23.5f, 1.4f);
    ImVec2 gripBotL = pt(26.8f, -1.6f);
    ImVec2 gripBotR = pt(26.8f, 1.6f);

    fg->AddQuadFilled(gripTopL, gripTopR, gripMidR, gripMidL, IM_COL32(36, 30, 26, 255));
    fg->AddQuadFilled(gripMidL, gripMidR, gripBotR, gripBotL, IM_COL32(30, 24, 20, 255));
    // Leather sheen along upper crest
    fg->AddLine(gripTopL, gripBotL, IM_COL32(82, 70, 60, 255), 1.0f);

    // Twisted Nordic brass/gold wire wraps with gleam
    fg->AddLine(pt(21.4f, -1.6f), pt(22.8f, 1.5f), IM_COL32(230, 185, 80, 240), 1.0f);
    fg->AddLine(pt(23.2f, -1.4f), pt(24.6f, 1.4f), IM_COL32(230, 185, 80, 240), 1.0f);
    fg->AddLine(pt(25.0f, -1.5f), pt(26.4f, 1.6f), IM_COL32(230, 185, 80, 240), 1.0f);

    // 3. Pommel (Faceted Nordic steel diamond counterweight with Skyrim gold rivet)
    ImVec2 pommelFront = pt(26.8f, 0.0f);
    ImVec2 pommelTop   = pt(27.3f, -3.0f);
    ImVec2 pommelEnd   = pt(30.6f, 0.0f);
    ImVec2 pommelBot   = pt(27.3f, 3.0f);

    // Upper light facet (Cold mountain steel)
    fg->AddTriangleFilled(pommelFront, pommelTop, pommelEnd, IM_COL32(235, 242, 252, 255));
    // Lower shadow facet
    fg->AddTriangleFilled(pommelFront, pommelEnd, pommelBot, IM_COL32(130, 140, 156, 255));
    // Pommel ridge highlight
    fg->AddLine(pommelFront, pommelEnd, IM_COL32(255, 255, 255, 230), 1.0f);
    // Center gold rivet
    fg->AddCircleFilled(pt(27.6f, 0.0f), 1.1f, IM_COL32(235, 185, 65, 255));
    // Tang peen block at the tip
    fg->AddCircleFilled(pt(30.4f, 0.0f), 0.85f, IM_COL32(200, 210, 225, 255));

    // 4. Crossguard (Nordic winged quillons with central gold langet & ruby)
    ImVec2 guardTopL = pt(17.5f, -2.4f);
    ImVec2 guardTopR = pt(17.5f, 2.4f);
    ImVec2 guardMidF = pt(17.6f, 0.0f);

    ImVec2 qUpperTip     = pt(15.8f, -9.0f);
    ImVec2 qUpperFlare   = pt(18.8f, -9.5f);
    ImVec2 qUpperRear    = pt(20.0f, -7.8f);
    ImVec2 qUpperBase    = pt(20.0f, -1.6f);
    ImVec2 qUpperKnuckle = pt(17.2f, -5.2f);

    ImVec2 qLowerTip     = pt(15.8f, 9.0f);
    ImVec2 qLowerFlare   = pt(18.8f, 9.5f);
    ImVec2 qLowerRear    = pt(20.0f, 7.8f);
    ImVec2 qLowerBase    = pt(20.0f, 1.6f);
    ImVec2 qLowerKnuckle = pt(17.2f, 5.2f);

    // Upper quillon (Light tempered steel)
    fg->AddTriangleFilled(guardMidF, qUpperKnuckle, qUpperBase, IM_COL32(212, 222, 238, 255));
    fg->AddQuadFilled(qUpperKnuckle, qUpperTip, qUpperFlare, qUpperRear, IM_COL32(230, 240, 252, 255));
    fg->AddTriangleFilled(qUpperKnuckle, qUpperRear, qUpperBase, IM_COL32(188, 198, 216, 255));
    // Upper quillon leading edge razor highlight
    fg->AddLine(guardTopL, qUpperTip, IM_COL32(255, 255, 255, 255), 1.0f);

    // Lower quillon (Shadowed steel)
    fg->AddTriangleFilled(guardMidF, qLowerBase, qLowerKnuckle, IM_COL32(136, 146, 162, 255));
    fg->AddQuadFilled(qLowerKnuckle, qLowerTip, qLowerFlare, qLowerRear, IM_COL32(146, 156, 172, 255));
    fg->AddTriangleFilled(qLowerKnuckle, qLowerRear, qLowerBase, IM_COL32(116, 126, 142, 255));

    // Guard center body
    fg->AddQuadFilled(guardTopL, guardTopR, qLowerBase, qUpperBase, IM_COL32(180, 190, 206, 255));

    // Central Skyrim gold diamond langet
    ImVec2 langetTop   = pt(17.2f, 0.0f);
    ImVec2 langetRight = pt(18.8f, 1.5f);
    ImVec2 langetBot   = pt(20.4f, 0.0f);
    ImVec2 langetLeft  = pt(18.8f, -1.5f);
    fg->AddQuadFilled(langetTop, langetRight, langetBot, langetLeft, IM_COL32(228, 175, 62, 255));
    fg->AddQuadFilled(pt(17.7f, 0.0f), pt(18.8f, 0.8f), pt(19.9f, 0.0f), pt(18.8f, -0.8f), IM_COL32(255, 232, 138, 255));
    // Center ruby jewel
    fg->AddCircleFilled(pt(18.8f, 0.0f), 0.75f, IM_COL32(190, 36, 36, 255));
    fg->AddCircleFilled(pt(18.6f, -0.2f), 0.35f, IM_COL32(255, 170, 170, 255));

    // 5. Blade (Tempered multi-faceted steel with fuller groove)
    ImVec2 tip = pt(0.0f, 0.0f);
    ImVec2 b1L = pt(2.5f, -1.0f);
    ImVec2 b1R = pt(2.5f, 1.0f);
    ImVec2 b1C = pt(2.5f, 0.0f);

    ImVec2 b2L = pt(9.0f, -1.9f);
    ImVec2 b2R = pt(9.0f, 1.9f);
    ImVec2 b2C = pt(9.0f, 0.0f);

    ImVec2 b3L = pt(17.5f, -2.4f);
    ImVec2 b3R = pt(17.5f, 2.4f);
    ImVec2 b3C = pt(17.5f, 0.0f);

    // Upper bevels (Polished cold steel catching overhead light)
    fg->AddTriangleFilled(tip, b1L, b1C, IM_COL32(255, 255, 255, 255));
    fg->AddQuadFilled(b1L, b2L, b2C, b1C, IM_COL32(240, 246, 255, 255));
    fg->AddQuadFilled(b2L, b3L, b3C, b2C, IM_COL32(220, 230, 244, 255));

    // Lower bevels (Gunmetal shadowed steel)
    fg->AddTriangleFilled(tip, b1C, b1R, IM_COL32(165, 175, 192, 255));
    fg->AddQuadFilled(b1C, b2C, b2R, b1R, IM_COL32(136, 146, 164, 255));
    fg->AddQuadFilled(b2C, b3C, b3R, b2R, IM_COL32(112, 122, 138, 255));

    // Central fuller (Blood groove - recessed deep steel channel)
    fg->AddLine(pt(2.5f, 0.0f), pt(16.5f, 0.0f), IM_COL32(58, 65, 78, 255), 1.0f);
    // Fuller specular highlight line (gleam along top ridge of groove)
    fg->AddLine(pt(1.8f, -0.4f), pt(16.8f, -0.4f), IM_COL32(255, 255, 255, 240), 0.8f);

    // Razor-sharp cutting edge highlight
    fg->AddLine(tip, b1L, IM_COL32(255, 255, 255, 255), 1.0f);
    fg->AddLine(b1L, b3L, IM_COL32(255, 255, 255, 240), 1.0f);

    // Tip specular star glint (gleam at needle point)
    fg->AddCircleFilled(pt(0.7f, -0.15f), 0.8f, IM_COL32(255, 255, 255, 255));
    fg->AddLine(pt(0.0f, -0.15f), pt(1.4f, -0.15f), IM_COL32(255, 255, 255, 180), 0.7f);
    fg->AddLine(pt(0.7f, -0.85f), pt(0.7f, 0.55f), IM_COL32(255, 255, 255, 180), 0.7f);

    // 6. Crisp Dark Outer Silhouette Outline (Gives razor definition against any background)
    ImVec2 outlinePoly[] = {
        tip,
        b1L,
        b2L,
        b3L,
        qUpperKnuckle,
        qUpperTip,
        qUpperFlare,
        qUpperRear,
        qUpperBase,
        gripTopL,
        gripMidL,
        gripBotL,
        pommelTop,
        pommelEnd,
        pommelBot,
        gripBotR,
        gripMidR,
        gripTopR,
        qLowerBase,
        qLowerRear,
        qLowerFlare,
        qLowerTip,
        qLowerKnuckle,
        b3R,
        b2R,
        b1R
    };
    constexpr int nOutline = sizeof(outlinePoly) / sizeof(outlinePoly[0]);
    fg->AddPolyline(outlinePoly, nOutline, IM_COL32(14, 16, 22, 255), ImDrawFlags_Closed, 1.15f);
}

} // namespace

// Demo test element for Skyrim verification
class SkyrimDemoCard : public PerfUI::UIElement {
public:
    SkyrimDemoCard() : PerfUI::UIElement("SkyrimDemoCard") {
        setFocusable(true);
    }

    void measure(PerfUI::Dimensions availableSize) override {
        m_desiredSize = { 460.0f, 280.0f };
        PerfUI::UIElement::measure(availableSize);
    }

    void arrange(const PerfUI::Rect& finalRect) override {
        float x = (finalRect.width - m_desiredSize.width) * 0.5f;
        float y = (finalRect.height - m_desiredSize.height) * 0.5f;
        PerfUI::UIElement::arrange(PerfUI::Rect{ x, y, m_desiredSize.width, m_desiredSize.height });
    }

    void render(PerfUI::UIRenderBackend& backend) override {
        if (!isVisible()) return;

        // Shadow
        backend.drawShadow(m_bounds, 12.0f, PerfUI::Color(0, 0, 0, 160), 20.0f, { 0.0f, 8.0f });

        // Card border & background
        PerfUI::Color border = m_hovered ? PerfUI::Color::NordicGold() : PerfUI::Color(65, 75, 90, 220);
        PerfUI::Color bg = m_hovered ? PerfUI::Color(22, 28, 38, 245) : PerfUI::Color::SlateDark();
        backend.drawRoundedRect(m_bounds, bg, 12.0f, border, 2.0f);

        // Header Title
        PerfUI::TextStyle titleStyle;
        titleStyle.color = PerfUI::Color::NordicGold();
        titleStyle.fontSize = 22.0f;
        titleStyle.bold = true;
        backend.drawText("PerfUI - Skyrim SE/AE Interface", { m_bounds.x + 24.0f, m_bounds.y + 24.0f }, titleStyle);

        // Body Info
        PerfUI::TextStyle bodyStyle;
        bodyStyle.color = PerfUI::Color(210, 215, 225, 255);
        bodyStyle.fontSize = 15.0f;
        backend.drawText("Engine: Skyrim SE 1.5.97 / SKSE64", { m_bounds.x + 24.0f, m_bounds.y + 68.0f }, bodyStyle);
        backend.drawText("Renderer: D3D11 Present Hook -> ImGuiBackend", { m_bounds.x + 24.0f, m_bounds.y + 94.0f }, bodyStyle);
        backend.drawText("Input: Keyboard & Mouse (F11 / ESC to close)", { m_bounds.x + 24.0f, m_bounds.y + 120.0f }, bodyStyle);

        // Interactive status badge
        PerfUI::Rect badgeRect{ m_bounds.x + 24.0f, m_bounds.y + 160.0f, 412.0f, 44.0f };
        PerfUI::Color badgeBg = m_pressed ? PerfUI::Color(45, 90, 45, 220) : (m_hovered ? PerfUI::Color(35, 50, 70, 220) : PerfUI::Color(18, 22, 30, 220));
        backend.drawRoundedRect(badgeRect, badgeBg, 8.0f, border, 1.0f);

        PerfUI::TextStyle badgeStyle;
        badgeStyle.color = m_pressed ? PerfUI::Color(140, 255, 140, 255) : (m_hovered ? PerfUI::Color::White() : PerfUI::Color(160, 170, 185, 255));
        badgeStyle.fontSize = 14.0f;
        const char* status = m_pressed ? "Mouse Click registered in Skyrim!" : (m_hovered ? "Hovering over card! Click to test" : "Move mouse here to test hover");
        backend.drawText(status, { badgeRect.x + 16.0f, badgeRect.y + 14.0f }, badgeStyle);

        // Footer status
        PerfUI::TextStyle footStyle;
        footStyle.color = PerfUI::Color(110, 120, 135, 255);
        footStyle.fontSize = 13.0f;
        backend.drawText("Phase 1 In-Game Verification: SUCCESS", { m_bounds.x + 24.0f, m_bounds.y + 235.0f }, footStyle);

        PerfUI::UIElement::render(backend);
    }
};

D3D11Hook& D3D11Hook::GetSingleton() {
    static D3D11Hook s_instance;
    return s_instance;
}

bool D3D11Hook::Install() {
    if (m_installed.load()) return true;

    auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
    if (!renderer) {
        SKSE::log::error("BSGraphics::Renderer::GetSingleton() returned null");
        return false;
    }

    auto& renderWindow = renderer->data.renderWindows[0];
    auto* swapChain = reinterpret_cast<IDXGISwapChain*>(renderWindow.swapChain);
    if (!swapChain) {
        SKSE::log::error("renderWindow.swapChain is null");
        return false;
    }

    m_hWnd = reinterpret_cast<HWND>(renderWindow.hWnd);

    // VMT Hook on swapChain instance
    void** originalVmt = *reinterpret_cast<void***>(swapChain);
    constexpr size_t kVmtEntries = 20;
    m_newVmt = new void*[kVmtEntries];
    std::memcpy(m_newVmt, originalVmt, sizeof(void*) * kVmtEntries);

    m_originalPresent = reinterpret_cast<PresentFn>(originalVmt[8]);
    m_originalResizeBuffers = reinterpret_cast<ResizeBuffersFn>(originalVmt[13]);

    m_newVmt[8] = reinterpret_cast<void*>(Hooked_Present);
    m_newVmt[13] = reinterpret_cast<void*>(Hooked_ResizeBuffers);

    *reinterpret_cast<void***>(swapChain) = m_newVmt;

    m_installed.store(true);
    SKSE::log::info("D3D11 Present and ResizeBuffers hooks installed successfully");
    return true;
}

void D3D11Hook::Uninstall() {
    if (!m_installed.load()) return;

    auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
    if (renderer && renderer->data.renderWindows[0].swapChain && m_newVmt) {
        auto* swapChain = reinterpret_cast<IDXGISwapChain*>(renderer->data.renderWindows[0].swapChain);
        void** currentVmt = *reinterpret_cast<void***>(swapChain);
        if (currentVmt == m_newVmt) {
            void** originalVmt = new void*[20];
            std::memcpy(originalVmt, m_newVmt, sizeof(void*) * 20);
            originalVmt[8] = reinterpret_cast<void*>(m_originalPresent);
            originalVmt[13] = reinterpret_cast<void*>(m_originalResizeBuffers);
            *reinterpret_cast<void***>(swapChain) = originalVmt;
        }
        delete[] m_newVmt;
        m_newVmt = nullptr;
    }

    CleanupRenderTarget();

    if (m_imguiInitialized.load() && m_imguiContext) {
        ImGuiContext* prevContext = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(m_imguiContext);
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(m_imguiContext);
        m_imguiContext = nullptr;
        if (prevContext && prevContext != m_imguiContext) {
            ImGui::SetCurrentContext(prevContext);
        } else {
            ImGui::SetCurrentContext(nullptr);
        }
        m_imguiInitialized.store(false);
    }

    m_installed.store(false);
}

void D3D11Hook::ToggleUI() {
    bool current = m_uiVisible.load();
    if (!current) {
        SetUIVisible(true);
        if (m_journalWindow) m_journalWindow->setVisible(true);
        if (m_mainMenuWindow) m_mainMenuWindow->setVisible(false);
    } else {
        if (m_journalWindow && m_journalWindow->isVisible()) {
            SetUIVisible(false);
        } else {
            if (m_journalWindow) m_journalWindow->setVisible(true);
            if (m_mainMenuWindow) m_mainMenuWindow->setVisible(false);
        }
    }
}

void D3D11Hook::ToggleMainMenu() {
    bool current = m_uiVisible.load();
    if (!current) {
        SetUIVisible(true);
        if (m_journalWindow) m_journalWindow->setVisible(false);
        if (m_mainMenuWindow) m_mainMenuWindow->setVisible(true);
    } else {
        if (m_mainMenuWindow && m_mainMenuWindow->isVisible()) {
            SetUIVisible(false);
        } else {
            if (m_journalWindow) m_journalWindow->setVisible(false);
            if (m_mainMenuWindow) m_mainMenuWindow->setVisible(true);
        }
    }
}

void D3D11Hook::SetUIVisible(bool visible) {
    m_uiVisible.store(visible);
    InputHook::GetSingleton().SetCaptureInput(visible);
    // In overlay mode, mouse stays with the UI tree while keyboard events go to the game.
    InputHook::GetSingleton().SetCaptureKeyboard(visible && !m_overlayMode.load());

    // m_uiVisible toggles input capture: RenderFrame draws the tree while any root
    // window is visible. Hide windows on close so they do not linger on screen.
    if (!visible) {
        if (m_journalWindow) m_journalWindow->setVisible(false);
        if (m_mainMenuWindow) m_mainMenuWindow->setVisible(false);
    }

    SkyrimSoundService::PlayUISound(visible ? "UIJournalOpen" : "UIJournalClose");

    if (visible) {
        SkyrimQuestService::GetSingleton().RequestQuestRefresh();
    }

    SKSE::GetTaskInterface()->AddTask([visible, modal = !m_overlayMode.load()]() {
        auto* controlMap = RE::ControlMap::GetSingleton();
        if (controlMap) {
            using UEFlag = RE::UserEvents::USER_EVENT_FLAG;
            using UType = std::underlying_type_t<UEFlag>;
            if (visible && modal) {
                // Disable gameplay controls so the game world does not react while modal UI is open
                const auto rawFlags = static_cast<UType>(UEFlag::kMovement) |
                                      static_cast<UType>(UEFlag::kLooking) |
                                      static_cast<UType>(UEFlag::kActivate) |
                                      static_cast<UType>(UEFlag::kFighting) |
                                      static_cast<UType>(UEFlag::kPOVSwitch) |
                                      static_cast<UType>(UEFlag::kSneaking) |
                                      static_cast<UType>(UEFlag::kJumping) |
                                      static_cast<UType>(UEFlag::kMainFour) |
                                      static_cast<UType>(UEFlag::kWheelZoom);
                controlMap->ToggleControls(static_cast<UEFlag>(rawFlags), false);
            } else {
                // Restore gameplay controls when modal UI closes or in overlay mode
                controlMap->ToggleControls(UEFlag::kAll, true);
            }
        }

        auto* ui = RE::UI::GetSingleton();
        if (ui) {
            auto hud = ui->GetMenu<RE::HUDMenu>(RE::HUDMenu::MENU_NAME);
            if (hud && hud->uiMovie) {
                hud->uiMovie->SetVisible(!(visible && modal));
            }
        }
    });

    SKSE::log::info("PerfUI visibility set to: {}", visible ? "Visible" : "Hidden");
}

bool D3D11Hook::HasVisibleRootWindows() const {
    return m_uiContext && m_uiContext->root() && m_uiContext->root()->hasVisibleChildren();
}

void D3D11Hook::EnsureUIContext() {
    if (m_uiContext) return;
    m_uiContext = std::make_unique<PerfUI::UIContext>();
    m_uiContext->setSoundCallback([](const std::string& soundId) {
        SkyrimSoundService::PlayUISound(soundId.c_str());
    });

    m_uiContext->overlayManager().setErrorCallback([](const std::string& name, const std::string& err) {
        SKSE::log::error("PerfUI Overlay '{}' threw exception: {}. Disabled.", name, err);
    });

    // Add retained-mode Nordic Journal Window (hidden by default)
    m_journalWindow = m_uiContext->root()->add<PerfUI::JournalWindow>();
    m_journalWindow->setVisible(false);
    m_journalWindow->onTrackQuest([](uint32_t formId, bool active) {
        SkyrimQuestService::GetSingleton().SetQuestActive(formId, active);
    });

    // Add retained-mode Main Menu Window (Phase 13)
    m_mainMenuWindow = m_uiContext->root()->add<PerfUI::MainMenuWindow>();
    m_mainMenuWindow->setVisible(false);
    m_mainMenuWindow->onContinueGame([this]() {
        SetUIVisible(false);
    });
    m_mainMenuWindow->onQuitToDesktop([]() {
        ::PostQuitMessage(0);
    });
}

void D3D11Hook::InitializeImGui(IDXGISwapChain* pSwapChain) {
    if (m_imguiInitialized.load()) return;

    pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&m_device));
    if (!m_device) {
        SKSE::log::error("Failed to retrieve ID3D11Device from IDXGISwapChain");
        return;
    }

    m_device->GetImmediateContext(&m_context);
    CreateRenderTarget(pSwapChain);

    IMGUI_CHECKVERSION();
    ImGuiContext* prevContext = ImGui::GetCurrentContext();
    m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // Initialize backend and load sharp TrueType vector fonts before creating DirectX 11 textures
    m_renderBackend = std::make_unique<PerfUI::ImGuiRenderBackend>();
    m_renderBackend->setD3D11Device(m_device, m_context);
    m_renderBackend->initFonts();

    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplDX11_Init(m_device, m_context);

    // Initialize PerfUI
    EnsureUIContext();
    m_uiContext->setRenderBackend(m_renderBackend.get());

    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    if (prevContext && prevContext != m_imguiContext) {
        ImGui::SetCurrentContext(prevContext);
    }
    m_imguiInitialized.store(true);
    SKSE::log::info("ImGui and PerfUI initialized inside Skyrim D3D11 pipeline");
}

void D3D11Hook::CreateRenderTarget(IDXGISwapChain* pSwapChain) {
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (SUCCEEDED(hr) && pBackBuffer) {
        m_device->CreateRenderTargetView(pBackBuffer, nullptr, &m_renderTargetView);
        pBackBuffer->Release();
    }
}

void D3D11Hook::CleanupRenderTarget() {
    if (m_renderTargetView) {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }
}

void D3D11Hook::RenderFrame() {
    bool uiVis = m_uiVisible.load();
    bool hasRootWindows = HasVisibleRootWindows();
    bool anyTreeVisible = uiVis || hasRootWindows;
    bool hasOverlays = m_uiContext && m_uiContext->hasVisibleOverlays();
    bool hasPendingTasks = m_uiContext && m_uiContext->hasPendingTasks();
    bool shouldShowCursor = IsCursorNeeded();

    if (!shouldShowCursor) {
        auto* mc = RE::MenuCursor::GetSingleton();
        if (mc && mc->showCursorCount > 0) {
            mc->SetCursorVisibility(false);
        }
    }

    if (!m_imguiInitialized.load() || (!anyTreeVisible && !shouldShowCursor && !hasOverlays && !hasPendingTasks) || !m_imguiContext) return;

    ImGuiContext* prevContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(m_imguiContext);

    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
    m_lastFrameTime = now;

    // Backup D3D11 state to prevent corrupting Skyrim's / ENB's render state
    ID3D11RasterizerState* oldRasterizerState = nullptr;
    m_context->RSGetState(&oldRasterizerState);

    ID3D11BlendState* oldBlendState = nullptr;
    FLOAT oldBlendFactor[4] = { 0 };
    UINT oldSampleMask = 0;
    m_context->OMGetBlendState(&oldBlendState, oldBlendFactor, &oldSampleMask);

    ID3D11DepthStencilState* oldDepthStencilState = nullptr;
    UINT oldStencilRef = 0;
    m_context->OMGetDepthStencilState(&oldDepthStencilState, &oldStencilRef);

    UINT numViewports = 1;
    D3D11_VIEWPORT oldViewport{};
    m_context->RSGetViewports(&numViewports, &oldViewport);

    ID3D11RenderTargetView* oldRTV = nullptr;
    ID3D11DepthStencilView* oldDSV = nullptr;
    m_context->OMGetRenderTargets(1, &oldRTV, &oldDSV);

    // Render ImGui & PerfUI
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    RECT clientRect;
    ::GetClientRect(m_hWnd, &clientRect);
    float width = static_cast<float>(clientRect.right - clientRect.left);
    float height = static_cast<float>(clientRect.bottom - clientRect.top);
    m_uiContext->setViewportSize({ width, height });

    // Use Windows hardware cursor - never draw duplicate ImGui software cursor
    ImGui::GetIO().MouseDrawCursor = false;
    ImGui::GetIO().WantSetMousePos = false;

    // Always keep Windows hardware cursor hidden - Skyrim renders its own native game cursor
    while (::ShowCursor(FALSE) >= 0);
    ::SetCursor(nullptr);

    if (anyTreeVisible) {

        // Poll mouse position every frame via GetCursorPos
        POINT pt;
        if (::GetCursorPos(&pt) && ::ScreenToClient(m_hWnd, &pt)) {
            float mx = static_cast<float>(pt.x);
            float my = static_cast<float>(pt.y);
            m_uiContext->onMouseMove({ mx, my });

            auto* mc = RE::MenuCursor::GetSingleton();
            if (mc) {
                mc->cursorPosX = mx;
                mc->cursorPosY = my;
            }
        }

        // Drain queued input from background hook
        auto queuedInputs = InputHook::GetSingleton().DrainInputQueue();
        for (const auto& ev : queuedInputs) {
            switch (ev.type) {
            case InputHook::QueuedInput::Type::Char:
                m_uiContext->onCharInput(ev.charCode);
                break;
            case InputHook::QueuedInput::Type::KeyDown:
                m_uiContext->onKeyDown(ev.keyCode);
                break;
            case InputHook::QueuedInput::Type::MouseMove:
                m_uiContext->onMouseMove({ ev.x, ev.y });
                break;
            case InputHook::QueuedInput::Type::MouseDown:
                m_uiContext->onMouseDown(ev.button, { ev.x, ev.y });
                break;
            case InputHook::QueuedInput::Type::MouseUp:
                m_uiContext->onMouseUp(ev.button, { ev.x, ev.y });
                break;
            case InputHook::QueuedInput::Type::MouseWheel:
                m_uiContext->onMouseWheel(ev.wheelDelta, { ev.x, ev.y });
                break;
            }
        }

        // Safely consume live quests and player stats loaded from Skyrim engine
        SkyrimDataBundle bundle;
        if (SkyrimQuestService::GetSingleton().ConsumeNewData(bundle)) {
            if (m_journalWindow) {
                m_journalWindow->setQuests(std::move(bundle.activeQuests), std::move(bundle.completedQuests));
                m_journalWindow->setPlayerStats(bundle.playerStats);
            }
        }
    }

    m_uiContext->update(dt);
    m_uiContext->render(*m_renderBackend, anyTreeVisible);

    if (shouldShowCursor) {
        auto* ui = RE::UI::GetSingleton();
        if (ui) {
            auto cursorMenu = ui->GetMenu(RE::CursorMenu::MENU_NAME);
            if (cursorMenu && cursorMenu->uiMovie && cursorMenu->uiMovie->GetVisible()) {
                cursorMenu->uiMovie->SetVisible(false);
            }
        }

        ImVec2 swordPos(0.0f, 0.0f);
        bool hasSwordPos = false;
        auto* mc = RE::MenuCursor::GetSingleton();
        bool isCapturingInput = InputHook::GetSingleton().IsCaptureInput();
        if (isCapturingInput) {
            POINT pt;
            if (::GetCursorPos(&pt) && ::ScreenToClient(m_hWnd, &pt)) {
                swordPos = ImVec2(static_cast<float>(pt.x), static_cast<float>(pt.y));
                hasSwordPos = true;
            }
        } else if (mc) {
            swordPos = ImVec2(mc->cursorPosX, mc->cursorPosY);
            hasSwordPos = true;
        } else {
            POINT pt;
            if (::GetCursorPos(&pt) && ::ScreenToClient(m_hWnd, &pt)) {
                swordPos = ImVec2(static_cast<float>(pt.x), static_cast<float>(pt.y));
                hasSwordPos = true;
            }
        }

        if (hasSwordPos) {
            DrawSkyrimSwordCursor(ImGui::GetForegroundDrawList(), swordPos);
        }
    }

    ImGui::Render();

    // Bind back buffer target view and render draw list
    m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Restore D3D11 state
    m_context->RSSetState(oldRasterizerState);
    if (oldRasterizerState) oldRasterizerState->Release();

    m_context->OMSetBlendState(oldBlendState, oldBlendFactor, oldSampleMask);
    if (oldBlendState) oldBlendState->Release();

    m_context->OMSetDepthStencilState(oldDepthStencilState, oldStencilRef);
    if (oldDepthStencilState) oldDepthStencilState->Release();

    m_context->RSSetViewports(numViewports, &oldViewport);

    m_context->OMSetRenderTargets(1, &oldRTV, oldDSV);
    if (oldRTV) oldRTV->Release();
    if (oldDSV) oldDSV->Release();

    if (prevContext && prevContext != m_imguiContext) {
        ImGui::SetCurrentContext(prevContext);
    }
}

HRESULT STDMETHODCALLTYPE D3D11Hook::Hooked_Present(
    IDXGISwapChain* pSwapChain,
    UINT SyncInterval,
    UINT Flags
) {
    static std::atomic<bool> s_presentThreadLogged{ false };
    if (!s_presentThreadLogged.exchange(true)) {
        SKSE::log::info(">>> Thread Check: D3D11 Hooked_Present Render Thread ID = {}", ::GetCurrentThreadId());
    }

    auto& hook = D3D11Hook::GetSingleton();
    if (!hook.m_imguiInitialized.load()) {
        hook.InitializeImGui(pSwapChain);
    }

    if (hook.m_uiContext) {
        WorldTrackingService::GetSingleton().Update();
        auto snapshot = WorldTrackingService::GetSingleton().GetLatestSnapshot();
        if (snapshot) {
            hook.m_uiContext->setWorldAnchorSnapshot(std::move(snapshot));
        }
    }

    bool anyTreeVisible = hook.m_uiVisible.load() || hook.HasVisibleRootWindows();
    bool hasOverlays = hook.m_uiContext && hook.m_uiContext->hasVisibleOverlays();
    bool hasPendingTasks = hook.m_uiContext && hook.m_uiContext->hasPendingTasks();
    bool cursorNeeded = hook.IsCursorNeeded();
    if (anyTreeVisible || hasOverlays || hasPendingTasks || cursorNeeded) {
        hook.RenderFrame();
    }

    return hook.m_originalPresent(pSwapChain, SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE D3D11Hook::Hooked_ResizeBuffers(
    IDXGISwapChain* pSwapChain,
    UINT BufferCount,
    UINT Width,
    UINT Height,
    DXGI_FORMAT NewFormat,
    UINT SwapChainFlags
) {
    auto& hook = D3D11Hook::GetSingleton();
    hook.CleanupRenderTarget();

    HRESULT hr = hook.m_originalResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    if (SUCCEEDED(hr)) {
        hook.CreateRenderTarget(pSwapChain);
    }
    return hr;
}

} // namespace PerfUI::Skyrim
