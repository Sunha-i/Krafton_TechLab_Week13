# 언리얼 엔진 물리 시뮬레이션 아키텍처 분석

> **목적**: Mundi 엔진의 PhysX 통합 시 참고용 아키텍처 문서
> **분석 대상**: Unreal Engine 5 (C:\UnrealEngine\Engine\Source)
> **작성일**: 2024-11-29

---

## 1. 핵심 클래스 계층 구조

```
┌─────────────────────────────────────────────────────────────────┐
│                     고수준 (Game Layer)                          │
├─────────────────────────────────────────────────────────────────┤
│  AActor                                                          │
│    └─ UPrimitiveComponent (렌더링 + 물리 표현)                   │
│         ├─ FBodyInstance BodyInstance (단일 바디)               │
│         └─ TArray<FBodyInstance> (다중 바디 - 스켈레탈)         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   중간 계층 (Physics Wrapper)                    │
├─────────────────────────────────────────────────────────────────┤
│  FBodyInstance (물리 바디 인스턴스)                              │
│    ├─ FPhysicsActorHandle ActorHandle → 실제 물리 액터          │
│    ├─ UBodySetup* → 공유 기하학 데이터                          │
│    └─ 질량, 댐핑, 충돌 설정 등                                  │
│                                                                  │
│  UBodySetup (공유 충돌 데이터)                                   │
│    ├─ FKAggregateGeom AggGeom (Box, Sphere, Capsule, Convex)   │
│    ├─ CookedFormatData (요리된 메시 데이터)                     │
│    └─ DefaultInstance (기본 물리 설정)                          │
│                                                                  │
│  FPhysScene (물리 씬 래퍼)                                       │
│    ├─ Tick(), StartFrame(), EndFrame()                          │
│    └─ 이벤트 관리, 액터 추가/제거                               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                   저수준 (Physics Engine)                        │
├─────────────────────────────────────────────────────────────────┤
│  FPhysScene_Chaos (또는 PhysX)                                   │
│    ├─ Chaos::FSolverEvolution (실제 시뮬레이터)                 │
│    ├─ 물리 프록시들 (Particle, RigidBody 등)                    │
│    └─ 충돌 감지, 제약 솔버, 적분기                              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. 핵심 클래스 상세

### 2.1 FBodyInstance

**파일**: `Engine/Source/Runtime/Engine/Classes/PhysicsEngine/BodyInstance.h`

게임 바디 하나를 나타내는 런타임 인스턴스. Component와 물리 엔진 사이의 브릿지 역할.

```cpp
struct FBodyInstance
{
    // === 물리 액터 핸들 ===
    FPhysicsActorHandle ActorHandle;  // 실제 Chaos/PhysX 액터

    // === 소유자 참조 ===
    TWeakObjectPtr<UPrimitiveComponent> OwnerComponent;

    // === 물리 파라미터 ===
    float LinearDamping;
    float AngularDamping;
    float MassInKgOverride;
    FVector InertiaTensorScale;

    // === 시뮬레이션 설정 ===
    uint8 bSimulatePhysics : 1;
    uint8 bEnableGravity : 1;
    EDOFMode::Type DOFMode;

    // === 충돌 설정 ===
    ECollisionChannel ObjectType;
    ECollisionEnabled::Type CollisionEnabled;
    FCollisionResponse CollisionResponses;

    // === 상태 추적 ===
    enum class ESceneState { NotAdded, AwaitingAdd, Added, AwaitingRemove, Removed };
    ESceneState CurrentSceneState;

    // === 핵심 함수 ===
    void InitBody(UBodySetup* Setup, const FTransform& Transform,
                  UPrimitiveComponent* Owner, FPhysScene* Scene);
    void TermBody();

    void SetInstanceSimulatePhysics(bool bSimulate);
    void AddForce(const FVector& Force);
    void AddImpulse(const FVector& Impulse);
    void AddTorqueInRadians(const FVector& Torque);

    FTransform GetUnrealWorldTransform() const;
    void SetBodyTransform(const FTransform& NewTransform);

    float GetBodyMass() const;
    FVector GetBodyInertiaTensor() const;
};
```

### 2.2 UBodySetup

**파일**: `Engine/Source/Runtime/Engine/Classes/PhysicsEngine/BodySetup.h`

충돌 기하학 데이터 저장소. 여러 FBodyInstance가 공유하여 메모리 효율성 확보.

```cpp
class UBodySetup : public UObject
{
    // === 기하학 데이터 ===
    FKAggregateGeom AggGeom;  // 단순 도형들 (Box, Sphere, Capsule, Convex)

    // === 요리된 데이터 ===
    FFormatContainer CookedFormatData;  // 플랫폼별 요리된 물리 데이터
    TArray<Chaos::FTriangleMeshImplicitObjectPtr> TriMeshGeometries;

    // === 기본 인스턴스 ===
    FBodyInstance DefaultInstance;  // 이 Setup을 사용하는 인스턴스들의 기본값

    // === 물리 재료 ===
    UPhysicalMaterial* PhysMaterial;

    // === 핵심 함수 ===
    void CreatePhysicsMeshes();
    void CreatePhysicsMeshesAsync(FOnAsyncPhysicsCookFinished Callback);
    bool GetCookedData(const FString& Format, FByteBulkData& OutData);
};
```

### 2.3 FPhysScene

**파일**: `Engine/Source/Runtime/Engine/Public/Physics/PhysScene.h`

물리 씬 관리자. 시뮬레이션 루프, 이벤트, 액터 생명주기 관리.

```cpp
class FPhysScene
{
    // === 시뮬레이션 ===
    void Tick(float DeltaSeconds);
    void StartFrame();
    void EndFrame();

    // === 액터 관리 ===
    void AddActor(FBodyInstance* Body);
    void RemoveActor(FBodyInstance* Body);

    // === 콜백 등록 ===
    void SetKinematicUpdateFunction(TFunction<void()> Func);
    void SetStartFrameFunction(TFunction<void()> Func);
    void SetEndFrameFunction(TFunction<void()> Func);
    void AddForceFunction(TFunction<void()> Func);

    // === 이벤트 ===
    void RegisterForCollisionEvents(UPrimitiveComponent* Component);
    void UnRegisterForCollisionEvents(UPrimitiveComponent* Component);
};
```

---

## 3. 물리 시뮬레이션 루프

```
UWorld::Tick(DeltaSeconds)
│
├─ 1. Pre-Physics (StartFrame)
│     ├─ 키네마틱 타겟 업데이트
│     ├─ 대기 중인 바디 추가/제거 처리
│     └─ StartFrame 콜백 실행
│
├─ 2. Simulate (물리 스레드 - 병렬 가능)
│     ├─ 힘/임펄스 적용
│     ├─ 충돌 감지
│     ├─ 제약 솔버
│     └─ 위치/속도 적분
│
└─ 3. Post-Physics (EndFrame)
      ├─ FetchResults (결과 동기화)
      ├─ Component Transform 업데이트
      └─ 충돌 이벤트 발생 (OnHit, OnOverlap)
```

---

## 4. RigidBody 생성/파괴 흐름

### 4.1 생성 흐름

```cpp
// 1. Component가 물리 상태 생성 요청
UPrimitiveComponent::CreatePhysicsState()
{
    BodyInstance.InitBody(GetBodySetup(), GetComponentTransform(), this, GetWorld()->GetPhysicsScene());
}

// 2. FBodyInstance::InitBody 내부
void FBodyInstance::InitBody(UBodySetup* Setup, const FTransform& Transform, ...)
{
    // 2-1. 물리 액터 생성
    ActorHandle = CreateActor_AssumesLocked(Transform);

    // 2-2. Shape 생성 (Box, Sphere, Capsule 등)
    CreateShapes_AssumesLocked(Setup->AggGeom);

    // 2-3. 질량, 관성 계산
    InitDynamicProperties_AssumesLocked();

    // 2-4. Scene에 추가
    Scene->AddActor(this);

    CurrentSceneState = ESceneState::Added;
}
```

### 4.2 파괴 흐름

```cpp
// 1. Component가 물리 상태 파괴 요청
UPrimitiveComponent::DestroyPhysicsState()
{
    BodyInstance.TermBody();
}

// 2. FBodyInstance::TermBody 내부
void FBodyInstance::TermBody()
{
    // Scene에서 제거 요청 (deferred)
    CurrentSceneState = ESceneState::AwaitingRemove;
    Scene->RemoveActor(this);

    // 제약 정리
    if (DOFConstraint) { /* 제거 */ }
    if (WeldParent) { UnWeld(this); }

    // 핸들 초기화
    ActorHandle.Reset();
}
```

---

## 5. 충돌 필터링 시스템

### 5.1 충돌 채널

```cpp
enum ECollisionChannel
{
    ECC_WorldStatic,        // 정적 월드 기하학
    ECC_WorldDynamic,       // 동적 월드 기하학
    ECC_Pawn,               // 폰/캐릭터
    ECC_Visibility,         // 시각선 채널
    ECC_Camera,             // 카메라
    ECC_PhysicsBody,        // 물리 바디
    ECC_GameTraceChannel1,  // 사용자 정의 (1~18)
    // ...
};
```

### 5.2 충돌 응답

```cpp
enum ECollisionResponse
{
    ECR_Ignore,   // 완전 무시 (쿼리X, 시뮬X)
    ECR_Overlap,  // 겹침만 감지 (쿼리O, 시뮬X)
    ECR_Block,    // 물리적 충돌 (쿼리O, 시뮬O)
};
```

### 5.3 필터 데이터

```cpp
struct FBodyCollisionFilterData
{
    // 시뮬레이션용
    uint32 SimFilter_Word0;  // 객체 타입
    uint32 SimFilter_Word1;  // 블록할 채널
    uint32 SimFilter_Word2;  // 겹칠 채널
    uint32 SimFilter_Word3;  // 무시할 채널

    // 쿼리용
    uint32 QueryFilter_Word0;
    uint32 QueryFilter_Word1;
    uint32 QueryFilter_Word2;
    uint32 QueryFilter_Word3;
};
```

---

## 6. 이벤트 시스템

### 6.1 충돌 이벤트

```cpp
// Hit 이벤트 (물리적 충돌)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FComponentHitSignature,
    UPrimitiveComponent*, HitComponent,
    AActor*, OtherActor,
    UPrimitiveComponent*, OtherComp,
    FVector, NormalImpulse,
    const FHitResult&, Hit);

// Overlap 이벤트 (겹침 감지)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FComponentBeginOverlapSignature,
    UPrimitiveComponent*, OverlappedComponent,
    AActor*, OtherActor,
    UPrimitiveComponent*, OtherComp,
    int32, OtherBodyIndex,
    bool, bFromSweep,
    const FHitResult&, SweepResult);
```

### 6.2 이벤트 발생 타이밍

```
Frame N
├─ Game Thread: Actor Tick
├─ Physics Thread: Simulate
│   └─ 충돌 감지 → 이벤트 큐 작성
└─ Game Thread (Frame N+1)
    └─ 이벤트 처리
        ├─ OnComponentHit 브로드캐스트
        └─ OnComponentBeginOverlap 브로드캐스트
```

---

## 7. 주요 파일 위치

| 클래스/구조체 | 파일 경로 |
|--------------|----------|
| **FBodyInstance** | `Engine/Classes/PhysicsEngine/BodyInstance.h` |
| **UBodySetup** | `Engine/Classes/PhysicsEngine/BodySetup.h` |
| **FPhysScene** | `Engine/Public/Physics/PhysScene.h` |
| **FPhysScene_Chaos** | `Engine/Public/Physics/Experimental/PhysScene_Chaos.h` |
| **FConstraintInstance** | `Engine/Classes/PhysicsEngine/ConstraintInstance.h` |
| **물리 필터링** | `Engine/Public/Physics/PhysicsFiltering.h` |
| **물리 인터페이스** | `Engine/Public/Physics/PhysicsInterfaceTypes.h` |
| **BodyInstance 구현** | `Engine/Private/PhysicsEngine/BodyInstance.cpp` |

---

## 8. Mundi 엔진 적용 가이드

### 8.1 클래스 매핑

| 언리얼 엔진 | Mundi 엔진 | 담당 |
|------------|-----------|------|
| `FPhysScene` | `UPhysicsManager` | 팀원 A (완료) |
| `FBodyInstance` | `FBodyInstance` (신규) | 팀원 B |
| `UBodySetup` | `UBodySetup` (신규) | 팀원 B |
| 좌표계 변환 | `PhysicsTypes.h` | 팀원 A |
| `PxMaterial` 래퍼 | `UPhysicalMaterial` | 팀원 B |

### 8.2 현재 구현 상태

```cpp
// 완료: UPhysicsManager (Source/Runtime/Engine/Physics/PhysicsManager.h)
class UPhysicsManager : public UObject
{
    // PhysX 초기화/종료
    void Initialize(UWorld* InWorld);
    void Shutdown();

    // 시뮬레이션
    void Simulate(float DeltaTime);
    void FetchResults(bool bBlock = true);

    // PhysX 객체 접근
    physx::PxPhysics* GetPhysics() const;
    physx::PxScene* GetScene() const;
    physx::PxMaterial* GetDefaultMaterial() const;
    physx::PxCooking* GetCooking() const;
};
```

### 8.3 다음 단계: FBodyInstance 구현

```cpp
// 제안: Source/Runtime/Engine/Physics/BodyInstance.h
struct FBodyInstance
{
    // PhysX 액터 (Static 또는 Dynamic)
    physx::PxRigidActor* RigidActor = nullptr;

    // 소유자
    class UShapeComponent* OwnerComponent = nullptr;

    // 물리 파라미터
    float Mass = 1.0f;
    float LinearDamping = 0.01f;
    float AngularDamping = 0.05f;
    bool bSimulatePhysics = false;
    bool bEnableGravity = true;

    // 생명주기
    void InitBody(class UBodySetup* Setup, const FTransform& Transform,
                  UShapeComponent* Owner, UPhysicsManager* PhysMgr);
    void TermBody();

    // 물리 제어
    void SetSimulatePhysics(bool bSimulate);
    void AddForce(const FVector& Force);
    void AddImpulse(const FVector& Impulse);

    // 상태 조회/설정
    FTransform GetWorldTransform() const;
    void SetWorldTransform(const FTransform& NewTransform);
};
```

---

## 9. 좌표계 변환 참고

### 9.1 좌표계 차이

| 항목 | Mundi 엔진 | PhysX |
|------|-----------|-------|
| Up 축 | Z | Y |
| Forward 축 | X | Z |
| Right 축 | Y | X |
| Handedness | Left-Handed | Right-Handed |

### 9.2 변환 함수 (PhysicsTypes.h)

```cpp
namespace PhysicsConversion
{
    // 위치 변환: Mundi(X,Y,Z) → PhysX(Y,Z,X)
    inline physx::PxVec3 ToPxVec3(const FVector& V)
    {
        return physx::PxVec3(V.Y, V.Z, V.X);
    }

    inline FVector ToFVector(const physx::PxVec3& V)
    {
        return FVector(V.z, V.x, V.y);
    }

    // 회전 변환: 축 재배치 + Handedness 반전
    inline physx::PxQuat ToPxQuat(const FQuat& Q)
    {
        return physx::PxQuat(-Q.Y, -Q.Z, -Q.X, Q.W);
    }

    inline FQuat ToFQuat(const physx::PxQuat& Q)
    {
        return FQuat(-Q.z, -Q.x, -Q.y, Q.w);
    }

    // Transform 변환
    inline physx::PxTransform ToPxTransform(const FTransform& T)
    {
        return physx::PxTransform(ToPxVec3(T.Translation), ToPxQuat(T.Rotation));
    }

    inline FTransform ToFTransform(const physx::PxTransform& T)
    {
        return FTransform(ToFVector(T.p), ToFQuat(T.q), FVector::One());
    }
}
```

---

## 10. 성능 최적화 팁

1. **기하학 단순화**: Complex collision 대신 Simple collision 사용
2. **UBodySetup 공유**: 동일 메시는 같은 BodySetup 재사용
3. **수면 최적화**: 정지한 객체는 자동 수면 처리
4. **충돌 필터링**: 불필요한 충돌 쌍 조기 제거
5. **Substep 최소화**: 필요한 경우에만 서브스텝 활성화
6. **비동기 시뮬레이션**: 가능하면 물리 스레드 분리 활용
