#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <chrono>
#include <memory>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "PerfUI/PerfUI.h"
#include "../../src/backends/imgui/ImGuiRenderBackend.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global Direct3D 11 data
static ID3D11Device*           g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// PerfUI test instance
static std::unique_ptr<PerfUI::UIContext> g_uiContext;
static std::unique_ptr<PerfUI::ImGuiRenderBackend> g_renderBackend;

// Simple custom test component to demonstrate retained rendering
class DemoPanel : public PerfUI::UIElement {
public:
    DemoPanel() : PerfUI::UIElement("DemoPanel") {
        setFocusable(true);
    }

    void measure(PerfUI::Dimensions availableSize) override {
        // Simple fixed size demo card
        m_desiredSize = { 420.0f, 260.0f };
        PerfUI::UIElement::measure(availableSize);
    }

    void arrange(const PerfUI::Rect& finalRect) override {
        // Center the demo card inside the parent window
        float x = (finalRect.width - m_desiredSize.width) * 0.5f;
        float y = (finalRect.height - m_desiredSize.height) * 0.5f;
        PerfUI::UIElement::arrange(PerfUI::Rect{ x, y, m_desiredSize.width, m_desiredSize.height });
    }

    void render(PerfUI::UIRenderBackend& backend) override {
        if (!isVisible()) return;

        // Draw soft drop shadow
        backend.drawShadow(m_bounds, 12.0f, PerfUI::Color(0, 0, 0, 140), 16.0f, { 0.0f, 6.0f });

        // Select border color based on state
        PerfUI::Color borderColor = PerfUI::Color(72, 79, 88, 200);
        if (m_pressed) {
            borderColor = PerfUI::Color(255, 215, 0, 255); // Highlight
        } else if (m_hovered) {
            borderColor = PerfUI::Color::NordicGold();
        }

        // Card background (Dark slate)
        PerfUI::Color cardBg = m_hovered ? PerfUI::Color(26, 32, 42, 240) : PerfUI::Color::SlateCard();
        backend.drawRoundedRect(m_bounds, cardBg, 12.0f, borderColor, 2.0f);

        // Header Title
        PerfUI::TextStyle titleStyle;
        titleStyle.color = PerfUI::Color::NordicGold();
        titleStyle.fontSize = 20.0f;
        titleStyle.bold = true;
        backend.drawText("PerfUI Framework - Standalone Sandbox", { m_bounds.x + 20.0f, m_bounds.y + 20.0f }, titleStyle);

        // Subtitle / description
        PerfUI::TextStyle bodyStyle;
        bodyStyle.color = PerfUI::Color(200, 205, 215, 255);
        bodyStyle.fontSize = 15.0f;
        backend.drawText("Status: Retained-Mode UI Tree active", { m_bounds.x + 20.0f, m_bounds.y + 60.0f }, bodyStyle);
        backend.drawText("Backend: UIRenderBackend -> ImGuiBackend", { m_bounds.x + 20.0f, m_bounds.y + 85.0f }, bodyStyle);
        backend.drawText("Input: Mouse & Keyboard First (Gamepad skipped)", { m_bounds.x + 20.0f, m_bounds.y + 110.0f }, bodyStyle);

        // State indicator badge
        PerfUI::Rect badgeRect{ m_bounds.x + 20.0f, m_bounds.y + 160.0f, 380.0f, 40.0f };
        PerfUI::Color badgeColor = m_pressed ? PerfUI::Color(40, 80, 40, 220) : (m_hovered ? PerfUI::Color(35, 45, 60, 220) : PerfUI::Color(20, 25, 35, 220));
        backend.drawRoundedRect(badgeRect, badgeColor, 6.0f, borderColor, 1.0f);

        PerfUI::TextStyle badgeStyle;
        badgeStyle.color = m_pressed ? PerfUI::Color(120, 255, 120, 255) : (m_hovered ? PerfUI::Color::White() : PerfUI::Color(150, 160, 175, 255));
        badgeStyle.fontSize = 14.0f;

        const char* statusText = m_pressed ? "Action: Pressed! (Click dispatched)" : (m_hovered ? "Mouse Hovered! (Click anywhere on card)" : "Hover over this card with mouse");
        backend.drawText(statusText, { badgeRect.x + 16.0f, badgeRect.y + 12.0f }, badgeStyle);

        // Footer hint
        PerfUI::TextStyle footerStyle;
        footerStyle.color = PerfUI::Color(100, 110, 125, 255);
        footerStyle.fontSize = 13.0f;
        backend.drawText("Phase 0 & 1 Verification | Press ESC to Exit", { m_bounds.x + 20.0f, m_bounds.y + 225.0f }, footerStyle);

        // Render children
        PerfUI::UIElement::render(backend);
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Register window class
    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L, 0L,
        hInstance,
        nullptr, nullptr, nullptr, nullptr,
        L"PerfUI_Sandbox_Class",
        nullptr
    };
    ::RegisterClassExW(&wc);

    HWND hWnd = ::CreateWindowW(
        wc.lpszClassName,
        L"PerfUI Desktop Sandbox (DirectX 11)",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1280, 720,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!CreateDeviceD3D(hWnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hWnd, nCmdShow);
    ::UpdateWindow(hWnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Initialize PerfUI Core & Render Backend
    g_uiContext = std::make_unique<PerfUI::UIContext>();
    g_renderBackend = std::make_unique<PerfUI::ImGuiRenderBackend>();

    // Add our Nordic Quest Journal window to the UIContext root
    g_uiContext->root()->add<PerfUI::JournalWindow>();

    auto lastTime = std::chrono::high_resolution_clock::now();

    // Main loop
    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }
        if (done) break;

        // Calculate delta time
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        // Start Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Update PerfUI viewport size
        RECT rect;
        ::GetClientRect(hWnd, &rect);
        float width = static_cast<float>(rect.right - rect.left);
        float height = static_cast<float>(rect.bottom - rect.top);
        g_uiContext->setViewportSize({ width, height });

        // Update & Render PerfUI through UIRenderBackend abstraction
        g_uiContext->update(dt);
        g_uiContext->render(*g_renderBackend);

        // Rendering to DX11 SwapChain
        ImGui::Render();
        const float clearColor[4] = { 0.05f, 0.07f, 0.09f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // VSync enabled
    }

    // Cleanup
    g_uiContext.reset();
    g_renderBackend.reset();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hWnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext
    );
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return true;
    }

    if (g_uiContext) {
        switch (msg) {
        case WM_MOUSEMOVE: {
            float x = static_cast<float>(LOWORD(lParam));
            float y = static_cast<float>(HIWORD(lParam));
            g_uiContext->onMouseMove({ x, y });
            break;
        }
        case WM_LBUTTONDOWN: {
            float x = static_cast<float>(LOWORD(lParam));
            float y = static_cast<float>(HIWORD(lParam));
            g_uiContext->onMouseDown(0, { x, y });
            break;
        }
        case WM_LBUTTONUP: {
            float x = static_cast<float>(LOWORD(lParam));
            float y = static_cast<float>(HIWORD(lParam));
            g_uiContext->onMouseUp(0, { x, y });
            break;
        }
        case WM_MOUSEWHEEL: {
            short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            float delta = static_cast<float>(zDelta) / static_cast<float>(WHEEL_DELTA);
            POINT pt{ LOWORD(lParam), HIWORD(lParam) };
            ::ScreenToClient(hWnd, &pt);
            g_uiContext->onMouseWheel(delta, { static_cast<float>(pt.x), static_cast<float>(pt.y) });
            break;
        }
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                ::PostQuitMessage(0);
                return 0;
            }
            break;
        }
        default:
            break;
        }
    }

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
