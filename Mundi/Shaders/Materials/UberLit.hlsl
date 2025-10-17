//================================================================================================
// Filename:      UberLit.hlsl
// Description:   오브젝트 표면 렌더링을 위한 기본 Uber 셰이더.
//================================================================================================

// --- 전역 상수 정의 ---
#define NUM_POINT_LIGHT 4
#define NUM_SPOT_LIGHT 4

// --- 조명 정보 구조체 ---
struct FAmbientLightInfo
{
    // ...
};

struct FDirectionalLightInfo
{
    // ...
};

struct FPointLightInfo
{
    // ...
};

struct FSpotLightInfo
{
    // ...
};

// --- 상수 버퍼 (Constant Buffers) ---
cbuffer PerObject : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}; 

cbuffer Lighting : register(b1)
{
    FAmbientLightInfo Ambient;
    FDirectionalLightInfo Directional;
    FPointLightInfo PointLights[NUM_POINT_LIGHT];
    FSpotLightInfo SpotLights[NUM_SPOT_LIGHT];
};

// --- 텍스처 및 샘플러 리소스 ---
Texture2D g_DiffuseTexColor : register(t0);
SamplerState g_Sample : register(s0);

// --- 셰이더 입출력 구조체 ---
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL0;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

// --- 유틸리티 함수 ---
float4 CalculateAmbientLight(FAmbientLightInfo info)
{
    float4 result;
    // ...
    return result;
}

//================================================================================================
// 버텍스 셰이더 (Vertex Shader)
//================================================================================================
PS_INPUT mainVS(VS_INPUT Input)
{
    PS_INPUT Out;
#if LIGHTING_MODEL_GOURAUD

#elif LIGHTING_MODEL_LAMBERT

#elif LIGHTING_MODEL_PHONG

#endif
    return Out;
}

//================================================================================================
// 픽셀 셰이더 (Pixel Shader)
//================================================================================================
float4 mainPS(PS_INPUT Input) : SV_TARGET
{
    float4 finalPixel = Input.Color;
    //finalPixel += Emissive;
#if LIGHTING_MODEL_GOURAUD
#elif LIGHTING_MODEL_LAMBERT
    finalPixel += CalculateAmbientLight(// ...);
    for(It : PointLights)
    {
        finalPixel += CalculatePointLight(// ...);
    }
#elif LIGHTING_MODEL_PHONG
    // Specular Reflectance
#endif
    return finalPixel;
}