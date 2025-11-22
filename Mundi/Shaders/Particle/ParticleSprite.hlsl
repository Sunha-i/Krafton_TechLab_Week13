// Particle Sprite Shader
// CPU에서 시뮬레이션, GPU에서 빌보드 정렬 및 Z축 회전

// b1: ViewProjBuffer (VS)
cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

// VS_INPUT은 FParticleSpriteVertex 구조체와 일치해야 함
struct VS_INPUT
{
    float3 WorldPosition : POSITION;    // 파티클 중심 위치
    float Rotation : TEXCOORD0;         // Z축 회전 (라디안)
    float2 UV : TEXCOORD1;              // 쿼드 코너 UV
    float2 Size : TEXCOORD2;            // 파티클 크기
    float4 Color : COLOR0;              // 파티클 색상
    float RelativeTime : TEXCOORD3;     // 상대 시간 (0~1)
};

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
    float RelativeTime : TEXCOORD1;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
};

Texture2D ParticleTexture : register(t0);
SamplerState LinearSampler : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;

    // 로컬 오프셋 계산 (UV 0.5 중심 기준)
    float2 localOffset = (input.UV - 0.5f) * input.Size;

    // Z축 회전 적용 (카메라 Forward 축 기준 회전)
    float cosR = cos(input.Rotation);
    float sinR = sin(input.Rotation);
    float2 rotatedOffset;
    rotatedOffset.x = localOffset.x * cosR - localOffset.y * sinR;
    rotatedOffset.y = localOffset.x * sinR + localOffset.y * cosR;

    // 뷰 공간에서 오프셋 생성 후 월드 공간으로 변환 (빌보드)
    float3 viewSpaceOffset = float3(rotatedOffset.x, rotatedOffset.y, 0.0f);
    float3 worldOffset = mul(float4(viewSpaceOffset, 0.0f), InverseViewMatrix).xyz;

    // 월드 위치에 빌보드 오프셋 적용
    float3 worldPos = input.WorldPosition + worldOffset;

    // View-Projection 변환
    output.Position = mul(float4(worldPos, 1.0f), mul(ViewMatrix, ProjectionMatrix));
    output.UV = input.UV;
    output.Color = input.Color;
    output.RelativeTime = input.RelativeTime;

    return output;
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;

    // 텍스처 샘플링
    float4 texColor = ParticleTexture.Sample(LinearSampler, input.UV);

    // 알파 테스트 (거의 투명한 픽셀 제거)
    if (texColor.a < 0.01f)
        discard;

    // 텍스처 색상과 파티클 색상 곱하기
    output.Color = texColor * input.Color;

    return output;
}
