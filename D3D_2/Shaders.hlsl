//***************************************************************************************
// Shaders.hlsl - 顶点着色器和像素着色器（支持高亮显示）
//***************************************************************************************

// 常量缓冲区 - 对象变换矩阵和高亮颜色
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
    float4 gHighlightColor; // 高亮颜色（alpha控制混合）
};

// 顶点输入结构
struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
};

// 顶点输出/像素输入结构
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float4 Color : COLOR;
};

// 顶点着色器
VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    // 位置变换到裁剪空间
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

    // 这里暂时直接把法线当作世界空间法线使用（若将来有非统一缩放，要用独立世界矩阵变换法线）
    vout.NormalW = normalize(vin.Normal);

    // 传递顶点颜色
    vout.Color = vin.Color;

    return vout;
}

// 像素着色器
float4 PS(VertexOut pin) : SV_Target
{
    // 简单 Blinn-Phong / Lambert 光照：环境 + 漫反射
    float3 N = normalize(pin.NormalW);

    // 固定一个方向光（从右上前方照射）
    float3 L = normalize(float3(0.5f, 1.0f, -0.5f));

    // 环境光因子
    float3 ambient = 0.2f.xxx;

    // 漫反射强度
    float ndotl = saturate(dot(N, L));
    float3 diffuse = ndotl.xxx;

    float3 baseColor = pin.Color.rgb * (ambient + diffuse);
    float4 finalColor = float4(baseColor, pin.Color.a);

    // 选中高亮混合
    if (gHighlightColor.a > 0.0f)
    {
        finalColor = lerp(finalColor, gHighlightColor, gHighlightColor.a);
    }

    return finalColor;
}