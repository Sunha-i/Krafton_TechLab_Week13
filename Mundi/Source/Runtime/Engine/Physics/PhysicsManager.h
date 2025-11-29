#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// PhysicsManager.h
// PhysX 물리 시뮬레이션 관리자
// ─────────────────────────────────────────────────────────────────────────────

#include "Object.h"
#include "PhysicsTypes.h"
#include <PxPhysicsAPI.h>

// 전방 선언
class UWorld;

/**
 * @brief PhysX 물리 시뮬레이션 관리자
 *
 * PhysX SDK 초기화, Scene 관리, 시뮬레이션 루프를 담당합니다.
 * World당 하나의 PhysicsManager가 존재합니다.
 */
class UPhysicsManager : public UObject
{
public:
    DECLARE_CLASS(UPhysicsManager, UObject)

    UPhysicsManager();
    ~UPhysicsManager() override;

    // ═══════════════════════════════════════════════════════════════════════
    // 생명주기 관리
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * @brief PhysX 초기화
     * @param InWorld 소유 월드
     *
     * Foundation, Physics, Scene, PVD를 초기화합니다.
     */
    void Initialize(UWorld* InWorld);

    /**
     * @brief PhysX 종료 및 리소스 해제
     */
    void Shutdown();

    /**
     * @brief 물리 시뮬레이션 스텝 실행
     * @param DeltaTime 프레임 델타 시간 (초)
     */
    void Simulate(float DeltaTime);

    /**
     * @brief 시뮬레이션 결과 동기화
     * @param bBlock true면 시뮬레이션 완료까지 대기
     */
    void FetchResults(bool bBlock = true);

    // ═══════════════════════════════════════════════════════════════════════
    // PhysX 객체 접근자
    // ═══════════════════════════════════════════════════════════════════════

    physx::PxPhysics* GetPhysics() const { return Physics; }
    physx::PxScene* GetScene() const { return Scene; }
    physx::PxMaterial* GetDefaultMaterial() const { return DefaultMaterial; }
    physx::PxCooking* GetCooking() const { return Cooking; }
    UWorld* GetWorld() const { return World; }

    // ═══════════════════════════════════════════════════════════════════════
    // Material 생성
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * @brief 커스텀 물리 머터리얼 생성
     * @param StaticFriction 정지 마찰 계수
     * @param DynamicFriction 동적 마찰 계수
     * @param Restitution 반발 계수 (0=완전 비탄성, 1=완전 탄성)
     * @return 생성된 PxMaterial (소유권은 Physics가 관리)
     */
    physx::PxMaterial* CreateMaterial(float StaticFriction, float DynamicFriction, float Restitution);

    // ═══════════════════════════════════════════════════════════════════════
    // 디버그 및 상태
    // ═══════════════════════════════════════════════════════════════════════

    bool IsInitialized() const { return bIsInitialized; }
    bool IsSimulating() const { return bIsSimulating; }

    /**
     * @brief 물리 통계 정보 (디버그용)
     */
    struct FPhysicsStats
    {
        uint32 ActiveActorCount = 0;
        uint32 StaticActorCount = 0;
        uint32 DynamicActorCount = 0;
        float SimulationTime = 0.0f;
    };
    FPhysicsStats GetStats() const;

private:
    // ═══════════════════════════════════════════════════════════════════════
    // 내부 초기화 함수
    // ═══════════════════════════════════════════════════════════════════════

    bool CreateFoundation();
    bool CreatePhysics();
    bool CreateScene();
    bool ConnectPVD();
    void DisconnectPVD();

    // ═══════════════════════════════════════════════════════════════════════
    // PhysX 핵심 객체
    // ═══════════════════════════════════════════════════════════════════════

    physx::PxDefaultAllocator Allocator;
    physx::PxDefaultErrorCallback ErrorCallback;

    physx::PxFoundation* Foundation = nullptr;
    physx::PxPhysics* Physics = nullptr;
    physx::PxScene* Scene = nullptr;
    physx::PxCooking* Cooking = nullptr;
    physx::PxMaterial* DefaultMaterial = nullptr;
    physx::PxDefaultCpuDispatcher* Dispatcher = nullptr;

    // PVD (PhysX Visual Debugger)
    physx::PxPvd* Pvd = nullptr;
    physx::PxPvdTransport* PvdTransport = nullptr;

    // ═══════════════════════════════════════════════════════════════════════
    // 상태 변수
    // ═══════════════════════════════════════════════════════════════════════

    UWorld* World = nullptr;
    bool bIsInitialized = false;
    bool bIsSimulating = false;

    // 시뮬레이션 설정
    static constexpr int32 CpuDispatcherThreadCount = 4;
    static constexpr float DefaultStaticFriction = 0.5f;
    static constexpr float DefaultDynamicFriction = 0.5f;
    static constexpr float DefaultRestitution = 0.6f;
};
