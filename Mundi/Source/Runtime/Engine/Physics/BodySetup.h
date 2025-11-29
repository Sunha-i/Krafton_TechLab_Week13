#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// BodySetup.h
// 충돌 기하학 데이터 클래스 (공유 가능)
// ─────────────────────────────────────────────────────────────────────────────
//
// 언리얼 엔진의 UBodySetup과 유사.
// 여러 FBodyInstance가 동일한 UBodySetup을 공유하여 메모리 효율성 확보.
// ─────────────────────────────────────────────────────────────────────────────

#include "Object.h"
#include "Vector.h"
#include "UBodySetup.generated.h"

// 전방 선언 (PhysX 의존성 숨김)
namespace physx
{
    class PxRigidActor;
    class PxShape;
    class PxMaterial;
}

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

    // ═══════════════════════════════════════════════════════════════════════
    // Shape 생성
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * @brief PhysX Shape 생성 및 Actor에 부착
     * @param RigidActor 대상 PxRigidActor
     * @param DefaultMaterial 기본 PxMaterial (PhysMaterial이 없을 때 사용)
     * @param Scale 월드 스케일 (형상 크기에 적용)
     * @return 생성된 PxShape (nullptr이면 실패)
     */
    physx::PxShape* CreatePhysicsShape(
        physx::PxRigidActor* RigidActor,
        physx::PxMaterial* DefaultMaterial,
        const FVector& Scale = FVector::One()) const;

private:
    /** Box 형상 생성 */
    physx::PxShape* CreateBoxShape(
        physx::PxRigidActor* RigidActor,
        physx::PxMaterial* Material,
        const FVector& Scale) const;

    /** Sphere 형상 생성 */
    physx::PxShape* CreateSphereShape(
        physx::PxRigidActor* RigidActor,
        physx::PxMaterial* Material,
        const FVector& Scale) const;

    /** Capsule 형상 생성 */
    physx::PxShape* CreateCapsuleShape(
        physx::PxRigidActor* RigidActor,
        physx::PxMaterial* Material,
        const FVector& Scale) const;
};
