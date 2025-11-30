#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// BodyInstance.h
// 물리 바디 인스턴스 (언리얼 스타일)
// ─────────────────────────────────────────────────────────────────────────────
//
// PhysX Actor와 게임 Component 사이의 브릿지.
// PIMPL 패턴을 사용하여 PhysX 의존성을 완전히 숨김.
// 언리얼 엔진의 FBodyInstance와 유사한 구조.
// ─────────────────────────────────────────────────────────────────────────────

#include "Vector.h"
#include <memory>

// 전방 선언 (PhysX 의존성 없음)
class UPrimitiveComponent;
class UBodySetup;
class FPhysScene;
class FBodyInstanceImpl;  // PIMPL

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
 * PIMPL 패턴으로 PhysX 의존성을 숨김.
 *
 * 언리얼 엔진의 FBodyInstance와 유사한 구조.
 */
struct FBodyInstance
{
    // ═══════════════════════════════════════════════════════════════════════
    // 생성자/소멸자
    // ═══════════════════════════════════════════════════════════════════════

    FBodyInstance();
    ~FBodyInstance();

    // 복사 생성자/대입 연산자 (물리 파라미터만 복사, PhysX Actor는 복사 안 함)
    FBodyInstance(const FBodyInstance& Other);
    FBodyInstance& operator=(const FBodyInstance& Other);

    // 이동 허용
    FBodyInstance(FBodyInstance&&) = default;
    FBodyInstance& operator=(FBodyInstance&&) = default;

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
    bool IsInitialized() const;

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
    // 렌더 보간 (고프레임 렌더링 지원)
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * @brief 물리 스텝 후 Transform 캡처
     * @note PhysScene::Simulate()에서 물리 스텝 실행 후 호출됨
     */
    void CapturePhysicsTransform();

    /**
     * @brief Alpha 값으로 보간된 Transform 계산하여 Component에 적용
     * @param Alpha 보간 비율 (0.0 ~ 1.0): 0=이전 Transform, 1=현재 Transform
     * @note PhysScene::Simulate() 종료 시점에 매 프레임 호출됨
     */
    void UpdateRenderInterpolation(float Alpha);

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

    // ═══════════════════════════════════════════════════════════════════════
    // Physics 모듈 내부 접근용 (PIMPL)
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * @brief 내부 구현 접근 (Physics 모듈 내부에서만 사용)
     *
     * 주의: FBodyInstanceImpl은 PhysX 의존성이 있으므로
     *       외부 모듈에서 사용하려면 BodyInstanceImpl.h include 필요
     */
    FBodyInstanceImpl* GetImpl() const;

private:
    /** PIMPL - PhysX 구현 숨김 */
    std::unique_ptr<FBodyInstanceImpl> Impl;

    /** 소유 씬 (Scene에서 제거 시 필요) */
    FPhysScene* OwnerScene = nullptr;
};
