# Physics Event Callback 구현 계획

> **담당**: 팀원 A (PhysX SDK 통합)
> **목표**: Unreal Engine 스타일의 물리 이벤트 콜백 시스템 구현
> **작성일**: 2024-11-29

---

## 개요

PhysX 시뮬레이션 이벤트(충돌, 트리거, 수면/각성 등)를 게임 레벨의 델리게이트(OnHit, OnBeginOverlap, OnEndOverlap)로 전달하는 시스템 구현.

### 아키텍처 개요

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        고수준 (Game Layer)                               │
├─────────────────────────────────────────────────────────────────────────┤
│  AActor                                                                  │
│    └─ UPrimitiveComponent                                               │
│         ├─ OnComponentHit (델리게이트)                                  │
│         ├─ OnComponentBeginOverlap (델리게이트)                         │
│         └─ OnComponentEndOverlap (델리게이트)                           │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      중간 계층 (Physics Wrapper)                         │
├─────────────────────────────────────────────────────────────────────────┤
│  FBodyInstance (신규)                                                    │
│    ├─ PxRigidActor* RigidActor                                          │
│    ├─ UPrimitiveComponent* OwnerComponent                               │
│    └─ userData로 PhysX Actor에 연결                                     │
│                                                                          │
│  FHitResult (신규)                                                       │
│    ├─ 충돌 위치, 노멀, 침투 깊이                                        │
│    └─ 충돌한 Actor/Component 정보                                       │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      저수준 (PhysX Layer)                                │
├─────────────────────────────────────────────────────────────────────────┤
│  FPhysicsEventCallback : PxSimulationEventCallback                      │
│    ├─ onContact() → OnHit 이벤트 생성                                   │
│    ├─ onTrigger() → OnBeginOverlap/OnEndOverlap 이벤트 생성            │
│    ├─ onWake() / onSleep() → 수면/각성 이벤트                          │
│    └─ onConstraintBreak() → 조인트 파손 이벤트                         │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 작업 목록

### 작업 1: FHitResult 구조체 생성 (우선순위 1)

**생성 파일**: `Source/Runtime/Engine/Physics/HitResult.h`

충돌 정보를 담는 구조체. Unreal Engine의 FHitResult를 참고하여 필수 필드만 구현.

```cpp
#pragma once

#include "Vector.h"

class AActor;
class UPrimitiveComponent;

/**
 * @brief 충돌 결과 정보
 *
 * PhysX의 충돌 정보를 게임 레이어에서 사용할 수 있는 형태로 변환한 구조체.
 */
struct FHitResult
{
    // ═══════════════════════════════════════════════════════════════════════
    // 충돌 위치 정보 (Mundi 좌표계)
    // ═══════════════════════════════════════════════════════════════════════

    /** 충돌 발생 여부 */
    bool bBlockingHit = false;

    /** 시작점에서 이미 겹쳐 있었는지 */
    bool bStartPenetrating = false;

    /** 충돌 지점 (월드 좌표) */
    FVector ImpactPoint = FVector::Zero();

    /** 충돌 표면의 노멀 벡터 */
    FVector ImpactNormal = FVector::Zero();

    /** 충돌 지점 (트레이스 표면 기준) */
    FVector Location = FVector::Zero();

    /** 충돌 표면의 노멀 (트레이스 방향 기준) */
    FVector Normal = FVector::Zero();

    /** 침투 깊이 (겹침 시) */
    float PenetrationDepth = 0.0f;

    /** 트레이스 시작점부터 충돌점까지의 거리 비율 (0~1) */
    float Time = 1.0f;

    /** 트레이스 시작점부터 충돌점까지의 실제 거리 */
    float Distance = 0.0f;

    // ═══════════════════════════════════════════════════════════════════════
    // 충돌 대상 정보
    // ═══════════════════════════════════════════════════════════════════════

    /** 충돌한 Actor */
    AActor* HitActor = nullptr;

    /** 충돌한 Component */
    UPrimitiveComponent* HitComponent = nullptr;

    /** 충돌한 본(Bone) 이름 (스켈레탈 메시용) */
    FString BoneName;

    /** 충돌한 머티리얼 인덱스 */
    int32 FaceIndex = -1;

    // ═══════════════════════════════════════════════════════════════════════
    // 유틸리티
    // ═══════════════════════════════════════════════════════════════════════

    /** 결과 초기화 */
    void Reset()
    {
        bBlockingHit = false;
        bStartPenetrating = false;
        ImpactPoint = FVector::Zero();
        ImpactNormal = FVector::Zero();
        Location = FVector::Zero();
        Normal = FVector::Zero();
        PenetrationDepth = 0.0f;
        Time = 1.0f;
        Distance = 0.0f;
        HitActor = nullptr;
        HitComponent = nullptr;
        BoneName.clear();
        FaceIndex = -1;
    }

    /** 유효한 충돌인지 확인 */
    bool IsValidBlockingHit() const
    {
        return bBlockingHit && HitComponent != nullptr;
    }
};

/**
 * @brief 오버랩 결과 정보
 */
struct FOverlapResult
{
    /** 오버랩한 Actor */
    AActor* Actor = nullptr;

    /** 오버랩한 Component */
    UPrimitiveComponent* Component = nullptr;

    /** 본 인덱스 (스켈레탈 메시용) */
    int32 ItemIndex = -1;

    /** 유효한 오버랩인지 확인 */
    bool IsValid() const
    {
        return Actor != nullptr && Component != nullptr;
    }
};
```

---

### 작업 2: FBodyInstance 구조체 생성 (우선순위 2)

**생성 파일**: `Source/Runtime/Engine/Physics/BodyInstance.h`

PhysX Actor와 게임 Component를 연결하는 브릿지 역할.

```cpp
#pragma once

#include "Vector.h"

// 전방 선언 (PIMPL 패턴 유지)
namespace physx
{
    class PxRigidActor;
    class PxShape;
}

class UPrimitiveComponent;
class FPhysScene;

/**
 * @brief 물리 바디 인스턴스
 *
 * PhysX Actor와 게임 Component 사이의 브릿지.
 * PhysX Actor의 userData에 이 포인터를 저장하여 콜백에서 접근.
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
    // 물리 파라미터
    // ═══════════════════════════════════════════════════════════════════════

    /** 물리 시뮬레이션 활성화 여부 */
    bool bSimulatePhysics = false;

    /** 중력 적용 여부 */
    bool bEnableGravity = true;

    /** 질량 (kg) */
    float MassInKg = 1.0f;

    /** 선형 감쇠 */
    float LinearDamping = 0.01f;

    /** 각속도 감쇠 */
    float AngularDamping = 0.05f;

    // ═══════════════════════════════════════════════════════════════════════
    // 충돌 설정
    // ═══════════════════════════════════════════════════════════════════════

    /** 충돌 이벤트 생성 여부 */
    bool bNotifyRigidBodyCollision = false;

    /** 트리거로 동작 (물리적 충돌 없이 겹침만 감지) */
    bool bIsTrigger = false;

    // ═══════════════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════════════

    /** 바디 초기화 */
    void InitBody(const FTransform& Transform, UPrimitiveComponent* Owner, FPhysScene* Scene);

    /** 바디 정리 */
    void TermBody();

    // ═══════════════════════════════════════════════════════════════════════
    // 상태 조회/설정
    // ═══════════════════════════════════════════════════════════════════════

    /** 월드 Transform 반환 (Mundi 좌표계) */
    FTransform GetWorldTransform() const;

    /** 월드 Transform 설정 (Mundi 좌표계) */
    void SetWorldTransform(const FTransform& NewTransform, bool bTeleport = false);

    /** 물리 시뮬레이션 활성화/비활성화 */
    void SetSimulatePhysics(bool bSimulate);

    // ═══════════════════════════════════════════════════════════════════════
    // 힘/임펄스 적용
    // ═══════════════════════════════════════════════════════════════════════

    /** 힘 적용 (지속적) */
    void AddForce(const FVector& Force, bool bAccelChange = false);

    /** 임펄스 적용 (순간적) */
    void AddImpulse(const FVector& Impulse, bool bVelChange = false);

    /** 토크 적용 */
    void AddTorqueInRadians(const FVector& Torque, bool bAccelChange = false);

    // ═══════════════════════════════════════════════════════════════════════
    // 속도
    // ═══════════════════════════════════════════════════════════════════════

    /** 선형 속도 반환 */
    FVector GetLinearVelocity() const;

    /** 선형 속도 설정 */
    void SetLinearVelocity(const FVector& Velocity, bool bAddToCurrent = false);

    /** 각속도 반환 */
    FVector GetAngularVelocity() const;

    /** 각속도 설정 */
    void SetAngularVelocity(const FVector& Velocity, bool bAddToCurrent = false);
};
```

---

### 작업 3: FPhysicsEventCallback 클래스 생성 (우선순위 3)

**생성 파일**:
- `Source/Runtime/Engine/Physics/PhysicsEventCallback.h`
- `Source/Runtime/Engine/Physics/PhysicsEventCallback.cpp`

PhysX의 `PxSimulationEventCallback` 인터페이스를 구현.

#### PhysicsEventCallback.h

```cpp
#pragma once

#include <PxPhysicsAPI.h>

class FPhysScene;

/**
 * @brief PhysX 시뮬레이션 이벤트 콜백
 *
 * PhysX에서 발생하는 충돌, 트리거, 수면/각성 이벤트를 처리하여
 * 게임 레이어의 델리게이트로 전달.
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
     * OnHit 델리게이트로 전달.
     */
    virtual void onContact(
        const physx::PxContactPairHeader& PairHeader,
        const physx::PxContactPair* Pairs,
        physx::PxU32 NbPairs) override;

    /**
     * @brief 트리거(Trigger) 이벤트 처리
     *
     * 트리거 볼륨에 진입/이탈했을 때 호출.
     * OnBeginOverlap/OnEndOverlap 델리게이트로 전달.
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
     * @brief Actor 각성 이벤트 처리
     */
    virtual void onWake(
        physx::PxActor** Actors,
        physx::PxU32 Count) override;

    /**
     * @brief Actor 수면 이벤트 처리
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
        struct FHitResult& OutHitResult);
};
```

#### PhysicsEventCallback.cpp

```cpp
#include "pch.h"
#include "PhysicsEventCallback.h"
#include "PhysScene.h"
#include "BodyInstance.h"
#include "HitResult.h"
#include "PhysicsTypes.h"
#include "PrimitiveComponent.h"
#include "Actor.h"
#include "GlobalConsole.h"

using namespace physx;

FPhysicsEventCallback::FPhysicsEventCallback(FPhysScene* InOwnerScene)
    : OwnerScene(InOwnerScene)
{
}

void FPhysicsEventCallback::onContact(
    const PxContactPairHeader& PairHeader,
    const PxContactPair* Pairs,
    PxU32 NbPairs)
{
    // 삭제된 Actor는 무시
    if (PairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_0 ||
        PairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_1)
    {
        return;
    }

    // userData에서 FBodyInstance 추출
    FBodyInstance* BodyInst0 = static_cast<FBodyInstance*>(PairHeader.actors[0]->userData);
    FBodyInstance* BodyInst1 = static_cast<FBodyInstance*>(PairHeader.actors[1]->userData);

    if (!BodyInst0 || !BodyInst1)
    {
        return;
    }

    UPrimitiveComponent* Comp0 = BodyInst0->OwnerComponent;
    UPrimitiveComponent* Comp1 = BodyInst1->OwnerComponent;

    if (!Comp0 || !Comp1)
    {
        return;
    }

    // 각 ContactPair 처리
    for (PxU32 i = 0; i < NbPairs; ++i)
    {
        const PxContactPair& ContactPair = Pairs[i];

        // 충돌 시작/유지/종료 확인
        bool bNewContact = ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND;
        bool bContactPersist = ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
        bool bContactLost = ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_LOST;

        if (bNewContact || bContactPersist)
        {
            // Hit 이벤트 생성
            FHitResult HitResult0, HitResult1;
            ExtractHitResult(ContactPair, PairHeader, true, HitResult0);
            ExtractHitResult(ContactPair, PairHeader, false, HitResult1);

            // 임펄스 계산
            PxVec3 TotalImpulse(0);
            if (ContactPair.contactCount > 0)
            {
                PxContactPairPoint ContactPoints[64];
                PxU32 ContactCount = ContactPair.extractContacts(ContactPoints, 64);
                for (PxU32 j = 0; j < ContactCount; ++j)
                {
                    TotalImpulse += ContactPoints[j].impulse;
                }
            }
            FVector NormalImpulse = PhysicsConversion::ToFVector(TotalImpulse);

            // Comp0에 OnHit 이벤트 전달
            HitResult0.HitActor = Comp1->GetOwner();
            HitResult0.HitComponent = Comp1;
            Comp0->OnComponentHit.Broadcast(Comp0, Comp1->GetOwner(), Comp1, NormalImpulse, HitResult0);

            // Comp1에 OnHit 이벤트 전달
            HitResult1.HitActor = Comp0->GetOwner();
            HitResult1.HitComponent = Comp0;
            Comp1->OnComponentHit.Broadcast(Comp1, Comp0->GetOwner(), Comp0, -NormalImpulse, HitResult1);
        }
    }
}

void FPhysicsEventCallback::onTrigger(PxTriggerPair* Pairs, PxU32 Count)
{
    for (PxU32 i = 0; i < Count; ++i)
    {
        const PxTriggerPair& TriggerPair = Pairs[i];

        // 삭제된 Shape는 무시
        if (TriggerPair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER |
                                  PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
        {
            continue;
        }

        // userData에서 FBodyInstance 추출
        FBodyInstance* TriggerBody = static_cast<FBodyInstance*>(TriggerPair.triggerActor->userData);
        FBodyInstance* OtherBody = static_cast<FBodyInstance*>(TriggerPair.otherActor->userData);

        if (!TriggerBody || !OtherBody)
        {
            continue;
        }

        UPrimitiveComponent* TriggerComp = TriggerBody->OwnerComponent;
        UPrimitiveComponent* OtherComp = OtherBody->OwnerComponent;

        if (!TriggerComp || !OtherComp)
        {
            continue;
        }

        // 트리거 진입/이탈 확인
        if (TriggerPair.status == PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            // BeginOverlap 이벤트
            TriggerComp->OnComponentBeginOverlap.Broadcast(
                TriggerComp, OtherComp->GetOwner(), OtherComp, 0, false, FHitResult());

            OtherComp->OnComponentBeginOverlap.Broadcast(
                OtherComp, TriggerComp->GetOwner(), TriggerComp, 0, false, FHitResult());
        }
        else if (TriggerPair.status == PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            // EndOverlap 이벤트
            TriggerComp->OnComponentEndOverlap.Broadcast(
                TriggerComp, OtherComp->GetOwner(), OtherComp, 0);

            OtherComp->OnComponentEndOverlap.Broadcast(
                OtherComp, TriggerComp->GetOwner(), TriggerComp, 0);
        }
    }
}

void FPhysicsEventCallback::onConstraintBreak(PxConstraintInfo* Constraints, PxU32 Count)
{
    // 조인트 파손 이벤트 (필요시 구현)
    for (PxU32 i = 0; i < Count; ++i)
    {
        UE_LOG("Physics: Constraint broken");
    }
}

void FPhysicsEventCallback::onWake(PxActor** Actors, PxU32 Count)
{
    // 수면에서 깨어난 Actor 처리 (필요시 구현)
}

void FPhysicsEventCallback::onSleep(PxActor** Actors, PxU32 Count)
{
    // 수면 상태로 전환된 Actor 처리 (필요시 구현)
}

void FPhysicsEventCallback::onAdvance(
    const PxRigidBody* const* BodyBuffer,
    const PxTransform* PoseBuffer,
    const PxU32 Count)
{
    // CCD(Continuous Collision Detection) 고급 이벤트 (필요시 구현)
}

void FPhysicsEventCallback::ExtractHitResult(
    const PxContactPair& ContactPair,
    const PxContactPairHeader& PairHeader,
    bool bIsFirstBody,
    FHitResult& OutHitResult)
{
    OutHitResult.Reset();
    OutHitResult.bBlockingHit = true;

    if (ContactPair.contactCount > 0)
    {
        PxContactPairPoint ContactPoints[64];
        PxU32 ContactCount = ContactPair.extractContacts(ContactPoints, 64);

        if (ContactCount > 0)
        {
            const PxContactPairPoint& FirstContact = ContactPoints[0];

            // PhysX 좌표를 Mundi 좌표로 변환
            OutHitResult.ImpactPoint = PhysicsConversion::ToFVector(FirstContact.position);
            OutHitResult.ImpactNormal = PhysicsConversion::ToFVector(
                bIsFirstBody ? FirstContact.normal : -FirstContact.normal);
            OutHitResult.Location = OutHitResult.ImpactPoint;
            OutHitResult.Normal = OutHitResult.ImpactNormal;
            OutHitResult.PenetrationDepth = FirstContact.separation < 0 ? -FirstContact.separation : 0;
            OutHitResult.bStartPenetrating = FirstContact.separation < 0;
        }
    }
}
```

---

### 작업 4: UPrimitiveComponent에 델리게이트 추가 (우선순위 4)

**수정 파일**: `Source/Runtime/Engine/Components/PrimitiveComponent.h`

충돌/오버랩 이벤트를 위한 델리게이트 추가.

```cpp
// PrimitiveComponent.h에 추가할 내용

#include "Delegate.h"  // 델리게이트 시스템 (구현 필요시)

// ═══════════════════════════════════════════════════════════════════════
// 델리게이트 타입 정의
// ═══════════════════════════════════════════════════════════════════════

/**
 * @brief Hit 이벤트 델리게이트
 *
 * @param HitComponent 충돌한 자신의 Component
 * @param OtherActor 충돌한 상대 Actor
 * @param OtherComp 충돌한 상대 Component
 * @param NormalImpulse 충돌 임펄스
 * @param Hit 충돌 상세 정보
 */
DECLARE_MULTICAST_DELEGATE_FiveParams(FComponentHitSignature,
    UPrimitiveComponent*, // HitComponent
    AActor*,              // OtherActor
    UPrimitiveComponent*, // OtherComp
    FVector,              // NormalImpulse
    const FHitResult&     // Hit
);

/**
 * @brief BeginOverlap 이벤트 델리게이트
 */
DECLARE_MULTICAST_DELEGATE_SixParams(FComponentBeginOverlapSignature,
    UPrimitiveComponent*, // OverlappedComponent
    AActor*,              // OtherActor
    UPrimitiveComponent*, // OtherComp
    int32,                // OtherBodyIndex
    bool,                 // bFromSweep
    const FHitResult&     // SweepResult
);

/**
 * @brief EndOverlap 이벤트 델리게이트
 */
DECLARE_MULTICAST_DELEGATE_FourParams(FComponentEndOverlapSignature,
    UPrimitiveComponent*, // OverlappedComponent
    AActor*,              // OtherActor
    UPrimitiveComponent*, // OtherComp
    int32                 // OtherBodyIndex
);

// UPrimitiveComponent 클래스 내부에 추가
class UPrimitiveComponent : public USceneComponent
{
public:
    // ... 기존 코드 ...

    // ═══════════════════════════════════════════════════════════════════════
    // 물리 이벤트 델리게이트
    // ═══════════════════════════════════════════════════════════════════════

    /** Hit 이벤트 (물리적 충돌 시) */
    FComponentHitSignature OnComponentHit;

    /** BeginOverlap 이벤트 (트리거 진입 시) */
    FComponentBeginOverlapSignature OnComponentBeginOverlap;

    /** EndOverlap 이벤트 (트리거 이탈 시) */
    FComponentEndOverlapSignature OnComponentEndOverlap;

    // ═══════════════════════════════════════════════════════════════════════
    // 충돌 설정
    // ═══════════════════════════════════════════════════════════════════════

    /** Hit 이벤트 생성 활성화 */
    UPROPERTY(EditAnywhere, Category="Collision")
    bool bNotifyRigidBodyCollision = false;

    /** 트리거로 동작 (물리적 충돌 없음) */
    UPROPERTY(EditAnywhere, Category="Collision")
    bool bIsTrigger = false;
};
```

---

### 작업 5: FPhysSceneImpl에 EventCallback 연동 (우선순위 5)

**수정 파일**:
- `Source/Runtime/Engine/Physics/PhysSceneImpl.h`
- `Source/Runtime/Engine/Physics/PhysScene.cpp`

Scene 생성 시 EventCallback 등록.

#### PhysSceneImpl.h 수정

```cpp
// private 멤버에 추가
class FPhysicsEventCallback;

class FPhysSceneImpl
{
    // ... 기존 코드 ...

private:
    // PhysX 객체
    physx::PxScene* PScene = nullptr;
    physx::PxDefaultCpuDispatcher* CpuDispatcher = nullptr;
    physx::PxMaterial* DefaultMaterial = nullptr;

    // 이벤트 콜백 (신규)
    FPhysicsEventCallback* EventCallback = nullptr;
};
```

#### PhysScene.cpp 수정 - CreateScene()

```cpp
bool FPhysSceneImpl::CreateScene(UWorld* InOwningWorld)
{
    using namespace physx;

    FPhysicsCore& Core = FPhysicsCore::Get();
    PxPhysics* Physics = Core.GetPhysics();

    // Scene 설정
    PxSceneDesc SceneDesc(Physics->getTolerancesScale());

    // 중력 설정 (PhysX Y-Up, 9.81 m/s²)
    SceneDesc.gravity = PxVec3(0, -9.81f, 0);

    // CPU Dispatcher
    CpuDispatcher = PxDefaultCpuDispatcherCreate(NumPhysxThreads);
    SceneDesc.cpuDispatcher = CpuDispatcher;

    // 필터 셰이더 (기본값 또는 커스텀)
    SceneDesc.filterShader = PxDefaultSimulationFilterShader;

    // 이벤트 콜백 등록 (신규)
    EventCallback = new FPhysicsEventCallback(/* FPhysScene 포인터 필요 */);
    SceneDesc.simulationEventCallback = EventCallback;

    // Scene 플래그
    SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    SceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    SceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

    // Scene 생성
    PScene = Physics->createScene(SceneDesc);

    // PVD 연결 (디버그용)
    if (PScene && Core.GetPvd())
    {
        PxPvdSceneClient* PvdClient = PScene->getScenePvdClient();
        if (PvdClient)
        {
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    return PScene != nullptr;
}
```

#### PhysScene.cpp 수정 - Terminate()

```cpp
void FPhysSceneImpl::Terminate()
{
    if (PScene)
    {
        PScene->release();
        PScene = nullptr;
    }

    if (CpuDispatcher)
    {
        CpuDispatcher->release();
        CpuDispatcher = nullptr;
    }

    // 이벤트 콜백 정리 (신규)
    if (EventCallback)
    {
        delete EventCallback;
        EventCallback = nullptr;
    }

    if (DefaultMaterial)
    {
        DefaultMaterial->release();
        DefaultMaterial = nullptr;
    }

    bInitialized = false;
}
```

---

### 작업 6: 커스텀 Filter Shader 구현 (우선순위 6)

**생성 파일**: `Source/Runtime/Engine/Physics/PhysicsFiltering.h`

충돌 필터링 로직 구현. 어떤 객체가 어떤 객체와 충돌/오버랩할지 결정.

```cpp
#pragma once

#include <PxPhysicsAPI.h>

/**
 * @brief 커스텀 충돌 필터 셰이더
 *
 * PhysX 시뮬레이션에서 두 Shape가 충돌할지 결정.
 * 반환값에 따라 충돌 동작이 결정됨.
 */
inline physx::PxFilterFlags MundiSimulationFilterShader(
    physx::PxFilterObjectAttributes Attributes0,
    physx::PxFilterData FilterData0,
    physx::PxFilterObjectAttributes Attributes1,
    physx::PxFilterData FilterData1,
    physx::PxPairFlags& PairFlags,
    const void* ConstantBlock,
    physx::PxU32 ConstantBlockSize)
{
    using namespace physx;

    // 트리거 처리
    if (PxFilterObjectIsTrigger(Attributes0) || PxFilterObjectIsTrigger(Attributes1))
    {
        PairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }

    // 기본 충돌 플래그
    PairFlags = PxPairFlag::eCONTACT_DEFAULT;

    // 충돌 이벤트 알림 활성화
    // FilterData.word0: 객체 타입
    // FilterData.word1: 충돌 마스크
    // FilterData.word2: 이벤트 플래그 (1 = 이벤트 생성)

    bool bNotify0 = (FilterData0.word2 & 1) != 0;
    bool bNotify1 = (FilterData1.word2 & 1) != 0;

    if (bNotify0 || bNotify1)
    {
        PairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        PairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
        PairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
    }

    // 채널 기반 필터링 (FilterData.word0 & word1로 마스킹)
    if ((FilterData0.word0 & FilterData1.word1) == 0 &&
        (FilterData1.word0 & FilterData0.word1) == 0)
    {
        return PxFilterFlag::eSUPPRESS;  // 충돌 무시
    }

    return PxFilterFlag::eDEFAULT;
}
```

---

## 파일 생성/수정 요약

### 생성할 파일 (4개)

| 파일 | 설명 |
|------|------|
| `Physics/HitResult.h` | 충돌 결과 구조체 |
| `Physics/BodyInstance.h` | 물리 바디 인스턴스 |
| `Physics/PhysicsEventCallback.h` | 이벤트 콜백 헤더 |
| `Physics/PhysicsEventCallback.cpp` | 이벤트 콜백 구현 |

### 수정할 파일 (4개)

| 파일 | 수정 내용 |
|------|----------|
| `PhysSceneImpl.h` | EventCallback 멤버 추가 |
| `PhysScene.cpp` | EventCallback 생성/등록/해제 |
| `PrimitiveComponent.h` | 델리게이트 추가 |
| `Mundi.vcxproj` | 새 파일 추가 |

---

## 이벤트 흐름

```
┌─────────────────────────────────────────────────────────────────────────┐
│  1. PhysX 시뮬레이션                                                     │
│     FPhysSceneImpl::Simulate(DeltaTime)                                 │
│     └─ PxScene->simulate(DeltaTime)                                     │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  2. 결과 Fetch (충돌 감지)                                               │
│     FPhysSceneImpl::FetchResults()                                      │
│     └─ PxScene->fetchResults(true)                                      │
│         └─ FPhysicsEventCallback::onContact() 호출                      │
│         └─ FPhysicsEventCallback::onTrigger() 호출                      │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  3. userData에서 FBodyInstance 추출                                      │
│     PxRigidActor->userData → FBodyInstance*                             │
│     FBodyInstance->OwnerComponent → UPrimitiveComponent*                │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  4. FHitResult 생성 (좌표계 변환)                                        │
│     PhysX 충돌 정보 → Mundi 좌표계로 변환                               │
│     PxVec3 → FVector (PhysicsConversion::ToFVector)                     │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  5. 델리게이트 브로드캐스트                                              │
│     UPrimitiveComponent::OnComponentHit.Broadcast(...)                  │
│     UPrimitiveComponent::OnComponentBeginOverlap.Broadcast(...)         │
│     UPrimitiveComponent::OnComponentEndOverlap.Broadcast(...)           │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│  6. 게임 로직 실행                                                       │
│     등록된 콜백 함수들 실행                                              │
│     예: 데미지 처리, 사운드 재생, 파티클 생성 등                        │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 테스트 계획

### 테스트 1: Hit 이벤트 테스트

```cpp
// 테스트 코드 예시
void TestHitEvent()
{
    // 1. 두 개의 동적 박스 생성
    // 2. 박스 A에 OnComponentHit 콜백 등록
    // 3. 박스 B를 박스 A 방향으로 발사
    // 4. 충돌 시 콜백 호출 확인
    // 5. FHitResult의 ImpactPoint, ImpactNormal 검증
}
```

### 테스트 2: Overlap 이벤트 테스트

```cpp
// 테스트 코드 예시
void TestOverlapEvent()
{
    // 1. 트리거 박스 생성 (bIsTrigger = true)
    // 2. 동적 구체 생성
    // 3. OnComponentBeginOverlap, OnComponentEndOverlap 콜백 등록
    // 4. 구체가 트리거를 통과하도록 설정
    // 5. 진입/이탈 시 콜백 호출 확인
}
```

### 테스트 3: 좌표계 변환 검증

```
PVD에서 확인할 항목:
- 충돌 위치가 올바른 좌표계로 변환되는지
- 충돌 노멀이 올바른 방향인지
- 임펄스 방향이 올바른지
```

---

## 의존성

```
FBodyInstance (이 작업)
    │
    ├─── HitResult.h (충돌 결과)
    │
    ├─── PhysicsEventCallback (이벤트 처리)
    │
    └─── UPrimitiveComponent (델리게이트)
         │
         ├─── UBoxComponent
         ├─── USphereComponent
         └─── UCapsuleComponent
```

---

## 참고 자료

- [PhysX 4.1 Guide - Simulation](https://nvidiagameworks.github.io/PhysX/4.1/documentation/physxguide/Manual/RigidBodyDynamics.html)
- [PxSimulationEventCallback](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/apireference/files/classPxSimulationEventCallback.html)
- Unreal Engine: `Engine/Source/Runtime/Engine/Classes/PhysicsEngine/BodyInstance.h`
- Unreal Engine: `Engine/Source/Runtime/Engine/Private/PhysicsEngine/PhysicsReplication.cpp`
