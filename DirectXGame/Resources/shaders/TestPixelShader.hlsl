#include "Test.hlsli"

struct PixcelShaderOutput
{
    float32_t4 color : SV_Target0;
};

PixcelShaderOutput main(VertexShaderOutput input)
{
    PixcelShaderOutput output;
    float32_t2 uv = input.texcooed;
    
    // https://learn.microsoft.com/ja-jp/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math
    // 位置セット ( x y z w ) か カラーセット ( r g b a )でアクセスできる
    output.color = float32_t4(uv.x, uv.y, 0.0f, 1.0f);
    
    return output;
}