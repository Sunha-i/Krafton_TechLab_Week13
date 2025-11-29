#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// BodyInstance.h
// 물리 바디 인스턴스 (언리얼 스타일)
// ─────────────────────────────────────────────────────────────────────────────
//
// PhysX Actor와 게임 Component 사이의 브릿지.
// PxRigidActor->userData에 이 포인터를 저장하여 콜백에서 접근.
// 언리얼 엔진의 FBodyInstance와 유사한 구조.
// ─────────────────────────────────────────────────────────────────────────────

#include "Vector.h"

// 전방 선언 (PIMPL 유지)
namespace physx
{
    class PxRigidActor;
    class PxRigidDynamic;
    class PxRigidStatic;
}

class UPrimitiveComponent;
class UBodySetup;
class FPhysScene;

/**
 * @brief Scene 상태 (언리얼 스타일)
 */
enum class EBodyInstanceSceneState : uint8
{
    NotAdded,       // Scene에 추가되지 않음
    AwaitingAdd,    // 추가 대기 중
    Added,          // Scene에 추가됨
    AwaitingRemove, // 제거 대기 중
    Removed         // Scene에서 제거됨
};

/**
 * @brief 물리 바디 인스턴스 (언리얼 스타일)
 *
 * PhysX Actor와 게임 Component 사이의 브릿지.
 * PxRigidActor->userData에 이 포인터를 저장하여 콜백에서 접근.
 *
 * 언리얼 엔진의 FBodyInstance와 유사한 구조.
 */
struct FBodyInstance
{
    // ═══════════════════════════════════════════════════════════════════════
    // PhysX 액터 (내부용)
    // ═══════════════════════════════════════════════════════════════════════

    /** PhysX 리지드 액터 (Static 또는 Dynamic) */
    physx::PxRigidActor* RigidActorSync = nullptr;

    // ═══════════════════════════════════════════════════════════════════════
    // 소유자 참조
    // ═══════════════════════════════════════════════════════════════════════

    /** 이 바디를 소유한 Component */
    UPrimitiveComponent* OwnerComponent = nullptr;

    // ═══════════════════════════════════════════════════════════════════════
    // Scene 상태 (언리얼 스타일)
    // ═══════════════════════════════════════════════════════════════════════

    /** 현재 Scene 상태 */
    EBodyInstanceSceneState CurrentSceneState = EBodyInstanceSceneState::NotAdded;

    // ═══════════════════════════════════════════════════════════════════════
    // 물리 파라미터
    // ═══════════════════════════════════════════════════════════════════════

    /** 물리 시뮬레이션 활성화 여부 */
    bool bSimulatePhysics = false;

    /** 중력 적용 여부 */
    bool bEnableGravity = true;

    /** 트리거로 동작 (물리적 충돌 없이 겹침만 감지) */
    bool bIsTrigger = false;

    /** 질량 (kg) - 0이면 자동 계산 */
    float MassInKg = 0.0f;

    /** 선형 감쇠 */
    float LinearDamping = 0.01f;

    /** 각속도 감쇠 */
    float AngularDamping = 0.05f;

    // ═══════════════════════════════════════════════════════════════════════
    // 생명주기 (언리얼 스타일 시그니처)
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * @brief 바디 초기화 - PhysX Actor 생성 및 Scene 추가
     * @param Setup 공유 기하학 데이터 (UBodySetup)
     * @param Transform 초기 월드 Transform (Mundi 좌표계)
     * @param Owner 소유 Component
     * @param Scene 물리 Scene
     */
    void InitBody(
        UBodySetup* Setup,
        const FTransform& Transform,
        UPrimitiveComponent* Owner,
        FPhysScene* Scene);

    /** 바디 정리 - PhysX Actor Scene에서 제거 및 해제 */
    void TermBody();

    /** 초기화 여부 */
    bool IsInitialized() const { return RigidActorSync != nullptr; }

    /** Scene에 추가되었는지 */
    bool IsInScene() const { return CurrentSceneState == EBodyInstanceSceneState::Added; }

    // ═══════════════════════════════════════════════════════════════════════
    // Transform 동기화
    // ═══════════════════════════════════════════════════════════════════════

    /** PhysX로부터 Transform 가져오기 (Mundi 좌표계) */
    FTransform GetWorldTransform() const;

    /** PhysX에 Transform 설정 (Mundi 좌표계) */
    void SetWorldTransform(const FTransform& NewTransform, bool bTeleport = false);

    /** Component Transform → PhysX 동기화 */
    void SyncComponentToPhysics();

    /** PhysX Transform → Component 동기화 */
    void SyncPhysicsToComponent();

    // ═══════════════════════════════════════════════════════════════════════
    // 물리 제어
    // ═══════════════════════════════════════════════════════════════════════

    /** 물리 시뮬레이션 활성화/비활성화 */
    void SetSimulatePhysics(bool bSimulate);

    /** 힘 적용 (지속적) */
    void AddForce(const FVector& Force, bool bAccelChange = false);

    /** 임펄스 적용 (순간적) */
    void AddImpulse(const FVector& Impulse, bool bVelChange = false);

    /** 토크 적용 */
    void AddTorqueInRadians(const FVector& Torque, bool bAccelChange = false);

    // ═══════════════════════════════════════════════════════════════════════
    // 속도
    // ═══════════════════════════════════════════════════════════════════════

    /** 선형 속도 반환 (Mundi 좌표계) */
    FVector GetLinearVelocity() const;

    /** 선형 속도 설정 */
    void SetLinearVelocity(const FVector& Velocity, bool bAddToCurrent = false);

    /** 각속도 반환 (Mundi 좌표계) */
    FVector GetAngularVelocityInRadians() const;

    /** 각속도 설정 */
    void SetAngularVelocityInRadians(const FVector& AngVel, bool bAddToCurrent = false);

    // ═══════════════════════════════════════════════════════════════════════
    // 질량/관성
    // ═══════════════════════════════════════════════════════════════════════

    /** 질량 반환 */
    float GetBodyMass() const;

    /** 관성 텐서 반환 */
    FVector GetBodyInertiaTensor() const;

private:
    /** 소유 씬 (Scene에서 제거 시 필요) */
    FPhysScene* OwnerScene = nullptr;

    /** Dynamic Actor로 캐스팅 (시뮬레이션 중일 때만 유효) */
    physx::PxRigidDynamic* GetPxRigidDynamic() const;
};
