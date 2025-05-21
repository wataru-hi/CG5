struct PixcelShaderOutput
{
    float32_t4 color : SV_Target0;
};

PixcelShaderOutput main()
{
    PixcelShaderOutput output;
    output.color = float32_t4(1.0f, 1.0f, 1.0f, 1.0f);
    return output;
}