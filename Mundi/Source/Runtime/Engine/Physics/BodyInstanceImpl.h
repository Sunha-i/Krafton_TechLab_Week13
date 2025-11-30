#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// BodyInstanceImpl.h
// FBodyInstance의 PhysX 구현부 (PIMPL - Physics 모듈 내부 전용)
// ─────────────────────────────────────────────────────────────────────────────
//
// 주의: 이 헤더는 Physics 모듈 내부에서만 사용해야 합니다.
//       외부 모듈에서 include하지 마세요.
// ─────────────────────────────────────────────────────────────────────────────

#include <PxPhysicsAPI.h>

struct FBodyInstance;

/**
 * @brief FBodyInstance의 PhysX 구현부
 *
 * PIMPL 패턴의 구현 클래스.
 * PhysX RigidActor 관련 모든 객체와 로직을 담당합니다.
 */
class FBodyInstanceImpl
{
public:
    FBodyInstanceImpl() = default;
    ~FBodyInstanceImpl() = default;

    // ═══════════════════════════════════════════════════════════════════════
    // PhysX 액터
    // ═══════════════════════════════════════════════════════════════════════

    /** PhysX 리지드 액터 (Static 또는 Dynamic) */
    physx::PxRigidActor* RigidActorSync = nullptr;

    // ═══════════════════════════════════════════════════════════════════════
    // 렌더 보간용 Transform 이력
    // ═══════════════════════════════════════════════════════════════════════

    /** 이전 물리 스텝의 Transform (PhysX 좌표계) */
    physx::PxTransform PreviousPhysicsTransform = physx::PxTransform(physx::PxIdentity);

    /** 현재 물리 스텝의 Transform (PhysX 좌표계) */
    physx::PxTransform CurrentPhysicsTransform = physx::PxTransform(physx::PxIdentity);

    /** 렌더 보간이 유효한지 (최소 1회 물리 스텝 실행 후 true) */
    bool bRenderInterpolationValid = false;

    // ═══════════════════════════════════════════════════════════════════════
    // 헬퍼 함수
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * @brief Dynamic Actor로 캐스팅
     * @return PxRigidDynamic 포인터 (Dynamic이 아니면 nullptr)
     */
    physx::PxRigidDynamic* GetPxRigidDynamic() const;
};
