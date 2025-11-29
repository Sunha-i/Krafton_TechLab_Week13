#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// PhysSceneImpl.h
// FPhysScene의 PhysX 구현부 (PIMPL - 내부 전용)
// ─────────────────────────────────────────────────────────────────────────────
//
// 주의: 이 헤더는 Physics 모듈 내부에서만 사용
//       외부 모듈에서 include 금지
// ─────────────────────────────────────────────────────────────────────────────

#include <PxPhysicsAPI.h>
#include "UEContainer.h"

class UWorld;
class FPhysScene;
class FPhysicsEventCallback;

/**
 * @brief FPhysScene의 PhysX 구현부
 *
 * PIMPL 패턴의 구현 클래스.
 * PhysX 관련 모든 객체와 로직을 담당.
 */
class FPhysSceneImpl
{
public:
    FPhysSceneImpl();
    ~FPhysSceneImpl();

    // ═══════════════════════════════════════════════════════════════════════
    // 초기화/종료
    // ═══════════════════════════════════════════════════════════════════════

    bool Initialize(FPhysScene* InOwnerScene, UWorld* InOwningWorld);
    void Terminate();
    bool IsInitialized() const { return bInitialized; }

    // ═══════════════════════════════════════════════════════════════════════
    // 시뮬레이션
    // ═══════════════════════════════════════════════════════════════════════

    void StartFrame();
    void Simulate(float DeltaSeconds);
    void FetchResults();
    bool IsSimulating() const { return bIsSimulating; }

    // ═══════════════════════════════════════════════════════════════════════
    // 중력 설정
    // ═══════════════════════════════════════════════════════════════════════

    void SetGravity(const physx::PxVec3& InGravity);
    physx::PxVec3 GetGravity() const;

    // ═══════════════════════════════════════════════════════════════════════
    // PhysX 객체 접근 (Physics 모듈 내부용)
    // ═══════════════════════════════════════════════════════════════════════

    physx::PxScene* GetPxScene() const { return PScene; }
    physx::PxMaterial* GetDefaultMaterial() const { return DefaultMaterial; }

    // ═══════════════════════════════════════════════════════════════════════
    // 통계
    // ═══════════════════════════════════════════════════════════════════════

    uint32 GetNumActors(physx::PxActorTypeFlags Types) const;

private:
    bool CreateScene(UWorld* InOwningWorld);
    void CreateTestActors();  // 좌표계 변환 검증용 테스트 함수

    // 헬퍼 함수: 동적 박스 생성
    physx::PxRigidDynamic* CreateDynamicBox(
        physx::PxPhysics* Physics,
        const physx::PxTransform& Pose,
        float HalfExtent,
        const char* DebugName = nullptr);

    // PhysX 객체
    physx::PxScene* PScene = nullptr;
    physx::PxDefaultCpuDispatcher* CpuDispatcher = nullptr;
    physx::PxMaterial* DefaultMaterial = nullptr;
    FPhysicsEventCallback* EventCallback = nullptr;

    // 소유자
    FPhysScene* OwnerScene = nullptr;

    // 상태
    bool bInitialized = false;
    bool bIsSimulating = false;

    // 설정
    static constexpr int32 NumPhysxThreads = 4;
    static constexpr float DefaultStaticFriction = 0.5f;
    static constexpr float DefaultDynamicFriction = 0.5f;
    static constexpr float DefaultRestitution = 0.6f;
};
