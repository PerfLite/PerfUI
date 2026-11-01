Texture2D g_Texture : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer BlurParams : register(b0) {
    float2 u_halfPixel;
    float  u_offset;
    float  u_padding;
};

struct VSOutput {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

float4 main(VSOutput input) : SV_Target {
    float2 uv = input.uv;
    float2 d = u_halfPixel * u_offset;

    float4 col = g_Texture.Sample(g_Sampler, uv) * 4.0f;
    col += g_Texture.Sample(g_Sampler, uv - d);
    col += g_Texture.Sample(g_Sampler, uv + d);
    col += g_Texture.Sample(g_Sampler, uv + float2(d.x, -d.y));
    col += g_Texture.Sample(g_Sampler, uv - float2(d.x, -d.y));

    return col * 0.125f;
}
