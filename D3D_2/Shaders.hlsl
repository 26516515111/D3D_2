cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
    float4 gHighlightColor;

    float3 gBaseColor;
    float gSpecularStrength;

    float gMatShininess;
    float gTexScale;
    float gTexOffsetU;
    float gTexOffsetV;

    int gTexMappingMode;
    int gTexStyle;
    int gHasTexture;
    float _padObj;
};

cbuffer cbPerPass : register(b1)
{
    float3 gLightPosW;
    float gAmbient;

    float gDiffuse;
    float gSpecular;
    float gShininess;
    float _pad0;

    float3 gEyePosW;
    float _pad1;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float3 PosW : POSITION1;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

    // 正确性=A：模型空间≈世界空间（演示编辑）
    vout.PosW = vin.PosL;
    vout.NormalW = normalize(vin.Normal);
    return vout;
}

// 生成 UV：按 mapping mode 从位置和法线生成
float2 CalcUV(float3 posW, float3 normalW, int mode)
{
    // posW 大致在 [-1,1] or [-radius,radius]，先做一个缩放
    float3 p = posW;

    if (mode == 0) // Planar: pick best axis by normal
    {
        float3 n = abs(normalW);
        if (n.y >= n.x && n.y >= n.z)
            return p.xz;           // top/bottom
        else if (n.x >= n.y && n.x >= n.z)
            return float2(p.z, p.y); // left/right
        else
            return float2(p.x, p.y); // front/back
    }
    else if (mode == 1) // Cylindrical: around Y; caps fall back to planar
    {
        if (abs(normalW.y) > 0.75f)
        {
            return p.xz; // caps
        }

        float theta = atan2(p.z, p.x); // [-pi,pi]
        float u = (theta / 6.2831853f) + 0.5f;
        float v = p.y; // 直接用 y，演示用
        return float2(u, v);
    }
    else // Spherical
    {
        float r = max(length(p), 1e-5f);
        float theta = atan2(p.z, p.x);
        float phi = acos(saturate(p.y / r));
        float u = (theta / 6.2831853f) + 0.5f;
        float v = phi / 3.1415926f;
        return float2(u, v);
    }
}

float Pattern(float2 uv, int style)
{
    // 统一缩放由 gTexScale 控制，这里只决定图案形态
    if (style == 0) // Checker
    {
        float2 t = floor(uv * 8.0f);
        return fmod(t.x + t.y, 2.0f);
    }
    else if (style == 1) // Stripes
    {
        float v = floor(uv.x * 12.0f);
        return fmod(v, 2.0f);
    }
    else // ImagePlaceholder（选了图片但不加载，给个更明显的“噪声感”图案）
    {
        float v = sin(uv.x * 30.0f) * cos(uv.y * 30.0f);
        return step(0.0f, v);
    }
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 N = normalize(pin.NormalW);
    float3 L = normalize(gLightPosW - pin.PosW);
    float3 V = normalize(gEyePosW - pin.PosW);
    float3 H = normalize(L + V);

    float ndotl = saturate(dot(N, L));
    float spec = pow(saturate(dot(N, H)), gMatShininess);

    float3 ambient = gAmbient.xxx;
    float3 diffuse = (gDiffuse * ndotl).xxx;
    float3 specular = (gSpecular * gSpecularStrength * spec).xxx;

    float2 uv = CalcUV(pin.PosW, pin.NormalW, gTexMappingMode);
    uv = uv * gTexScale + float2(gTexOffsetU, gTexOffsetV);

    float3 texColor;
    if (gHasTexture == 1)
    {
        texColor = gTexture.Sample(gSampler, uv).rgb;
    }
    else
    {
        float p = Pattern(uv, gTexStyle);
        texColor = lerp(float3(0.1f, 0.1f, 0.1f), float3(1.0f, 1.0f, 1.0f), p);
    }

    float3 baseColor = (gBaseColor * texColor) * (ambient + diffuse) + specular;
    float4 finalColor = float4(baseColor, 1.0f);

    if (gHighlightColor.a > 0.0f)
    {
        finalColor = lerp(finalColor, gHighlightColor, gHighlightColor.a);
    }

    return finalColor;
}