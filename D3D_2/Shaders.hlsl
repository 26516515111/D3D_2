//***************************************************************************************
// Shaders.hlsl - 顶点着色器和像素着色器（支持高亮显示）
//***************************************************************************************

// 常量缓冲区 - 对象变换矩阵和高亮颜色
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
    float4 gHighlightColor; // 新增：高亮颜色（alpha通道控制混合程度）
};

// 顶点输入结构
struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

// 顶点输出/像素输入结构
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

//=============================================================================
// 顶点着色器
//=============================================================================
VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    // 将顶点位置从局部空间变换到齐次裁剪空间
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

    // 直接传递顶点颜色到像素着色器
    vout.Color = vin.Color;

    return vout;
}

//=============================================================================
// 像素着色器
//=============================================================================
float4 PS(VertexOut pin) : SV_Target
{
    // 混合原始颜色和高亮颜色
    float4 finalColor = pin.Color;
    
    // 如果高亮颜色有alpha值，则混合
    if (gHighlightColor.a > 0.0f)
    {
        finalColor = lerp(pin.Color, gHighlightColor, gHighlightColor.a);
    }
    
    return finalColor;
}