# FBodyInstance 구현 계획 (언리얼 스타일)

> 담당: 팀원 A
> 상태: Phase 2 구현 예정
> 선행 작업: FPhysicsCore, FPhysScene, FPhysicsEventCallback (Phase 1) 완료

---

## 개요

FBodyInstance는 PhysX Actor와 게임 Component를 연결하는 브릿지 역할을 합니다.
**언리얼 엔진 아키텍처를 따라** `UPrimitiveComponent`에 FBodyInstance를 배치하고,
`UBodySetup`을 통해 기하학 데이터를 공유합니다.

---

## 언리얼 vs 현재 계획 비교

| 항목 | 언리얼 엔진 | 이전 계획 | 수정된 계획 |
|------|------------|----------|------------|
| **FBodyInstance 위치** | `UPrimitiveComponent` | `UShapeComponent` | ✅ `UPrimitiveComponent` |
| **UBodySetup** | 있음 (공유 기하학) | 없음 | ✅ 추가 |
| **InitBody 시그니처** | `InitBody(UBodySetup*, ...)` | `InitBody(FTransform, ...)` | ✅ UBodySetup 포함 |
| **Scene 상태 추적** | `ESceneState` enum | 없음 | ✅ 추가 |
| **GetBodySetup()** | 가상 함수 | 없음 | ✅ 추가 |

---

## 아키텍처 (언리얼 스타일)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        고수준 (Game Layer)                               │
├─────────────────────────────────────────────────────────────────────────┤
│  UPrimitiveComponent (기본 클래스)                                       │
│    ├─ FBodyInstance BodyInstance           ← 물리 인스턴스              │
│    ├─ virtual UBodySetup* GetBodySetup()   ← 파생 클래스에서 구현       │
│    ├─ CreatePhysicsState()                 ← BeginPlay에서 호출         │
│    ├─ DestroyPhysicsState()                ← EndPlay에서 호출           │
│    │                                                                     │
│    ├─ UShapeComponent                                                   │
│    │    ├─ UBoxComponent      → GetBodySetup() { return BoxSetup; }    │
│    │    ├─ USphereComponent   → GetBodySetup() { return SphereSetup; } │
│    │    └─ UCapsuleComponent  → GetBodySetup() { return CapsuleSetup; }│
│    │                                                                     │
│    └─ UMeshComponent                                                    │
│         ├─ UStaticMeshComponent → GetBodySetup() from UStaticMesh      │
│         └─ USkeletalMeshComponent → (나중에 구현)                       │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      중간 계층 (Physics Wrapper)                         │
├─────────────────────────────────────────────────────────────────────────┤
│  UBodySetup (공유 데이터) - UObject 파생                                 │
│    ├─ EBodySetupType BodyType              ← Box, Sphere, Capsule 등   │
│    ├─ FVector BoxExtent / float Radius / float HalfHeight              │
│    ├─ UPhysicalMaterial* PhysMaterial      ← 물리 재질 (선택)          │
│    └─ CreatePhysicsShapes(PxRigidActor*)   ← PxShape 생성              │
│                                                                          │
│  FBodyInstance (런타임 인스턴스)                                         │
│    ├─ PxRigidActor* RigidActorSync                                      │
│    ├─ UPrimitiveComponent* OwnerComponent                               │
│    ├─ EBodyInstanceSceneState CurrentSceneState                         │
│    ├─ 물리 파라미터 (Mass, Damping, Gravity 등)                         │
│    └─ InitBody(UBodySetup*, Transform, Owner, Scene)                    │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      저수준 (PhysX Layer)                                │
├─────────────────────────────────────────────────────────────────────────┤
│  FPhysicsEventCallback                                                   │
│    ├─ onContact() → userData에서 FBodyInstance 추출 → 델리게이트 호출   │
│    └─ onTrigger() → userData에서 FBodyInstance 추출 → 델리게이트 호출   │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 작업 1: UBodySetup 클래스 생성

**새 파일**: `Source/Runtime/Engine/Physics/BodySetup.h`

```cpp
#pragma once

#include "Object.h"
#include "Vector.h"

// 전방 선언
namespace physx
{
    class PxRigidActor;
    class PxShape;
    class PxMaterial;
}

class UPhysicalMaterial;

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
class UBodySetup : public UObject
{
public:
    DECLARE_CLASS(UBodySetup, UObject)

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
    // 물리 재질
    // ═══════════════════════════════════════════════════════════════════════

    /** 물리 재질 (nullptr이면 기본 재질 사용) */
    UPhysicalMaterial* PhysMaterial = nullptr;

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
```

---

## 작업 2: FBodyInstance 구조체 생성

**새 파일**: `Source/Runtime/Engine/Physics/BodyInstance.h`

```cpp
#pragma once

#include "Vector.h"
#include "Transform.h"

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
```

---

## 작업 3: UPrimitiveComponent 수정

**수정 파일**: `Source/Runtime/Engine/Components/PrimitiveComponent.h`

```cpp
// 추가할 멤버
#include "BodyInstance.h"

class UBodySetup;

class UPrimitiveComponent : public USceneComponent
{
public:
    DECLARE_CLASS(UPrimitiveComponent, USceneComponent)

    // ... 기존 코드 ...

    // ═══════════════════════════════════════════════════════════════════════
    // 물리 시스템 (언리얼 스타일)
    // ═══════════════════════════════════════════════════════════════════════

    /** 물리 바디 인스턴스 */
    FBodyInstance BodyInstance;

    /**
     * @brief BodySetup 반환 (파생 클래스에서 오버라이드)
     *
     * ShapeComponent: 내부 BodySetup 반환
     * StaticMeshComponent: UStaticMesh의 BodySetup 반환
     */
    virtual UBodySetup* GetBodySetup() const { return nullptr; }

    /** 물리 상태 생성 (BeginPlay에서 호출) */
    virtual void CreatePhysicsState();

    /** 물리 상태 파괴 (EndPlay에서 호출) */
    virtual void DestroyPhysicsState();

    /** 물리 시뮬레이션 활성화/비활성화 */
    void SetSimulatePhysics(bool bSimulate);

    /** 트리거 설정 */
    void SetIsTrigger(bool bTrigger);

    /** 물리 상태가 생성되었는지 */
    bool HasValidPhysicsState() const { return BodyInstance.IsInitialized(); }

protected:
    /** BeginPlay 오버라이드 */
    virtual void BeginPlay() override;

    /** EndPlay 오버라이드 */
    virtual void EndPlay() override;
};
```

**수정 파일**: `Source/Runtime/Engine/Components/PrimitiveComponent.cpp`

```cpp
void UPrimitiveComponent::BeginPlay()
{
    Super::BeginPlay();
    CreatePhysicsState();
}

void UPrimitiveComponent::EndPlay()
{
    DestroyPhysicsState();
    Super::EndPlay();
}

void UPrimitiveComponent::CreatePhysicsState()
{
    // BodySetup이 없으면 물리 상태 생성 안 함
    UBodySetup* Setup = GetBodySetup();
    if (!Setup)
        return;

    if (UWorld* World = GetWorld())
    {
        if (FPhysScene* PhysScene = World->GetPhysScene())
        {
            BodyInstance.InitBody(Setup, GetWorldTransform(), this, PhysScene);
        }
    }
}

void UPrimitiveComponent::DestroyPhysicsState()
{
    BodyInstance.TermBody();
}

void UPrimitiveComponent::SetSimulatePhysics(bool bSimulate)
{
    BodyInstance.bSimulatePhysics = bSimulate;
    if (BodyInstance.IsInitialized())
    {
        BodyInstance.SetSimulatePhysics(bSimulate);
    }
}

void UPrimitiveComponent::SetIsTrigger(bool bTrigger)
{
    BodyInstance.bIsTrigger = bTrigger;
    // 이미 생성된 경우 재생성 필요 (또는 Shape 플래그 수정)
}
```

---

## 작업 4: UShapeComponent 수정

**수정 파일**: `Source/Runtime/Engine/Components/ShapeComponent.h`

```cpp
#include "BodySetup.h"

class UShapeComponent : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UShapeComponent, UPrimitiveComponent)

    // ... 기존 코드 ...

    // ═══════════════════════════════════════════════════════════════════════
    // BodySetup (Shape별로 내부에서 관리)
    // ═══════════════════════════════════════════════════════════════════════

    /** 이 Shape의 BodySetup 반환 */
    virtual UBodySetup* GetBodySetup() const override { return ShapeBodySetup; }

protected:
    /** Shape용 BodySetup (각 파생 클래스에서 초기화) */
    UBodySetup* ShapeBodySetup = nullptr;

    /** BodySetup 생성/업데이트 (Shape 크기 변경 시 호출) */
    virtual void UpdateBodySetup() = 0;
};
```

**UBoxComponent 예시:**

```cpp
class UBoxComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(UBoxComponent, UShapeComponent)

    UBoxComponent();

    /** Box Extent 설정 */
    void SetBoxExtent(const FVector& InExtent);
    FVector GetBoxExtent() const { return BoxExtent; }

protected:
    virtual void UpdateBodySetup() override;

private:
    FVector BoxExtent = FVector(50.0f, 50.0f, 50.0f);
};

// BoxComponent.cpp
UBoxComponent::UBoxComponent()
{
    ShapeBodySetup = ObjectFactory::NewObject<UBodySetup>();
    UpdateBodySetup();
}

void UBoxComponent::SetBoxExtent(const FVector& InExtent)
{
    BoxExtent = InExtent;
    UpdateBodySetup();

    // 물리 상태가 이미 있으면 재생성
    if (HasValidPhysicsState())
    {
        DestroyPhysicsState();
        CreatePhysicsState();
    }
}

void UBoxComponent::UpdateBodySetup()
{
    if (ShapeBodySetup)
    {
        ShapeBodySetup->BodyType = EBodySetupType::Box;
        ShapeBodySetup->BoxExtent = BoxExtent;
    }
}
```

---

## 작업 5: PhysicsEventCallback Phase 2 업데이트

**수정 파일**: `Source/Runtime/Engine/Physics/PhysicsEventCallback.cpp`

```cpp
#include "BodyInstance.h"
#include "PrimitiveComponent.h"
#include "Actor.h"

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

    // 테스트용 Actor는 userData가 없음
    if (!BodyInst0 || !BodyInst1)
    {
        // Phase 1 로그 출력 (기존 코드 유지)
        for (PxU32 i = 0; i < NbPairs; ++i)
        {
            const PxContactPair& ContactPair = Pairs[i];
            bool bNewContact = (ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND);
            if (bNewContact)
            {
                ++TotalContactEvents;
                UE_LOG("[Contact] #%u - Collision (no BodyInstance)", TotalContactEvents);
            }
        }
        return;
    }

    UPrimitiveComponent* Comp0 = BodyInst0->OwnerComponent;
    UPrimitiveComponent* Comp1 = BodyInst1->OwnerComponent;

    if (!Comp0 || !Comp1)
        return;

    // 각 ContactPair 처리
    for (PxU32 i = 0; i < NbPairs; ++i)
    {
        const PxContactPair& ContactPair = Pairs[i];

        bool bNewContact = (ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND);
        bool bContactLost = (ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_LOST);

        if (bNewContact)
        {
            ++TotalContactEvents;

            // HitResult 추출
            FHitResult HitResult;
            ExtractHitResult(ContactPair, PairHeader, true, HitResult);

            // 델리게이트 브로드캐스트
            AActor* Owner0 = Comp0->GetOwner();
            AActor* Owner1 = Comp1->GetOwner();

            if (Owner0)
            {
                Owner0->OnComponentHit.Broadcast(Comp0, Owner1, Comp1, HitResult);
            }
            if (Owner1)
            {
                FHitResult HitResult1;
                ExtractHitResult(ContactPair, PairHeader, false, HitResult1);
                Owner1->OnComponentHit.Broadcast(Comp1, Owner0, Comp0, HitResult1);
            }

            UE_LOG("[Contact] #%u - Hit event broadcast", TotalContactEvents);
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
            ++TotalTriggerEvents;
            UE_LOG("[Trigger] #%u - Trigger event (no BodyInstance)", TotalTriggerEvents);
            continue;
        }

        UPrimitiveComponent* TriggerComp = TriggerBody->OwnerComponent;
        UPrimitiveComponent* OtherComp = OtherBody->OwnerComponent;

        if (!TriggerComp || !OtherComp)
            continue;

        AActor* TriggerOwner = TriggerComp->GetOwner();
        AActor* OtherOwner = OtherComp->GetOwner();

        ++TotalTriggerEvents;

        if (TriggerPair.status == PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            // BeginOverlap 델리게이트
            if (TriggerOwner)
                TriggerOwner->OnComponentBeginOverlap.Broadcast(TriggerComp, OtherOwner, OtherComp);
            if (OtherOwner)
                OtherOwner->OnComponentBeginOverlap.Broadcast(OtherComp, TriggerOwner, TriggerComp);

            UE_LOG("[Trigger] #%u - BeginOverlap broadcast", TotalTriggerEvents);
        }
        else if (TriggerPair.status == PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            // EndOverlap 델리게이트
            if (TriggerOwner)
                TriggerOwner->OnComponentEndOverlap.Broadcast(TriggerComp, OtherOwner, OtherComp);
            if (OtherOwner)
                OtherOwner->OnComponentEndOverlap.Broadcast(OtherComp, TriggerOwner, TriggerComp);

            UE_LOG("[Trigger] #%u - EndOverlap broadcast", TotalTriggerEvents);
        }
    }
}
```

---

## 작업 6: Transform 동기화 (FetchResults)

**수정 파일**: `Source/Runtime/Engine/Physics/PhysScene.cpp`

```cpp
void FPhysSceneImpl::FetchResults()
{
    if (!PScene || !bIsSimulating)
        return;

    PScene->fetchResults(true);
    bIsSimulating = false;

    // Active Actors Transform 동기화
    PxU32 NumActiveActors = 0;
    PxActor** ActiveActors = PScene->getActiveActors(NumActiveActors);

    for (PxU32 i = 0; i < NumActiveActors; ++i)
    {
        if (PxRigidActor* RigidActor = ActiveActors[i]->is<PxRigidActor>())
        {
            FBodyInstance* BodyInst = static_cast<FBodyInstance*>(RigidActor->userData);
            if (BodyInst && BodyInst->IsInScene())
            {
                BodyInst->SyncPhysicsToComponent();
            }
        }
    }
}
```

---

## 파일 생성/수정 요약

| 작업 | 파일 | 설명 |
|------|------|------|
| 신규 | `Physics/BodySetup.h` | UBodySetup 클래스 정의 |
| 신규 | `Physics/BodySetup.cpp` | UBodySetup 구현 (Shape 생성) |
| 신규 | `Physics/BodyInstance.h` | FBodyInstance 구조체 정의 |
| 신규 | `Physics/BodyInstance.cpp` | FBodyInstance 구현 |
| 수정 | `Components/PrimitiveComponent.h` | FBodyInstance, GetBodySetup() 추가 |
| 수정 | `Components/PrimitiveComponent.cpp` | CreatePhysicsState/DestroyPhysicsState |
| 수정 | `Components/ShapeComponent.h` | ShapeBodySetup, UpdateBodySetup() |
| 수정 | `Components/BoxComponent.cpp` | BodySetup 초기화 |
| 수정 | `Components/SphereComponent.cpp` | BodySetup 초기화 |
| 수정 | `Components/CapsuleComponent.cpp` | BodySetup 초기화 |
| 수정 | `Physics/PhysicsEventCallback.cpp` | Phase 2 - 델리게이트 브로드캐스트 |
| 수정 | `Physics/PhysScene.cpp` | FetchResults에서 Transform 동기화 |
| 수정 | `Mundi.vcxproj` | 새 파일 등록 |

---

## 구현 순서 (체크리스트)

1. [ ] `BodySetup.h/.cpp` 생성 - UBodySetup 클래스
2. [ ] `BodyInstance.h/.cpp` 생성 - FBodyInstance 구조체 (언리얼 스타일)
3. [ ] `PrimitiveComponent.h/cpp` 수정 - FBodyInstance 통합, GetBodySetup()
4. [ ] `ShapeComponent.h/cpp` 수정 - ShapeBodySetup, UpdateBodySetup()
5. [ ] `BoxComponent.cpp` 수정 - BodySetup 초기화
6. [ ] `SphereComponent.cpp` 수정 - BodySetup 초기화
7. [ ] `CapsuleComponent.cpp` 수정 - BodySetup 초기화
8. [ ] `PhysicsEventCallback.cpp` 수정 - Phase 2 델리게이트
9. [ ] `PhysScene.cpp` 수정 - Transform 동기화
10. [ ] `Mundi.vcxproj` 수정 - 파일 등록
11. [ ] 빌드 및 테스트

---

## 테스트 계획

### 1. 기본 동작 테스트
- BoxComponent 생성 후 물리 시뮬레이션 활성화
- 중력에 의해 떨어지는지 확인
- PVD에서 Actor 확인

### 2. BodySetup 공유 테스트
- 동일 크기의 Box 여러 개 생성
- 같은 BodySetup을 공유하는지 확인 (메모리 효율)

### 3. 충돌 이벤트 테스트
- 두 Dynamic Box 충돌 시 OnComponentHit 호출 확인
- 로그 또는 Lua 콜백으로 검증

### 4. 트리거 이벤트 테스트
- Trigger Box 생성 (bIsTrigger = true)
- Dynamic Sphere가 통과 시 OnBeginOverlap/OnEndOverlap 호출 확인

### 5. Transform 동기화 테스트
- 물리 시뮬레이션 후 Component Transform이 업데이트되는지 확인
- 렌더링 위치와 물리 위치 일치 확인

---

## 참고 문서

- [UnrealEngine_Physics_Architecture.md](./UnrealEngine_Physics_Architecture.md) - 언리얼 엔진 물리 아키텍처 참고
- [WEEK13_발제.md](./WEEK13_발제.md) - Week 13 발제 요구사항
- [PLAN_PhysicsEventCallback.md](./PLAN_PhysicsEventCallback.md) - Physics Event Callback 구현 계획
