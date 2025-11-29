#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// PhysicsEventCallback.h
// PhysX 시뮬레이션 이벤트 콜백 (Phase 1: 로그 출력만)
// ─────────────────────────────────────────────────────────────────────────────
//
// Phase 1: FBodyInstance 없이 이벤트 감지 및 로그 출력
// Phase 2: FBodyInstance 구현 후 델리게이트 브로드캐스트 추가
// ─────────────────────────────────────────────────────────────────────────────

#include <PxPhysicsAPI.h>

class FPhysScene;
struct FHitResult;

/**
 * @brief PhysX 시뮬레이션 이벤트 콜백
 *
 * PhysX에서 발생하는 충돌, 트리거, 수면/각성 이벤트를 처리.
 * Phase 1에서는 이벤트 발생 시 로그 출력만 수행.
 */
class FPhysicsEventCallback : public physx::PxSimulationEventCallback
{
public:
    explicit FPhysicsEventCallback(FPhysScene* InOwnerScene);
    virtual ~FPhysicsEventCallback() = default;

    // ═══════════════════════════════════════════════════════════════════════
    // PxSimulationEventCallback 인터페이스 구현
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * @brief 접촉(Contact) 이벤트 처리
     *
     * 물리적 충돌이 발생했을 때 호출.
     * Phase 1: 충돌 정보 로그 출력
     * Phase 2: OnHit 델리게이트로 전달
     */
    virtual void onContact(
        const physx::PxContactPairHeader& PairHeader,
        const physx::PxContactPair* Pairs,
        physx::PxU32 NbPairs) override;

    /**
     * @brief 트리거(Trigger) 이벤트 처리
     *
     * 트리거 볼륨에 진입/이탈했을 때 호출.
     * Phase 1: 트리거 이벤트 로그 출력
     * Phase 2: OnBeginOverlap/OnEndOverlap 델리게이트로 전달
     */
    virtual void onTrigger(
        physx::PxTriggerPair* Pairs,
        physx::PxU32 Count) override;

    /**
     * @brief 제약(Constraint) 파손 이벤트 처리
     */
    virtual void onConstraintBreak(
        physx::PxConstraintInfo* Constraints,
        physx::PxU32 Count) override;

    /**
     * @brief Actor 활성화 이벤트 처리
     */
    virtual void onWake(
        physx::PxActor** Actors,
        physx::PxU32 Count) override;

    /**
     * @brief Actor 비활성화 이벤트 처리
     */
    virtual void onSleep(
        physx::PxActor** Actors,
        physx::PxU32 Count) override;

    /**
     * @brief 고급 시뮬레이션 이벤트 (CCD용)
     */
    virtual void onAdvance(
        const physx::PxRigidBody* const* BodyBuffer,
        const physx::PxTransform* PoseBuffer,
        const physx::PxU32 Count) override;

private:
    /** 소유 FPhysScene */
    FPhysScene* OwnerScene = nullptr;

    /** Contact 이벤트에서 FHitResult 생성 */
    void ExtractHitResult(
        const physx::PxContactPair& ContactPair,
        const physx::PxContactPairHeader& PairHeader,
        bool bIsFirstBody,
        FHitResult& OutHitResult);

    /** 이벤트 통계 (디버깅용) */
    uint32 TotalContactEvents = 0;
    uint32 TotalTriggerEvents = 0;
};
