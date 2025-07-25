#include "Test.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// ヴィネット用の定数バッファ
cbuffer VignetteConstants : register(b0) // このシェーダー専用 b0
{
    float32_t gVignetteIntensity; // ヴィネットの強さ
    float32_t gVignetteRadius; // ヴィネットの中心からの範囲
};

struct PixcelShaderOutput
{
    float32_t4 color : SV_Target0;
};

PixcelShaderOutput main(VertexShaderOutput input)
{
    PixcelShaderOutput output;
    
    // UV座標を使ってテクスチャから色をサンプリング
    float32_t2 uv = input.texcooed;
    float32_t4 textureColor = gTexture.Sample(gSampler, uv);

    // ▼▼▼ ココからヴィネット処理 ▼▼▼

    // UV座標の中心を (0.5, 0.5) に移動
    float32_t2 centeredUV = uv - float32_t2(0.5f, 0.5f);

    // 中心からの距離を計算
    float32_t dist = length(centeredUV);
    
    // gVignetteRadiusを超えると、どんどん暗くなるような係数を計算
    float32_t vignetteFactor = smoothstep(gVignetteRadius, gVignetteRadius + gVignetteIntensity, dist);

    // 最終的な色にヴィネット効果を適用
    output.color.rgb = lerp(textureColor.rgb, float32_t3(0.0f, 0.0f, 0.0f), vignetteFactor);
    output.color.a = textureColor.a; // アルファ値はそのまま

    return output;
}