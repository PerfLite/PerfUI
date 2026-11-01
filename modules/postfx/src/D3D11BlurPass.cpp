#include "PerfUI/PostFx/D3D11BlurPass.h"
#include "shaders/VS_Fullscreen.h"
#include "shaders/PS_Downsample.h"
#include "shaders/PS_KawaseBlur.h"

#include <algorithm>
#include <cstring>

namespace PerfUI::PostFx {

namespace {

struct alignas(16) BlurConstants {
    float param0[2]; // halfPixel or pixelSize
    float param1;    // offset or iteration
    float padding;
};

template <typename T>
void SafeRelease(T*& resource) {
    if (resource) {
        resource->Release();
        resource = nullptr;
    }
}

} // namespace

D3D11BlurPass::~D3D11BlurPass() {
    Shutdown();
}

bool D3D11BlurPass::Init(ID3D11Device* device, ID3D11DeviceContext* context) {
    if (!device || !context) return false;
    Shutdown();

    m_device = device;
    m_context = context;

    // 1. Create Shaders
    HRESULT hr = m_device->CreateVertexShader(g_VS_Fullscreen, sizeof(g_VS_Fullscreen), nullptr, &m_vsFullscreen);
    if (FAILED(hr)) return false;

    hr = m_device->CreatePixelShader(g_PS_Downsample, sizeof(g_PS_Downsample), nullptr, &m_psDownsample);
    if (FAILED(hr)) return false;

    hr = m_device->CreatePixelShader(g_PS_KawaseBlur, sizeof(g_PS_KawaseBlur), nullptr, &m_psKawase);
    if (FAILED(hr)) return false;

    // 2. Create Constant Buffer
    D3D11_BUFFER_DESC cbDesc{};
    cbDesc.ByteWidth = sizeof(BlurConstants);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);
    if (FAILED(hr)) return false;

    // 3. Create Bilinear Clamp Sampler
    D3D11_SAMPLER_DESC sampDesc{};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0.0f;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_device->CreateSamplerState(&sampDesc, &m_sampler);
    if (FAILED(hr)) return false;

    // 4. Create Rasterizer State (No culling, solid)
    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.DepthClipEnable = FALSE;
    hr = m_device->CreateRasterizerState(&rsDesc, &m_rasterizerState);
    if (FAILED(hr)) return false;

    // 5. Create Opaque Blend State (Direct overwrite)
    D3D11_BLEND_DESC bDesc{};
    bDesc.RenderTarget[0].BlendEnable = FALSE;
    bDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = m_device->CreateBlendState(&bDesc, &m_blendState);
    if (FAILED(hr)) return false;

    // 6. Create Depth-Stencil State (Disabled)
    D3D11_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable = FALSE;
    dsDesc.StencilEnable = FALSE;
    hr = m_device->CreateDepthStencilState(&dsDesc, &m_depthStencilState);
    if (FAILED(hr)) return false;

    m_initialized = true;
    return true;
}

void D3D11BlurPass::ReleaseRenderTarget(ID3D11Texture2D*& tex, ID3D11RenderTargetView*& rtv, ID3D11ShaderResourceView*& srv) {
    SafeRelease(srv);
    SafeRelease(rtv);
    SafeRelease(tex);
}

void D3D11BlurPass::Shutdown() {
    ReleaseRenderTarget(m_down1Tex, m_down1RTV, m_down1SRV);
    ReleaseRenderTarget(m_pingTex, m_pingRTV, m_pingSRV);
    ReleaseRenderTarget(m_pongTex, m_pongRTV, m_pongSRV);

    SafeRelease(m_captureSRV);
    SafeRelease(m_captureTex);

    SafeRelease(m_sampler);
    SafeRelease(m_constantBuffer);
    SafeRelease(m_rasterizerState);
    SafeRelease(m_blendState);
    SafeRelease(m_depthStencilState);

    SafeRelease(m_vsFullscreen);
    SafeRelease(m_psDownsample);
    SafeRelease(m_psKawase);

    m_finalBlurredSRV = nullptr;
    m_hasValidBackdrop = false;
    m_initialized = false;
    m_width = 0;
    m_height = 0;
}

bool D3D11BlurPass::EnsureResources(uint32_t width, uint32_t height, DXGI_FORMAT format) {
    if (m_width == width && m_height == height && m_format == format && m_captureTex != nullptr) {
        return true;
    }

    ReleaseRenderTarget(m_down1Tex, m_down1RTV, m_down1SRV);
    ReleaseRenderTarget(m_pingTex, m_pingRTV, m_pingSRV);
    ReleaseRenderTarget(m_pongTex, m_pongRTV, m_pongSRV);
    SafeRelease(m_captureSRV);
    SafeRelease(m_captureTex);

    m_width = width;
    m_height = height;
    m_down1Width = (std::max)(1u, width / 2);
    m_down1Height = (std::max)(1u, height / 2);
    m_down2Width = (std::max)(1u, width / 4);
    m_down2Height = (std::max)(1u, height / 4);
    m_format = format;

    // 1. Capture texture (matches backbuffer format for CopySubresourceRegion)
    D3D11_TEXTURE2D_DESC capDesc{};
    capDesc.Width = m_width;
    capDesc.Height = m_height;
    capDesc.MipLevels = 1;
    capDesc.ArraySize = 1;
    capDesc.Format = format;
    capDesc.SampleDesc.Count = 1;
    capDesc.Usage = D3D11_USAGE_DEFAULT;
    capDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    HRESULT hr = m_device->CreateTexture2D(&capDesc, nullptr, &m_captureTex);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = m_device->CreateShaderResourceView(m_captureTex, &srvDesc, &m_captureSRV);
    if (FAILED(hr)) return false;

    // Intermediate render targets format
    DXGI_FORMAT rtFormat = (format == DXGI_FORMAT_B8G8R8A8_UNORM) ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;

    auto createRT = [this, rtFormat](uint32_t w, uint32_t h, ID3D11Texture2D*& tex, ID3D11RenderTargetView*& rtv, ID3D11ShaderResourceView*& srv) -> bool {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = rtFormat;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &tex))) return false;
        if (FAILED(m_device->CreateRenderTargetView(tex, nullptr, &rtv))) return false;
        if (FAILED(m_device->CreateShaderResourceView(tex, nullptr, &srv))) return false;
        return true;
    };

    if (!createRT(m_down1Width, m_down1Height, m_down1Tex, m_down1RTV, m_down1SRV)) return false;
    if (!createRT(m_down2Width, m_down2Height, m_pingTex, m_pingRTV, m_pingSRV)) return false;
    if (!createRT(m_down2Width, m_down2Height, m_pongTex, m_pongRTV, m_pongSRV)) return false;

    return true;
}

bool D3D11BlurPass::Process(IDXGISwapChain* swapChain, float blurRadius) {
    if (!m_initialized || !swapChain || !m_device || !m_context) return false;

    // 1. Fetch backbuffer from swapchain
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (FAILED(hr) || !backBuffer) return false;

    D3D11_TEXTURE2D_DESC bbDesc{};
    backBuffer->GetDesc(&bbDesc);

    if (!EnsureResources(bbDesc.Width, bbDesc.Height, bbDesc.Format)) {
        backBuffer->Release();
        return false;
    }

    // 2. Fast GPU DMA copy of game backbuffer into m_captureTex
    m_context->CopySubresourceRegion(m_captureTex, 0, 0, 0, 0, backBuffer, 0, nullptr);
    backBuffer->Release();

    // 3. Backup pipeline states modified by post-process passes
    ID3D11RenderTargetView* oldRTV = nullptr;
    ID3D11DepthStencilView* oldDSV = nullptr;
    m_context->OMGetRenderTargets(1, &oldRTV, &oldDSV);

    UINT oldNumVP = 1;
    D3D11_VIEWPORT oldVP{};
    m_context->RSGetViewports(&oldNumVP, &oldVP);

    ID3D11RasterizerState* oldRS = nullptr;
    m_context->RSGetState(&oldRS);

    ID3D11BlendState* oldBS = nullptr;
    FLOAT oldBlendFactor[4]{ 0 };
    UINT oldSampleMask = 0;
    m_context->OMGetBlendState(&oldBS, oldBlendFactor, &oldSampleMask);

    ID3D11DepthStencilState* oldDSS = nullptr;
    UINT oldStencilRef = 0;
    m_context->OMGetDepthStencilState(&oldDSS, &oldStencilRef);

    ID3D11SamplerState* oldSampler = nullptr;
    m_context->PSGetSamplers(0, 1, &oldSampler);

    ID3D11VertexShader* oldVS = nullptr;
    m_context->VSGetShader(&oldVS, nullptr, nullptr);

    ID3D11PixelShader* oldPS = nullptr;
    m_context->PSGetShader(&oldPS, nullptr, nullptr);

    D3D11_PRIMITIVE_TOPOLOGY oldTopology;
    m_context->IAGetPrimitiveTopology(&oldTopology);

    // Setup common blur pass state
    m_context->IASetInputLayout(nullptr);
    m_context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    m_context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_vsFullscreen, nullptr, 0);
    m_context->RSSetState(m_rasterizerState);
    m_context->OMSetBlendState(m_blendState, nullptr, 0xFFFFFFFF);
    m_context->OMSetDepthStencilState(m_depthStencilState, 0);
    m_context->PSSetSamplers(0, 1, &m_sampler);
    m_context->PSSetConstantBuffers(0, 1, &m_constantBuffer);

    auto updateConstants = [this](float p0x, float p0y, float p1) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            BlurConstants* cb = static_cast<BlurConstants*>(mapped.pData);
            cb->param0[0] = p0x;
            cb->param0[1] = p0y;
            cb->param1 = p1;
            cb->padding = 0.0f;
            m_context->Unmap(m_constantBuffer, 0);
        }
    };

    ID3D11RenderTargetView* nullRTV = nullptr;
    ID3D11ShaderResourceView* nullSRV = nullptr;

    // --- Pass 1: Downsample Full -> Half (Level 1) ---
    D3D11_VIEWPORT vp1{};
    vp1.Width = static_cast<float>(m_down1Width);
    vp1.Height = static_cast<float>(m_down1Height);
    vp1.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp1);
    m_context->OMSetRenderTargets(1, &m_down1RTV, nullptr);
    m_context->PSSetShader(m_psDownsample, nullptr, 0);
    m_context->PSSetShaderResources(0, 1, &m_captureSRV);
    updateConstants(0.5f / static_cast<float>(m_width), 0.5f / static_cast<float>(m_height), 1.0f);
    m_context->Draw(3, 0);

    m_context->OMSetRenderTargets(1, &nullRTV, nullptr);
    m_context->PSSetShaderResources(0, 1, &nullSRV);

    // --- Pass 2: Downsample Half -> Quarter (Level 2 Ping) ---
    D3D11_VIEWPORT vp2{};
    vp2.Width = static_cast<float>(m_down2Width);
    vp2.Height = static_cast<float>(m_down2Height);
    vp2.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp2);
    m_context->OMSetRenderTargets(1, &m_pingRTV, nullptr);
    m_context->PSSetShaderResources(0, 1, &m_down1SRV);
    updateConstants(0.5f / static_cast<float>(m_down1Width), 0.5f / static_cast<float>(m_down1Height), 1.0f);
    m_context->Draw(3, 0);

    m_context->OMSetRenderTargets(1, &nullRTV, nullptr);
    m_context->PSSetShaderResources(0, 1, &nullSRV);

    // --- Pass 3: Kawase Iteration 1 (Ping -> Pong) ---
    float radiusFactor = (std::clamp)(blurRadius, 0.5f, 3.0f);
    m_context->OMSetRenderTargets(1, &m_pongRTV, nullptr);
    m_context->PSSetShader(m_psKawase, nullptr, 0);
    m_context->PSSetShaderResources(0, 1, &m_pingSRV);
    updateConstants(1.0f / static_cast<float>(m_down2Width), 1.0f / static_cast<float>(m_down2Height), 0.8f * radiusFactor);
    m_context->Draw(3, 0);

    m_context->OMSetRenderTargets(1, &nullRTV, nullptr);
    m_context->PSSetShaderResources(0, 1, &nullSRV);

    // --- Pass 4: Kawase Iteration 2 (Pong -> Ping) ---
    m_context->OMSetRenderTargets(1, &m_pingRTV, nullptr);
    m_context->PSSetShaderResources(0, 1, &m_pongSRV);
    updateConstants(1.0f / static_cast<float>(m_down2Width), 1.0f / static_cast<float>(m_down2Height), 1.6f * radiusFactor);
    m_context->Draw(3, 0);

    m_context->OMSetRenderTargets(1, &nullRTV, nullptr);
    m_context->PSSetShaderResources(0, 1, &nullSRV);

    // --- Pass 5: Kawase Iteration 3 (Ping -> Pong) ---
    m_context->OMSetRenderTargets(1, &m_pongRTV, nullptr);
    m_context->PSSetShaderResources(0, 1, &m_pingSRV);
    updateConstants(1.0f / static_cast<float>(m_down2Width), 1.0f / static_cast<float>(m_down2Height), 2.5f * radiusFactor);
    m_context->Draw(3, 0);

    m_context->OMSetRenderTargets(1, &nullRTV, nullptr);
    m_context->PSSetShaderResources(0, 1, &nullSRV);

    // Set final blurred backdrop
    m_finalBlurredSRV = m_pongSRV;
    m_hasValidBackdrop = true;

    // 4. Restore previous pipeline states
    m_context->RSSetState(oldRS);
    if (oldRS) oldRS->Release();

    m_context->OMSetBlendState(oldBS, oldBlendFactor, oldSampleMask);
    if (oldBS) oldBS->Release();

    m_context->OMSetDepthStencilState(oldDSS, oldStencilRef);
    if (oldDSS) oldDSS->Release();

    m_context->RSSetViewports(oldNumVP, &oldVP);

    m_context->OMSetRenderTargets(1, &oldRTV, oldDSV);
    if (oldRTV) oldRTV->Release();
    if (oldDSV) oldDSV->Release();

    m_context->PSSetSamplers(0, 1, &oldSampler);
    if (oldSampler) oldSampler->Release();

    m_context->VSSetShader(oldVS, nullptr, 0);
    if (oldVS) oldVS->Release();

    m_context->PSSetShader(oldPS, nullptr, 0);
    if (oldPS) oldPS->Release();

    m_context->IASetPrimitiveTopology(oldTopology);

    return true;
}

} // namespace PerfUI::PostFx
