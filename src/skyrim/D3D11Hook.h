#pragma once

#include "Pch.h"
#include "../../src/backends/imgui/ImGuiRenderBackend.h"
#include "PerfUI/JournalWindow.h"
#include "PerfUI/MainMenuWindow.h"

struct ImGuiContext;

namespace PerfUI::Skyrim {

class D3D11Hook {
public:
    static D3D11Hook& GetSingleton();

    bool Install();
    void Uninstall();

    void ToggleUI();
    void ToggleMainMenu();
    void SetUIVisible(bool visible);
    bool IsUIVisible() const { return m_uiVisible.load(); }

    void EnsureUIContext();
    UIContext* GetContext() {
        if (!m_uiContext) {
            EnsureUIContext();
        }
        return m_uiContext.get();
    }
    ImGuiContext* GetImGuiContext() const { return m_imguiContext; }

private:
    D3D11Hook() = default;
    ~D3D11Hook() = default;

    static HRESULT STDMETHODCALLTYPE Hooked_Present(
        IDXGISwapChain* pSwapChain,
        UINT SyncInterval,
        UINT Flags
    );

    static HRESULT STDMETHODCALLTYPE Hooked_ResizeBuffers(
        IDXGISwapChain* pSwapChain,
        UINT BufferCount,
        UINT Width,
        UINT Height,
        DXGI_FORMAT NewFormat,
        UINT SwapChainFlags
    );

    void InitializeImGui(IDXGISwapChain* pSwapChain);
    void CreateRenderTarget(IDXGISwapChain* pSwapChain);
    void CleanupRenderTarget();
    void RenderFrame();

    using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
    using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    PresentFn m_originalPresent{ nullptr };
    ResizeBuffersFn m_originalResizeBuffers{ nullptr };
    void** m_newVmt{ nullptr };

    std::atomic<bool> m_installed{ false };
    std::atomic<bool> m_imguiInitialized{ false };
    std::atomic<bool> m_uiVisible{ false };

    HWND m_hWnd{ nullptr };
    ID3D11Device* m_device{ nullptr };
    ID3D11DeviceContext* m_context{ nullptr };
    ID3D11RenderTargetView* m_renderTargetView{ nullptr };
    ImGuiContext* m_imguiContext{ nullptr };

    std::unique_ptr<PerfUI::UIContext> m_uiContext;
    std::unique_ptr<PerfUI::ImGuiRenderBackend> m_renderBackend;
    PerfUI::JournalWindow* m_journalWindow{ nullptr };
    PerfUI::MainMenuWindow* m_mainMenuWindow{ nullptr };

    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
};

} // namespace PerfUI::Skyrim
