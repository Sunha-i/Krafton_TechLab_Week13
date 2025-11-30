#include "pch.h"
#include "BodySetup.h"

// ═══════════════════════════════════════════════════════════════════════════
// 기본값 상수 정의 (단일 정의 지점)
// ═══════════════════════════════════════════════════════════════════════════

const FVector UBodySetup::DefaultBoxExtent = FVector(0.5f, 0.5f, 0.5f);
const float UBodySetup::DefaultSphereRadius = 0.5f;
const float UBodySetup::DefaultCapsuleRadius = 0.5f;
const float UBodySetup::DefaultCapsuleHalfHeight = 1.0f;

// ═══════════════════════════════════════════════════════════════════════════
// 생성자
// ═══════════════════════════════════════════════════════════════════════════

UBodySetup::UBodySetup()
    : BoxExtent(DefaultBoxExtent)
    , SphereRadius(DefaultSphereRadius)
    , CapsuleHalfHeight(DefaultCapsuleHalfHeight)
{
}
