struct PixcelShaderOutput
{
    float color : SV_Target0;
};

PixcelShaderOutput main()
{
    PixcelShaderOutput output;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    return output;
}