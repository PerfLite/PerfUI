#pragma once

#include "PerfUI/Types.h"
#include <d3d11.h>
#include <dxgi.h>
#include <cstdint>

namespace PerfUI::PostFx {

class PERFUI_API D3D11BlurPass {
public:
    D3D11BlurPass() = default;
    ~D3D11BlurPass();

    bool Init(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Captures current backbuffer from swapchain and executes multi-pass Dual Kawase blur.
    bool Process(IDXGISwapChain* swapChain, float blurRadius = 1.0f);

    ID3D11ShaderResourceView* GetBlurredSRV() const { return m_finalBlurredSRV; }
    bool HasValidBackdrop() const { return m_hasValidBackdrop && m_finalBlurredSRV != nullptr; }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    void InvalidateBackdrop() { m_hasValidBackdrop = false; }

private:
    bool EnsureResources(uint32_t width, uint32_t height, DXGI_FORMAT format);
    void ReleaseRenderTarget(ID3D11Texture2D*& tex, ID3D11RenderTargetView*& rtv, ID3D11ShaderResourceView*& srv);

    ID3D11Device* m_device{ nullptr };
    ID3D11DeviceContext* m_context{ nullptr };

    ID3D11VertexShader* m_vsFullscreen{ nullptr };
    ID3D11PixelShader* m_psDownsample{ nullptr };
    ID3D11PixelShader* m_psKawase{ nullptr };

    ID3D11SamplerState* m_sampler{ nullptr };
    ID3D11Buffer* m_constantBuffer{ nullptr };
    ID3D11RasterizerState* m_rasterizerState{ nullptr };
    ID3D11BlendState* m_blendState{ nullptr };
    ID3D11DepthStencilState* m_depthStencilState{ nullptr };

    ID3D11Texture2D* m_captureTex{ nullptr };
    ID3D11ShaderResourceView* m_captureSRV{ nullptr };

    ID3D11Texture2D* m_down1Tex{ nullptr };
    ID3D11RenderTargetView* m_down1RTV{ nullptr };
    ID3D11ShaderResourceView* m_down1SRV{ nullptr };

    ID3D11Texture2D* m_pingTex{ nullptr };
    ID3D11RenderTargetView* m_pingRTV{ nullptr };
    ID3D11ShaderResourceView* m_pingSRV{ nullptr };

    ID3D11Texture2D* m_pongTex{ nullptr };
    ID3D11RenderTargetView* m_pongRTV{ nullptr };
    ID3D11ShaderResourceView* m_pongSRV{ nullptr };

    ID3D11ShaderResourceView* m_finalBlurredSRV{ nullptr };

    uint32_t m_width{ 0 };
    uint32_t m_height{ 0 };
    uint32_t m_down1Width{ 0 };
    uint32_t m_down1Height{ 0 };
    uint32_t m_down2Width{ 0 };
    uint32_t m_down2Height{ 0 };
    DXGI_FORMAT m_format{ DXGI_FORMAT_UNKNOWN };

    bool m_initialized{ false };
    bool m_hasValidBackdrop{ false };
};

} // namespace PerfUI::PostFx
