#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// BodySetup.h
// 충돌 기하학 데이터 클래스 (공유 가능)
// ─────────────────────────────────────────────────────────────────────────────
//
// 언리얼 엔진의 UBodySetup과 유사.
// 여러 FBodyInstance가 동일한 UBodySetup을 공유하여 메모리 효율성 확보.
//
// PhysX 의존성이 없는 순수 데이터 클래스.
// Shape 생성은 BodySetupImpl.h의 BodySetupHelper 네임스페이스에서 담당.
// ─────────────────────────────────────────────────────────────────────────────

#include "Object.h"
#include "Vector.h"
#include "UBodySetup.generated.h"

/**
 * @brief 충돌 형상 타입
 */
enum class EBodySetupType : uint8
{
    None,
    Box,
    Sphere,
    Capsule,
    // 향후 확장: Convex, TriMesh
};

/**
 * @brief 충돌 기하학 데이터 (공유 가능)
 *
 * 언리얼 엔진의 UBodySetup과 유사.
 * 여러 FBodyInstance가 동일한 UBodySetup을 공유하여 메모리 효율성 확보.
 *
 * 이 클래스는 순수 데이터만 보유하며, PhysX 의존성이 없습니다.
 * 실제 PhysX Shape 생성은 BodySetupImpl.h의 BodySetupHelper에서 담당합니다.
 */
UCLASS(DisplayName = "충돌 기하 데이터", Description = "공유 가능한 충돌 기하 데이터 입니다")
class UBodySetup : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UBodySetup();
    virtual ~UBodySetup() = default;

    // ═══════════════════════════════════════════════════════════════════════
    // 형상 데이터
    // ═══════════════════════════════════════════════════════════════════════

    /** 형상 타입 */
    EBodySetupType BodyType = EBodySetupType::None;

    /** Box 형상용 - Half Extent (Mundi 좌표계) */
    FVector BoxExtent = FVector(50.0f, 50.0f, 50.0f);

    /** Sphere/Capsule 형상용 - 반지름 */
    float SphereRadius = 50.0f;

    /** Capsule 형상용 - Half Height (반구 제외) */
    float CapsuleHalfHeight = 50.0f;
};
