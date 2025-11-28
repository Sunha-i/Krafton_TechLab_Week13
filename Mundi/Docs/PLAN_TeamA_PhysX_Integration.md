# 팀원 A: PhysX SDK 통합 및 기반 구축 계획

## 개요

- **담당**: Engine Core - PhysX 기반
- **목표**: 다른 팀원들이 사용할 PhysX 인터페이스 제공

---

## 작업 목록

### 작업 1: PhysX 4.1 SDK 빌드 (우선순위 1)

**파일 작업**: 외부 작업 (SDK 빌드 후 파일 배치)

**산출물 경로**:
```
Mundi/ThirdParty/include/PhysX/   <- 헤더 파일
Mundi/ThirdParty/lib/PhysX/Debug/   <- Debug 라이브러리
Mundi/ThirdParty/lib/PhysX/Release/ <- Release 라이브러리
```

**필요한 라이브러리 파일**:
- PhysX_64.lib
- PhysXCommon_64.lib
- PhysXFoundation_64.lib
- PhysXExtensions_static_64.lib

---

### 작업 2: vcxproj 설정 (우선순위 2)

**수정 파일**: `Mundi/Mundi.vcxproj`

**추가할 내용**:
- AdditionalIncludeDirectories에 `ThirdParty\include\PhysX` 추가
- AdditionalLibraryDirectories에 `ThirdParty\lib\PhysX\$(Configuration)\` 추가
- AdditionalDependencies에 PhysX 라이브러리들 추가

---

### 작업 3: PhysicsTypes.h 생성 (우선순위 3)

**생성 파일**: `Mundi/Source/Runtime/Engine/Physics/PhysicsTypes.h`

**좌표계 변환 규칙**:

| 항목 | Mundi 엔진 | PhysX |
|------|-----------|-------|
| Up 축 | Z | Y |
| Forward 축 | X | Z |
| Right 축 | Y | X |
| Handedness | Left-Handed | Right-Handed |

**구현할 함수**:
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
    inline physx::PxTransform ToPxTransform(const FTransform& T);
    inline FTransform ToFTransform(const physx::PxTransform& T);

    // 행렬 변환
    inline FMatrix ToFMatrix(const physx::PxMat44& M);
}
```

---

### 작업 4: PhysicsManager 생성 (우선순위 4)

**생성 파일**:
- `Mundi/Source/Runtime/Engine/Physics/PhysicsManager.h`
- `Mundi/Source/Runtime/Engine/Physics/PhysicsManager.cpp`

**클래스 구조**:
```cpp
class UPhysicsManager : public UObject
{
public:
    DECLARE_CLASS(UPhysicsManager, UObject)

    void Initialize();
    void Shutdown();
    void Simulate(float DeltaTime);
    void FetchResults(bool bBlock = true);

    physx::PxPhysics* GetPhysics() const;
    physx::PxScene* GetScene() const;
    physx::PxMaterial* GetDefaultMaterial() const;
    physx::PxMaterial* CreateMaterial(float StaticFriction, float DynamicFriction, float Restitution);

private:
    physx::PxDefaultAllocator Allocator;
    physx::PxDefaultErrorCallback ErrorCallback;
    physx::PxFoundation* Foundation = nullptr;
    physx::PxPhysics* Physics = nullptr;
    physx::PxScene* Scene = nullptr;
    physx::PxMaterial* DefaultMaterial = nullptr;
    physx::PxDefaultCpuDispatcher* Dispatcher = nullptr;
    physx::PxPvd* Pvd = nullptr;  // 디버거
    FPhysicsEventCallback* EventCallback = nullptr;
};
```

**Initialize() 구현 요점**:
- PxCreateFoundation 호출
- PVD 연결 (디버그 빌드에서만)
- PxCreatePhysics 호출
- PxInitExtensions 호출
- 기본 Material 생성 (0.5f, 0.5f, 0.6f)
- Scene 생성 (중력: Y-Down, 멀티스레드 지원)

**Scene 설정**:
```cpp
SceneDesc.gravity = PxVec3(0, -9.81f, 0);  // Y-Up (PhysX 기본)
SceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(4);
SceneDesc.filterShader = PxDefaultSimulationFilterShader;
SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
SceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
SceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
```

---

### 작업 5: PhysicsEventCallback 생성 (우선순위 5)

**생성 파일**:
- `Mundi/Source/Runtime/Engine/Physics/PhysicsEventCallback.h`
- `Mundi/Source/Runtime/Engine/Physics/PhysicsEventCallback.cpp`

**클래스 구조**:
```cpp
class FPhysicsEventCallback : public physx::PxSimulationEventCallback
{
public:
    FPhysicsEventCallback(UPhysicsManager* InManager);

    virtual void onContact(const physx::PxContactPairHeader& PairHeader,
                          const physx::PxContactPair* Pairs,
                          physx::PxU32 NbPairs) override;
    virtual void onTrigger(physx::PxTriggerPair* Pairs, physx::PxU32 Count) override;
    virtual void onConstraintBreak(physx::PxConstraintInfo* Constraints, physx::PxU32 Count) override;
    virtual void onWake(physx::PxActor** Actors, physx::PxU32 Count) override;
    virtual void onSleep(physx::PxActor** Actors, physx::PxU32 Count) override;
    virtual void onAdvance(const physx::PxRigidBody* const* BodyBuffer,
                          const physx::PxTransform* PoseBuffer,
                          const physx::PxU32 Count) override;

private:
    UPhysicsManager* PhysicsManager;
};
```

**userData 패턴**: PhysX Actor의 userData에 FBodyInstance* 저장하여 콜백에서 게임 오브젝트 접근

---

### 작업 6: PhysicsLock.h 생성 (우선순위 6)

**생성 파일**: `Mundi/Source/Runtime/Engine/Physics/PhysicsLock.h`

**내용**:
```cpp
#pragma once
#include <PxPhysicsAPI.h>

#define SCOPED_SCENE_READ_LOCK(Scene) \
    physx::PxSceneReadLock ScopedReadLock(Scene)

#define SCOPED_SCENE_WRITE_LOCK(Scene) \
    physx::PxSceneWriteLock ScopedWriteLock(Scene)
```

---

### 작업 7: World.h/cpp 수정 (우선순위 7)

**수정 파일**:
- `Mundi/Source/Runtime/Engine/GameFramework/World.h`
- `Mundi/Source/Runtime/Engine/GameFramework/World.cpp`

**추가할 내용**:
```cpp
// World.h
class UWorld final : public UObject
{
public:
    UPhysicsManager* GetPhysicsManager() { return PhysicsManager.get(); }

private:
    std::unique_ptr<UPhysicsManager> PhysicsManager;
};

// World.cpp - Initialize()에 추가
PhysicsManager = std::make_unique<UPhysicsManager>();
PhysicsManager->Initialize();

// World.cpp - Tick()에 추가
PhysicsManager->Simulate(DeltaSeconds);
PhysicsManager->FetchResults(true);
```

---

## 파일 생성/수정 요약

### 생성할 파일 (6개)
| 파일 | 설명 |
|------|------|
| `Source/Runtime/Engine/Physics/PhysicsTypes.h` | 좌표계 변환 유틸리티 |
| `Source/Runtime/Engine/Physics/PhysicsManager.h` | PhysX 관리자 헤더 |
| `Source/Runtime/Engine/Physics/PhysicsManager.cpp` | PhysX 관리자 구현 |
| `Source/Runtime/Engine/Physics/PhysicsEventCallback.h` | 이벤트 콜백 헤더 |
| `Source/Runtime/Engine/Physics/PhysicsEventCallback.cpp` | 이벤트 콜백 구현 |
| `Source/Runtime/Engine/Physics/PhysicsLock.h` | 스레드 락 매크로 |

### 수정할 파일 (3개)
| 파일 | 수정 내용 |
|------|----------|
| `Mundi.vcxproj` | PhysX include/lib 경로 추가 |
| `Source/Runtime/Engine/GameFramework/World.h` | PhysicsManager 멤버 추가 |
| `Source/Runtime/Engine/GameFramework/World.cpp` | PhysicsManager 초기화/Tick 연동 |

---

## 의존성

```
팀원 A (이 작업)
    │
    ├──→ 팀원 B: UPhysicsAsset, FBodyInstance
    ├──→ 팀원 C: Ragdoll, Vehicle
    └──→ 팀원 D: Debug 렌더링
```

---

## 테스트 체크리스트

- [ ] PhysX SDK 빌드 성공
- [ ] 엔진 링크 성공
- [ ] PhysicsManager 초기화/종료
- [ ] 기본 RigidBody 시뮬레이션
- [ ] PVD 연결 (디버그)
- [ ] 좌표계 변환 검증
- [ ] Event Callback 동작
- [ ] 멀티스레드 안정성
