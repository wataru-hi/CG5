#include "Test.hlsli"

Texture2D<float32_t4> gTexture : register(t0); // SRV register => t
SamplerState gSampler : register(s0); // Sampler register => s

struct PixcelShaderOutput
{
    float32_t4 color : SV_Target0;
};

PixcelShaderOutput main(VertexShaderOutput input)
{
    PixcelShaderOutput output;
    
    float32_t2 uv = input.texcooed;
    float32_t4 textureColor = gTexture.Sample(gSampler, uv);

    float32_t value = dot(textureColor.rgb, float32_t3(0.21225f, 0.7154f, 0.0721f));
    output.color = float32_t4(value, value, value, textureColor.a);
    //output.color = textureColor; // non effect

    return output;
}