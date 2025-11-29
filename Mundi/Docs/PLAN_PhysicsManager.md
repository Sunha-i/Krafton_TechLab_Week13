# PhysicsManager 구현 계획

## 개요

- **담당**: 팀원 A (PhysX SDK 통합 및 기반 구축)
- **목표**: PhysX 물리 시뮬레이션 관리자 구현
- **의존성**: PhysicsTypes.h (완료)

---

## 파일 구조

```
Mundi/Source/Runtime/Engine/Physics/
├── PhysicsTypes.h          ← 완료 (좌표계 변환 유틸리티)
├── PhysicsManager.h        ← 신규 생성
├── PhysicsManager.cpp      ← 신규 생성
├── PhysicsEventCallback.h  ← 신규 생성
├── PhysicsEventCallback.cpp← 신규 생성
└── PhysicsLock.h           ← 신규 생성
```

---

## 작업 1: PhysicsManager 클래스 설계

### 1.1 PhysicsManager.h

```cpp
#pragma once

#include "Object.h"
#include "PhysicsTypes.h"
#include <PxPhysicsAPI.h>
#include <memory>

// 전방 선언
class UWorld;
class FPhysicsEventCallback;

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
     * @brief 물리 통계 정보 반환 (디버그용)
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

    // 이벤트 콜백
    std::unique_ptr<FPhysicsEventCallback> EventCallback;

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
```

### 1.2 PhysicsManager.cpp 핵심 구현

```cpp
#include "pch.h"
#include "PhysicsManager.h"
#include "PhysicsEventCallback.h"
#include "World.h"
#include "GlobalConsole.h"

IMPLEMENT_CLASS(UPhysicsManager)

using namespace physx;

UPhysicsManager::UPhysicsManager()
{
}

UPhysicsManager::~UPhysicsManager()
{
    Shutdown();
}

void UPhysicsManager::Initialize(UWorld* InWorld)
{
    if (bIsInitialized)
    {
        UE_LOG("PhysicsManager: Already initialized");
        return;
    }

    World = InWorld;

    // 1. Foundation 생성
    if (!CreateFoundation())
    {
        UE_LOG("PhysicsManager: Failed to create Foundation");
        return;
    }

    // 2. PVD 연결 (디버그 빌드에서만)
#ifdef _DEBUG
    ConnectPVD();
#endif

    // 3. Physics 생성
    if (!CreatePhysics())
    {
        UE_LOG("PhysicsManager: Failed to create Physics");
        Shutdown();
        return;
    }

    // 4. Extensions 초기화 (Joint, Vehicle 등 사용을 위해)
    if (!PxInitExtensions(*Physics, Pvd))
    {
        UE_LOG("PhysicsManager: Failed to initialize Extensions");
        Shutdown();
        return;
    }

    // 5. Cooking 생성 (ConvexMesh 등 생성용)
    Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, PxCookingParams(Physics->getTolerancesScale()));
    if (!Cooking)
    {
        UE_LOG("PhysicsManager: Failed to create Cooking");
        Shutdown();
        return;
    }

    // 6. 기본 Material 생성
    DefaultMaterial = Physics->createMaterial(DefaultStaticFriction, DefaultDynamicFriction, DefaultRestitution);

    // 7. Scene 생성
    if (!CreateScene())
    {
        UE_LOG("PhysicsManager: Failed to create Scene");
        Shutdown();
        return;
    }

    bIsInitialized = true;
    UE_LOG("PhysicsManager: Initialization complete");
}

bool UPhysicsManager::CreateFoundation()
{
    Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, Allocator, ErrorCallback);
    return Foundation != nullptr;
}

bool UPhysicsManager::CreatePhysics()
{
    Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), true, Pvd);
    return Physics != nullptr;
}

bool UPhysicsManager::CreateScene()
{
    PxSceneDesc SceneDesc(Physics->getTolerancesScale());

    // 중력 설정 (PhysX 좌표계: Y-Up)
    // Mundi에서 Z-Up이므로 PhysX에서는 Y-Down
    SceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);

    // CPU Dispatcher 생성
    Dispatcher = PxDefaultCpuDispatcherCreate(CpuDispatcherThreadCount);
    SceneDesc.cpuDispatcher = Dispatcher;

    // 필터 셰이더 설정
    SceneDesc.filterShader = PxDefaultSimulationFilterShader;

    // 이벤트 콜백 설정
    EventCallback = std::make_unique<FPhysicsEventCallback>(this);
    SceneDesc.simulationEventCallback = EventCallback.get();

    // Scene 플래그 설정
    SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;  // 활성 Actor 추적
    SceneDesc.flags |= PxSceneFlag::eENABLE_CCD;            // 연속 충돌 감지
    SceneDesc.flags |= PxSceneFlag::eENABLE_PCM;            // 지속적 접촉 다중점

    Scene = Physics->createScene(SceneDesc);
    if (!Scene)
        return false;

    // PVD Scene 연결
    if (Pvd && Pvd->isConnected())
    {
        PxPvdSceneClient* PvdClient = Scene->getScenePvdClient();
        if (PvdClient)
        {
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    return true;
}

bool UPhysicsManager::ConnectPVD()
{
    Pvd = PxCreatePvd(*Foundation);
    if (!Pvd)
        return false;

    PvdTransport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    if (!PvdTransport)
    {
        Pvd->release();
        Pvd = nullptr;
        return false;
    }

    bool bConnected = Pvd->connect(*PvdTransport, PxPvdInstrumentationFlag::eALL);
    if (bConnected)
    {
        UE_LOG("PhysicsManager: PVD connected successfully (127.0.0.1:5425)");
    }
    else
    {
        UE_LOG("PhysicsManager: PVD connection failed - Make sure PVD app is running");
    }

    return bConnected;
}

void UPhysicsManager::Simulate(float DeltaTime)
{
    if (!bIsInitialized || !Scene)
        return;

    // 최소/최대 스텝 제한
    DeltaTime = FMath::Clamp(DeltaTime, 1.0f / 240.0f, 1.0f / 30.0f);

    bIsSimulating = true;
    Scene->simulate(DeltaTime);
}

void UPhysicsManager::FetchResults(bool bBlock)
{
    if (!bIsSimulating || !Scene)
        return;

    Scene->fetchResults(bBlock);
    bIsSimulating = false;

    // Active Actors 처리 (추후 FBodyInstance 연동 시 구현)
    // PxU32 NbActiveActors;
    // PxActor** ActiveActors = Scene->getActiveActors(NbActiveActors);
    // for (PxU32 i = 0; i < NbActiveActors; ++i) { ... }
}

void UPhysicsManager::Shutdown()
{
    if (!bIsInitialized)
        return;

    // 역순으로 해제
    DisconnectPVD();

    PxCloseExtensions();

    if (Scene)
    {
        Scene->release();
        Scene = nullptr;
    }

    if (Dispatcher)
    {
        Dispatcher->release();
        Dispatcher = nullptr;
    }

    if (Cooking)
    {
        Cooking->release();
        Cooking = nullptr;
    }

    // Material은 Physics가 관리하므로 별도 해제 불필요

    if (Physics)
    {
        Physics->release();
        Physics = nullptr;
    }

    if (Foundation)
    {
        Foundation->release();
        Foundation = nullptr;
    }

    EventCallback.reset();
    World = nullptr;
    bIsInitialized = false;

    UE_LOG("PhysicsManager: Shutdown complete");
}

physx::PxMaterial* UPhysicsManager::CreateMaterial(float StaticFriction, float DynamicFriction, float Restitution)
{
    if (!Physics)
        return nullptr;

    return Physics->createMaterial(StaticFriction, DynamicFriction, Restitution);
}

UPhysicsManager::FPhysicsStats UPhysicsManager::GetStats() const
{
    FPhysicsStats Stats;

    if (Scene)
    {
        Stats.ActiveActorCount = Scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
        Stats.StaticActorCount = Scene->getNbActors(PxActorTypeFlag::eRIGID_STATIC);
        Stats.DynamicActorCount = Scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
    }

    return Stats;
}

void UPhysicsManager::DisconnectPVD()
{
    if (Pvd)
    {
        if (Pvd->isConnected())
            Pvd->disconnect();
        Pvd->release();
        Pvd = nullptr;
    }

    if (PvdTransport)
    {
        PvdTransport->release();
        PvdTransport = nullptr;
    }
}
```

---

## 작업 2: PhysicsEventCallback 구현

### 2.1 PhysicsEventCallback.h

```cpp
#pragma once

#include <PxPhysicsAPI.h>

class UPhysicsManager;

/**
 * @brief PhysX 시뮬레이션 이벤트 콜백
 *
 * 충돌, 트리거, Sleep/Wake 이벤트를 처리합니다.
 */
class FPhysicsEventCallback : public physx::PxSimulationEventCallback
{
public:
    FPhysicsEventCallback(UPhysicsManager* InManager);
    virtual ~FPhysicsEventCallback() = default;

    // ═══════════════════════════════════════════════════════════════════════
    // PxSimulationEventCallback 구현
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * @brief 충돌 이벤트 처리
     * 두 Actor가 접촉했을 때 호출됩니다.
     */
    virtual void onContact(
        const physx::PxContactPairHeader& PairHeader,
        const physx::PxContactPair* Pairs,
        physx::PxU32 NbPairs) override;

    /**
     * @brief 트리거 이벤트 처리
     * Actor가 트리거 볼륨에 진입/이탈했을 때 호출됩니다.
     */
    virtual void onTrigger(
        physx::PxTriggerPair* Pairs,
        physx::PxU32 Count) override;

    /**
     * @brief 조인트 파괴 이벤트 처리
     */
    virtual void onConstraintBreak(
        physx::PxConstraintInfo* Constraints,
        physx::PxU32 Count) override;

    /**
     * @brief Actor 활성화 이벤트
     */
    virtual void onWake(
        physx::PxActor** Actors,
        physx::PxU32 Count) override;

    /**
     * @brief Actor 휴면 이벤트
     */
    virtual void onSleep(
        physx::PxActor** Actors,
        physx::PxU32 Count) override;

    /**
     * @brief 고급 시뮬레이션 이벤트 (CCD 결과)
     */
    virtual void onAdvance(
        const physx::PxRigidBody* const* BodyBuffer,
        const physx::PxTransform* PoseBuffer,
        const physx::PxU32 Count) override;

private:
    UPhysicsManager* PhysicsManager;
};
```

### 2.2 PhysicsEventCallback.cpp

```cpp
#include "pch.h"
#include "PhysicsEventCallback.h"
#include "PhysicsManager.h"

using namespace physx;

FPhysicsEventCallback::FPhysicsEventCallback(UPhysicsManager* InManager)
    : PhysicsManager(InManager)
{
}

void FPhysicsEventCallback::onContact(
    const PxContactPairHeader& PairHeader,
    const PxContactPair* Pairs,
    PxU32 NbPairs)
{
    // Actor가 삭제 중인 경우 무시
    if (PairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_0 ||
        PairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_1)
    {
        return;
    }

    // userData를 통해 FBodyInstance 접근 (팀원 B 작업 후 구현)
    // FBodyInstance* BodyA = static_cast<FBodyInstance*>(PairHeader.actors[0]->userData);
    // FBodyInstance* BodyB = static_cast<FBodyInstance*>(PairHeader.actors[1]->userData);

    for (PxU32 i = 0; i < NbPairs; ++i)
    {
        const PxContactPair& Pair = Pairs[i];

        // 충돌 시작
        if (Pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            // OnCollisionBegin 이벤트 발생 (추후 구현)
        }

        // 충돌 지속
        if (Pair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
        {
            // OnCollisionStay 이벤트 발생 (추후 구현)
        }

        // 충돌 종료
        if (Pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            // OnCollisionEnd 이벤트 발생 (추후 구현)
        }
    }
}

void FPhysicsEventCallback::onTrigger(PxTriggerPair* Pairs, PxU32 Count)
{
    for (PxU32 i = 0; i < Count; ++i)
    {
        const PxTriggerPair& Pair = Pairs[i];

        // 삭제된 Actor 무시
        if (Pair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER |
                          PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
        {
            continue;
        }

        // 트리거 진입
        if (Pair.status == PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            // OnTriggerEnter 이벤트 발생 (추후 구현)
        }

        // 트리거 이탈
        if (Pair.status == PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            // OnTriggerExit 이벤트 발생 (추후 구현)
        }
    }
}

void FPhysicsEventCallback::onConstraintBreak(PxConstraintInfo* Constraints, PxU32 Count)
{
    // Joint 파괴 이벤트 처리 (팀원 C Ragdoll 작업과 연동)
    for (PxU32 i = 0; i < Count; ++i)
    {
        // OnJointBreak 이벤트 발생 (추후 구현)
    }
}

void FPhysicsEventCallback::onWake(PxActor** Actors, PxU32 Count)
{
    // Actor 활성화 로깅 (디버그용)
}

void FPhysicsEventCallback::onSleep(PxActor** Actors, PxU32 Count)
{
    // Actor 휴면 로깅 (디버그용)
}

void FPhysicsEventCallback::onAdvance(
    const PxRigidBody* const* BodyBuffer,
    const PxTransform* PoseBuffer,
    const PxU32 Count)
{
    // CCD 고급 이벤트 - 현재 미사용
}
```

---

## 작업 3: PhysicsLock.h 구현

```cpp
#pragma once

#include <PxPhysicsAPI.h>

// ═══════════════════════════════════════════════════════════════════════════
// 스레드 안전 Scene 접근 매크로
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Scene 읽기 락 (여러 스레드에서 동시 읽기 가능)
 * @param Scene PxScene 포인터
 *
 * 사용 예:
 * {
 *     SCOPED_SCENE_READ_LOCK(PhysicsManager->GetScene());
 *     PxTransform Pose = Actor->getGlobalPose();
 * }
 */
#define SCOPED_SCENE_READ_LOCK(Scene) \
    physx::PxSceneReadLock _ScopedReadLock_##__LINE__(Scene)

/**
 * @brief Scene 쓰기 락 (단일 스레드만 접근 가능)
 * @param Scene PxScene 포인터
 *
 * 사용 예:
 * {
 *     SCOPED_SCENE_WRITE_LOCK(PhysicsManager->GetScene());
 *     Actor->setGlobalPose(NewPose);
 * }
 */
#define SCOPED_SCENE_WRITE_LOCK(Scene) \
    physx::PxSceneWriteLock _ScopedWriteLock_##__LINE__(Scene)

// ═══════════════════════════════════════════════════════════════════════════
// RAII 스타일 락 클래스 (선택적 사용)
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Scene 읽기 락 RAII 래퍼
 */
class FPhysicsReadLock
{
public:
    explicit FPhysicsReadLock(physx::PxScene* InScene)
        : Scene(InScene)
    {
        if (Scene)
            Scene->lockRead();
    }

    ~FPhysicsReadLock()
    {
        if (Scene)
            Scene->unlockRead();
    }

    // 복사 금지
    FPhysicsReadLock(const FPhysicsReadLock&) = delete;
    FPhysicsReadLock& operator=(const FPhysicsReadLock&) = delete;

private:
    physx::PxScene* Scene;
};

/**
 * @brief Scene 쓰기 락 RAII 래퍼
 */
class FPhysicsWriteLock
{
public:
    explicit FPhysicsWriteLock(physx::PxScene* InScene)
        : Scene(InScene)
    {
        if (Scene)
            Scene->lockWrite();
    }

    ~FPhysicsWriteLock()
    {
        if (Scene)
            Scene->unlockWrite();
    }

    // 복사 금지
    FPhysicsWriteLock(const FPhysicsWriteLock&) = delete;
    FPhysicsWriteLock& operator=(const FPhysicsWriteLock&) = delete;

private:
    physx::PxScene* Scene;
};
```

---

## 작업 4: World 통합

### 4.1 World.h 수정 사항

```cpp
// 전방 선언 추가 (line ~30)
class UPhysicsManager;

// 멤버 변수 추가 (line ~180, 다른 매니저 옆)
private:
    std::unique_ptr<UPhysicsManager> PhysicsManager;

// 접근자 추가 (line ~130, 다른 Get 함수 옆)
public:
    UPhysicsManager* GetPhysicsManager() const { return PhysicsManager.get(); }
```

### 4.2 World.cpp 수정 사항

```cpp
// include 추가 (파일 상단)
#include "Physics/PhysicsManager.h"

// 생성자 또는 Initialize()에서 PhysicsManager 생성 (line ~140)
// CollisionManager 생성 직후
if (!IsPreviewWorld())
{
    PhysicsManager = std::make_unique<UPhysicsManager>();
    PhysicsManager->Initialize(this);
}

// Tick()에서 물리 시뮬레이션 호출 (line ~337, CollisionManager 업데이트 직전)
// 물리 시뮬레이션은 게임 로직 전에 실행
if (PhysicsManager && PhysicsManager->IsInitialized())
{
    PhysicsManager->Simulate(GetDeltaTime(EDeltaTime::Game));
    PhysicsManager->FetchResults(true);
}

// 소멸자에서 정리 (필요시)
// unique_ptr이므로 자동 해제됨
```

---

## 작업 5: vcxproj 등록

### 5.1 Mundi.vcxproj에 추가할 항목

```xml
<!-- ClInclude 섹션 -->
<ClInclude Include="Source\Runtime\Engine\Physics\PhysicsManager.h" />
<ClInclude Include="Source\Runtime\Engine\Physics\PhysicsEventCallback.h" />
<ClInclude Include="Source\Runtime\Engine\Physics\PhysicsLock.h" />

<!-- ClCompile 섹션 -->
<ClCompile Include="Source\Runtime\Engine\Physics\PhysicsManager.cpp" />
<ClCompile Include="Source\Runtime\Engine\Physics\PhysicsEventCallback.cpp" />
```

### 5.2 Mundi.vcxproj.filters에 추가할 항목

```xml
<!-- Filter 정의 -->
<Filter Include="Source\Runtime\Engine\Physics">
    <UniqueIdentifier>{새로운-GUID-생성}</UniqueIdentifier>
</Filter>

<!-- 헤더 파일 -->
<ClInclude Include="Source\Runtime\Engine\Physics\PhysicsManager.h">
    <Filter>Source\Runtime\Engine\Physics</Filter>
</ClInclude>
<ClInclude Include="Source\Runtime\Engine\Physics\PhysicsEventCallback.h">
    <Filter>Source\Runtime\Engine\Physics</Filter>
</ClInclude>
<ClInclude Include="Source\Runtime\Engine\Physics\PhysicsLock.h">
    <Filter>Source\Runtime\Engine\Physics</Filter>
</ClInclude>

<!-- 소스 파일 -->
<ClCompile Include="Source\Runtime\Engine\Physics\PhysicsManager.cpp">
    <Filter>Source\Runtime\Engine\Physics</Filter>
</ClCompile>
<ClCompile Include="Source\Runtime\Engine\Physics\PhysicsEventCallback.cpp">
    <Filter>Source\Runtime\Engine\Physics</Filter>
</ClCompile>
```

---

## 구현 순서

| 순서 | 작업 | 파일 | 예상 작업량 |
|------|------|------|------------|
| 1 | PhysicsLock.h 생성 | PhysicsLock.h | 작음 |
| 2 | PhysicsEventCallback 생성 | .h, .cpp | 중간 |
| 3 | PhysicsManager 생성 | .h, .cpp | 큼 |
| 4 | vcxproj 등록 | Mundi.vcxproj, .filters | 작음 |
| 5 | World 통합 | World.h, World.cpp | 중간 |
| 6 | 빌드 및 테스트 | - | 중간 |

---

## 테스트 계획

### 기본 테스트

1. **빌드 테스트**
   - [ ] Debug 빌드 성공
   - [ ] Release 빌드 성공

2. **초기화 테스트**
   - [ ] PhysicsManager 생성 성공
   - [ ] Foundation, Physics, Scene 생성 확인
   - [ ] PVD 연결 확인 (Debug 빌드)

3. **시뮬레이션 테스트**
   - [ ] Simulate/FetchResults 호출 시 크래시 없음
   - [ ] 프레임레이트 저하 없음

### 통합 테스트 (PhysicsManager 완료 후)

```cpp
// 테스트 코드 예시 (임시 - 팀원 B FBodyInstance 작업 전)
void TestPhysicsManager(UWorld* World)
{
    UPhysicsManager* PhysMgr = World->GetPhysicsManager();
    if (!PhysMgr)
        return;

    physx::PxScene* Scene = PhysMgr->GetScene();
    physx::PxPhysics* Physics = PhysMgr->GetPhysics();
    physx::PxMaterial* Material = PhysMgr->GetDefaultMaterial();

    // 바닥 생성 (Static)
    physx::PxRigidStatic* Ground = PxCreatePlane(
        *Physics,
        physx::PxPlane(0, 1, 0, 0),  // Y-Up (PhysX 좌표계)
        *Material
    );
    Scene->addActor(*Ground);

    // 박스 생성 (Dynamic)
    physx::PxTransform BoxPose(physx::PxVec3(0, 5, 0));  // Y=5 위치
    physx::PxRigidDynamic* Box = PxCreateDynamic(
        *Physics,
        BoxPose,
        physx::PxBoxGeometry(0.5f, 0.5f, 0.5f),
        *Material,
        10.0f  // 밀도
    );
    Scene->addActor(*Box);

    // 시뮬레이션 후 박스가 떨어지는지 확인
}
```

---

## 다른 팀원 연동 인터페이스

### 팀원 B (FBodyInstance)

```cpp
// PhysicsManager에서 제공하는 인터페이스
physx::PxPhysics* GetPhysics();      // RigidBody 생성용
physx::PxScene* GetScene();          // Actor 추가/제거용
physx::PxMaterial* GetDefaultMaterial();
physx::PxMaterial* CreateMaterial(float, float, float);
physx::PxCooking* GetCooking();      // ConvexMesh 생성용
```

### 팀원 C (Ragdoll, Vehicle)

```cpp
// Joint 사용을 위한 Extensions 초기화 완료
// PxD6Joint, PxRevoluteJoint 등 사용 가능

// Vehicle 사용을 위한 Extensions 초기화 완료
// PxVehicleSDK 사용 가능 (별도 초기화 필요)
```

### 팀원 D (Debug 렌더링)

```cpp
// 디버그 정보 제공
FPhysicsStats GetStats();            // Actor 개수, 시뮬레이션 시간

// Scene 접근 (디버그 렌더링용)
physx::PxScene* GetScene();          // Actor 순회하여 Shape 정보 획득
```

---

## 참고 자료

- [PhysX 4.1 Startup Guide](https://nvidiagameworks.github.io/PhysX/4.1/documentation/physxguide/Manual/Startup.html)
- [PhysX Visual Debugger](https://developer.nvidia.com/physx-visual-debugger)
- [PxSimulationEventCallback](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/apireference/files/classPxSimulationEventCallback.html)
