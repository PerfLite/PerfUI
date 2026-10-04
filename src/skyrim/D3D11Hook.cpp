#include "D3D11Hook.h"
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include "InputHook.h"
#include "PerfUI/JournalWindow.h"
#include "SkyrimQuestService.h"
#include "SkyrimSoundService.h"

namespace PerfUI::Skyrim {

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

    SkyrimSoundService::PlayUISound(visible ? "UIJournalOpen" : "UIJournalClose");

    if (visible) {
        SkyrimQuestService::GetSingleton().RequestQuestRefresh();
    }

    SKSE::GetTaskInterface()->AddTask([visible]() {
        auto* controlMap = RE::ControlMap::GetSingleton();
        if (controlMap) {
            using UEFlag = RE::UserEvents::USER_EVENT_FLAG;
            controlMap->ToggleControls(UEFlag::kAll, !visible);
        }
    });

    SKSE::log::info("PerfUI visibility set to: {}", visible ? "Visible" : "Hidden");
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
    m_renderBackend->initFonts();

    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplDX11_Init(m_device, m_context);

    // Initialize PerfUI
    m_uiContext = std::make_unique<PerfUI::UIContext>();
    m_uiContext->setSoundCallback([](const std::string& soundId) {
        SkyrimSoundService::PlayUISound(soundId.c_str());
    });

    // Add retained-mode Nordic Journal Window
    m_journalWindow = m_uiContext->root()->add<PerfUI::JournalWindow>();
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
    if (!m_imguiInitialized.load() || !m_uiVisible.load() || !m_imguiContext) return;

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

    // Draw software cursor inside Skyrim when UI is visible
    ImGui::GetIO().MouseDrawCursor = m_uiVisible.load();

    // Drain queued input from background hook
    auto queuedInputs = InputHook::GetSingleton().DrainInputQueue();
    for (const auto& ev : queuedInputs) {
        if (ev.type == InputHook::QueuedInput::Type::Char) {
            m_uiContext->onCharInput(ev.charCode);
        } else if (ev.type == InputHook::QueuedInput::Type::KeyDown) {
            m_uiContext->onKeyDown(ev.keyCode);
        }
    }

    // Directly synchronize UIContext pointer with ImGui's Win32 mouse state
    const auto& io = ImGui::GetIO();
    m_uiContext->onMouseMove({ io.MousePos.x, io.MousePos.y });
    if (io.MouseClicked[0]) {
        m_uiContext->onMouseDown(0, { io.MousePos.x, io.MousePos.y });
    }
    if (io.MouseReleased[0]) {
        m_uiContext->onMouseUp(0, { io.MousePos.x, io.MousePos.y });
    }
    if (io.MouseClicked[1]) {
        m_uiContext->onMouseDown(1, { io.MousePos.x, io.MousePos.y });
    }
    if (io.MouseReleased[1]) {
        m_uiContext->onMouseUp(1, { io.MousePos.x, io.MousePos.y });
    }
    if (io.MouseWheel != 0.0f) {
        m_uiContext->onMouseWheel(io.MouseWheel, { io.MousePos.x, io.MousePos.y });
    }

    // Safely consume live quests and player stats loaded from Skyrim engine
    SkyrimDataBundle bundle;
    if (SkyrimQuestService::GetSingleton().ConsumeNewData(bundle)) {
        if (m_journalWindow) {
            m_journalWindow->setQuests(std::move(bundle.activeQuests), std::move(bundle.completedQuests));
            m_journalWindow->setPlayerStats(bundle.playerStats);
        }
    }

    m_uiContext->update(dt);
    m_uiContext->render(*m_renderBackend);

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
    auto& hook = D3D11Hook::GetSingleton();
    if (!hook.m_imguiInitialized.load()) {
        hook.InitializeImGui(pSwapChain);
    }

    if (hook.m_uiVisible.load()) {
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
