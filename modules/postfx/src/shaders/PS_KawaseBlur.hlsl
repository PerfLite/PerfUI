Texture2D g_Texture : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer BlurParams : register(b0) {
    float2 u_pixelSize;
    float  u_iteration;
    float  u_padding;
};

struct VSOutput {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target {
    float2 uv = input.uv;
    float2 d = u_pixelSize * (u_iteration + 0.5f);

    float4 sum = 0.0f;
    sum += g_Texture.Sample(g_Sampler, uv + float2(-d.x * 2.0f, 0.0f));
    sum += g_Texture.Sample(g_Sampler, uv + float2(-d.x,  d.y)) * 2.0f;
    sum += g_Texture.Sample(g_Sampler, uv + float2( 0.0f, d.y * 2.0f));
    sum += g_Texture.Sample(g_Sampler, uv + float2( d.x,  d.y)) * 2.0f;
    sum += g_Texture.Sample(g_Sampler, uv + float2( d.x * 2.0f, 0.0f));
    sum += g_Texture.Sample(g_Sampler, uv + float2( d.x, -d.y)) * 2.0f;
    sum += g_Texture.Sample(g_Sampler, uv + float2( 0.0f, -d.y * 2.0f));
    sum += g_Texture.Sample(g_Sampler, uv + float2(-d.x, -d.y)) * 2.0f;

    return sum * (1.0f / 12.0f);
}
