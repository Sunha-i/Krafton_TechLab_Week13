# WEEK13 작업 분배

> 6일, 4명 팀 기준 | 병렬 작업 가능한 형태로 구성

---

## 작업량 비교

| 팀원 | 작업 수 | 주요 작업 | 예상 부하 |
|------|--------|----------|----------|
| A | 5개 | PhysX 빌드/통합, 래퍼, 콜백, 멀티스레드 | ★★★★☆ |
| B | 7개 | UPhysicsAsset, FBodyInstance, Shape, NvCloth 빌드/통합 | ★★★★☆ |
| C | 5개 | Ragdoll 구조/Joint/시뮬, Vehicle 기초/입력 | ★★★★☆ |
| D | 9개 | DOF, Physics Editor, Debug 렌더링, Cloth 시뮬레이션 | ★★★★☆ |

> 모든 팀원의 작업량이 균등하게 분배됨

---

## 작업 분배 개요

| 팀원 | 담당 영역 | 핵심 키워드 |
|------|----------|------------|
| A | PhysX SDK 통합 및 기반 구축 | PhysX 초기화, 래퍼, 콜백, 멀티스레드 |
| B | Physics Asset & Body & **NvCloth SDK** | UPhysicsAsset, FBodyInstance, Shape, **NvCloth 빌드/통합** |
| C | Ragdoll & Vehicle 시뮬레이션 | Joint, Ragdoll, PxVehicle |
| D | 에디터 UI & 렌더링 & **Cloth** | Physics Editor, Debug 렌더링, DOF, **Cloth 시뮬레이션** |

---

## 👤 팀원 A: PhysX SDK 통합 및 기반 구축

**담당 영역:** Engine Core - PhysX 기반

### 작업 목록

| 작업 | 설명 |
|------|------|
| PhysX 4.1 SDK 빌드 | VS2022 지원 패치 적용, 빌드 검증 |
| PhysX 엔진 통합 | `PxFoundation`, `PxPhysics`, `PxScene` 초기화 |
| 데이터 구조 래퍼 | `FVector`↔`PxVec3`, `FTransform`↔`PxTransform` 등 |
| Event Callback 구현 | `PxSimulationEventCallback` (충돌, 트리거, Sleep/Wake) |
| Multithread 지원 | `PxDefaultCpuDispatcher`, `PxSceneReadLock` |

### 주요 구현 코드

```cpp
// PhysX 전역 초기화
PxDefaultAllocator      gAllocator;
PxDefaultErrorCallback  gErrorCallback;
PxFoundation*           gFoundation = nullptr;
PxPhysics*              gPhysics = nullptr;
PxScene*                gScene = nullptr;
PxMaterial*             gMaterial = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;

void InitPhysX() {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale());
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0, -9.81f, 0);
    gDispatcher = PxDefaultCpuDispatcherCreate(4);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    gScene = gPhysics->createScene(sceneDesc);
}
```

### 의존성
- **없음** (가장 먼저 시작하는 기반 작업)

### 산출물
- 다른 팀원들이 사용할 PhysX 인터페이스
- 물리 시뮬레이션 기본 루프

---

## 👤 팀원 B: Physics Asset & Body & NvCloth SDK

**담당 영역:** Engine Core - Asset 구조 & NvCloth 기반

### 작업 목록

| 작업 | 설명 |
|------|------|
| `UPhysicsAsset` 클래스 | `UBodySetup`, `FKAggregateGeom` 구현 |
| `FBodyInstance` 구현 | PhysX Actor와 게임 오브젝트 연결 (`userData`) |
| StaticMesh Body 연결 | `UStaticMesh`에 `BodySetup` 통합 |
| SkeletalMesh Body 연결 | `USkeletalMesh`에 `PhysicsAsset` 통합 |
| Shape 생성 | Sphere, Box, Capsule, Convex 지오메트리 생성 |
| **NvCloth 1.1.6 SDK 빌드** | CUDA 10.0 지원, VS2022 빌드 |
| **NvCloth 엔진 통합** | `NvClothFactory`, `NvClothSolver` 초기화 |

### 주요 구현 코드

```cpp
struct FKAggregateGeom
{
    TArray<FKSphereElem> SphereElems;
    TArray<FKBoxElem> BoxElems;
    TArray<FKSphylElem> SphylElems;
    TArray<FKConvexElem> ConvexElems;
};

class UBodySetup : public UBodySetupCore
{
    FName BoneName;
    struct FKAggregateGeom AggGeom;
};

class UPhysicsAsset : public UObject
{
    TArray<UBodySetup*> BodySetup;
};

struct FBodyInstance
{
    UPrimitiveComponent* OwnerComponent;
    PxRigidActor* RigidActor;
};
```

### 의존성
- 팀원 A의 PhysX 초기화 (Day 2부터 통합 테스트)

### NvCloth 초기화 코드

```cpp
#include <NvCloth/Factory.h>
#include <NvCloth/Solver.h>

nv::cloth::Factory* gClothFactory = nullptr;
nv::cloth::Solver* gClothSolver = nullptr;

void InitNvCloth()
{
    // CPU Factory 생성 (또는 CUDA Factory)
    gClothFactory = NvClothCreateFactoryCPU();

    // Solver 생성
    gClothSolver = gClothFactory->createSolver();
}

void ShutdownNvCloth()
{
    if (gClothSolver)
    {
        delete gClothSolver;
        gClothSolver = nullptr;
    }
    if (gClothFactory)
    {
        NvClothDestroyFactory(gClothFactory);
        gClothFactory = nullptr;
    }
}
```

### 산출물
- 메시에 물리 바디 부착 기능
- PhysX Actor ↔ 게임 오브젝트 매핑
- **NvCloth 인터페이스 (팀원 D가 사용)**

---

## 👤 팀원 C: Ragdoll & Vehicle 시뮬레이션

**담당 영역:** Engine Core - 물리 시뮬레이션

### 작업 목록

| 작업 | 설명 |
|------|------|
| Ragdoll 구조 설계 | `RagdollBone` 구조체, 본 계층 정의 |
| Joint 시스템 | `PxD6Joint`, Twist/Swing 각도 제한 |
| Ragdoll 시뮬레이션 | `CreateRagdoll()`, 본 위치 업데이트 |
| Vehicle 기초 | `PxVehicleDrive4W`, 휠/서스펜션 설정 |
| Vehicle 입력 처리 | `PxVehicleDrive4WRawInputData` 연동 |

### 주요 구현 코드

```cpp
struct RagdollBone
{
    const char* name;
    PxVec3 offset;           // 부모로부터의 위치
    PxVec3 halfSize;         // Capsule or box 크기
    int parentIndex;         // -1이면 루트
    PxRigidDynamic* body = nullptr;
    PxJoint* joint = nullptr;
};

void CreateRagdoll(const PxVec3& worldRoot)
{
    for (int i = 0; i < boneCount; ++i)
    {
        RagdollBone& bone = bones[i];

        // 부모 기준 위치 계산
        PxVec3 parentPos = (bone.parentIndex >= 0)
            ? bones[bone.parentIndex].body->getGlobalPose().p
            : worldRoot;
        PxVec3 bonePos = parentPos + bone.offset;

        // 바디 생성
        PxRigidDynamic* body = gPhysics->createRigidDynamic(PxTransform(bonePos));
        // ... Shape 생성 및 부착 ...

        // D6 Joint 연결 (부모가 있는 경우)
        if (bone.parentIndex >= 0)
        {
            PxD6Joint* joint = PxD6JointCreate(...);
            joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
            joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
            joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
            joint->setTwistLimit(PxJointAngularLimitPair(-PxPi/4, PxPi/4));
            joint->setSwingLimit(PxJointLimitCone(PxPi/6, PxPi/6));
        }
    }
}
```

### 의존성
- 팀원 A의 PhysX Scene (Day 2부터 통합)
- 팀원 B의 Body 시스템 (Day 3부터 통합)

### 산출물
- 래그돌 물리 시뮬레이션
- 기본 차량 물리

---

## 👤 팀원 D: 에디터 UI & 렌더링 & Cloth

**담당 영역:** Editor & Rendering - 시각화 & Cloth 시뮬레이션

### 작업 목록

| 작업 | 설명 |
|------|------|
| Physics Asset Editor | 본/조인트 편집 UI (ImGui 기반) |
| Debug 렌더링 | 물리 바디 와이어프레임 시각화 |
| Show Flag 구현 | 물리 디버그 표시 토글 |
| Stat 지원 | 물리 시뮬레이션 통계 표시 |
| **Depth of Field** | 포스트프로세스 셰이더 구현 |
| DOF UI | F-Stop, Focus Distance 조절 UI |
| **Cloth 메시 생성** | `nv::cloth::Fabric`, 정점/삼각형 데이터 설정 |
| **Cloth 시뮬레이션** | `nv::cloth::Cloth`, 중력/바람/충돌 설정 |
| **Cloth 렌더링 연동** | 시뮬레이션 결과를 메시 버텍스에 반영 |

### 주요 구현 코드

```hlsl
// Depth of Field 셰이더 (PostProcess)
float CalculateBlurAmount(float pixelDepth, float focusDepth, float focusRange)
{
    return saturate(abs(pixelDepth - focusDepth) / focusRange);
}

// 메인 DOF 패스
float4 PS_DepthOfField(VS_OUTPUT input) : SV_Target
{
    float depth = DepthTexture.Sample(sampler, input.UV).r;
    float blur = CalculateBlurAmount(depth, FocusDistance, FocusRange);

    float4 sharpColor = SceneTexture.Sample(pointSampler, input.UV);
    float4 blurColor = BlurredTexture.Sample(linearSampler, input.UV);

    return lerp(sharpColor, blurColor, blur);
}
```

### Cloth 시뮬레이션 주요 구현 코드

```cpp
#include <NvCloth/Cloth.h>
#include <NvCloth/Fabric.h>

// Cloth 생성 (팀원 B의 gClothFactory, gClothSolver 사용)
nv::cloth::ClothMeshDesc meshDesc;
meshDesc.points.data = vertices;
meshDesc.points.stride = sizeof(PxVec4);
meshDesc.points.count = vertexCount;

meshDesc.triangles.data = indices;
meshDesc.triangles.stride = sizeof(uint32_t) * 3;
meshDesc.triangles.count = triangleCount;

// Fabric 생성 (천의 구조 정의)
nv::cloth::Fabric* fabric = NvClothCookFabricFromMesh(
    gClothFactory, meshDesc, PxVec3(0, -1, 0));

// Cloth 인스턴스 생성
nv::cloth::Cloth* cloth = gClothFactory->createCloth(
    nv::cloth::Range<PxVec4>(particles, particles + vertexCount), *fabric);

// 시뮬레이션 파라미터 설정
cloth->setGravity(PxVec3(0, -9.81f, 0));
cloth->setDamping(PxVec3(0.1f, 0.1f, 0.1f));
cloth->setFriction(0.5f);

// 고정 정점 설정 (예: 상단 고정)
nv::cloth::Range<PxVec4> invMasses = cloth->getParticleInvMasses();
for (int i = 0; i < topRowCount; ++i)
    invMasses[topRowIndices[i]].w = 0.0f;  // 고정

// Solver에 추가
gClothSolver->addCloth(cloth);

// 시뮬레이션 루프
void SimulateCloth(float dt)
{
    gClothSolver->beginSimulation(dt);
    for (int i = 0; i < gClothSolver->getSimulationChunkCount(); ++i)
        gClothSolver->simulateChunk(i);
    gClothSolver->endSimulation();

    // 결과를 렌더링 버텍스에 반영
    nv::cloth::Range<const PxVec4> particles = cloth->getCurrentParticles();
    for (uint32_t i = 0; i < vertexCount; ++i)
    {
        renderVertices[i].position = PxVec3(particles[i].x, particles[i].y, particles[i].z);
    }
}
```

### 의존성

- DOF/Editor UI: **없음** (Day 1부터 독립 작업 가능)
- Cloth: 팀원 B의 NvCloth 초기화 (Day 3부터 통합)

### 산출물

- Physics Asset 에디터 UI
- 물리 디버그 시각화
- DOF 포스트프로세스 효과
- **Cloth 시뮬레이션 (천, 깃발, 망토 등)**

---

## 일정 타임라인

```
┌─────────┬────────────────────┬────────────────────┬────────────────────┬────────────────────┐
│   일정   │      팀원 A        │      팀원 B        │      팀원 C        │      팀원 D        │
├─────────┼────────────────────┼────────────────────┼────────────────────┼────────────────────┤
│ Day 1-2 │ PhysX SDK 빌드     │ UPhysicsAsset      │ Ragdoll 구조 설계  │ DOF 셰이더 구현    │
│         │ PhysX 초기화       │ 클래스 설계        │ 본 계층 정의       │ DOF UI            │
│         │                    │ NvCloth SDK 빌드   │                    │                    │
├─────────┼────────────────────┼────────────────────┼────────────────────┼────────────────────┤
│ Day 3-4 │ Event Callback     │ FBodyInstance      │ Joint 시스템       │ Physics Editor UI  │
│         │ Multithread 지원   │ 메시 연결          │ Ragdoll 생성       │ Cloth 메시 생성    │
│         │                    │ NvCloth 통합       │                    │                    │
├─────────┼────────────────────┼────────────────────┼────────────────────┼────────────────────┤
│ Day 5-6 │ 전체 통합          │ Shape 생성         │ Vehicle 시스템     │ Cloth 시뮬레이션   │
│         │ 버그 수정          │ 테스트             │                    │ Debug 렌더링       │
│         │                    │                    │                    │ Show Flag / Stat   │
└─────────┴────────────────────┴────────────────────┴────────────────────┴────────────────────┘
```

---

## 병렬 작업 구조

```
Day 1   Day 2   Day 3   Day 4   Day 5   Day 6
  │       │       │       │       │       │
  ▼       ▼       ▼       ▼       ▼       ▼
┌───────────────────────────────────────────────────────┐
│ A: PhysX SDK 빌드 ──→ 초기화 ──→ Callback ──→ 통합    │ ◀── PhysX 기반
└──────────────┬────────────────────────────────────────┘
               │
               ▼ (PhysX 인터페이스 제공)
┌───────────────────────────────────────────────────────┐
│ B: Asset 설계 → NvCloth 빌드 → FBodyInstance → Shape  │ ◀── Asset + NvCloth
└──────────────┬────────────────────────────────────────┘
               │
               ▼ (Body + NvCloth 인터페이스 제공)
┌───────────────────────────────────────────────────────┐
│ C: Ragdoll 설계 ──→ Joint/Ragdoll ──→ Vehicle         │
└───────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────┐
│ D: DOF 셰이더 → Editor UI → Cloth 시뮬 → Debug 렌더링 │ ◀── 렌더링 + Cloth
└───────────────────────────────────────────────────────┘
```

---

## 병렬 작업 가능 이유

| 팀원 | 독립성 | 설명 |
|------|--------|------|
| A | ★★★★★ | PhysX 기반 레이어, 다른 모든 작업의 기반 |
| B | ★★★★☆ | A의 PhysX 인터페이스 사용 + NvCloth SDK 담당 |
| C | ★★★★☆ | A/B의 인터페이스 사용, Ragdoll/Vehicle 집중 |
| D | ★★★★☆ | 렌더링/UI 독립 + B의 NvCloth로 Cloth 구현 |

### 핵심 포인트

- **팀원 A**: PhysX에만 집중하여 작업량 적정화
- **팀원 B**: Asset 시스템 + NvCloth SDK로 기반 역할 분담
- **팀원 C**: Ragdoll + Vehicle만 담당하여 집중도 향상
- **팀원 D**: DOF/UI는 Day 1부터 독립, Cloth는 Day 3부터 B의 NvCloth 사용
- **Cloth 시뮬레이션**은 렌더링 연동이 핵심이므로 팀원 D가 담당

---

## 참고 자료

### PhysX Documentation
- [PhysX 4.1 Guide](https://nvidiagameworks.github.io/PhysX/4.1/documentation/physxguide/Index.html)
- [PhysX Geometry](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html)
- [PxSimulationEventCallback](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/apireference/files/classPxSimulationEventCallback.html)

### NvCloth Documentation
- [NvCloth 1.1 Documentation](https://nvidiagameworks.github.io/NvCloth/1.1/index.html)
- CUDA 10.0 필요

### Depth of Field
- [Unreal Engine: Depth of Field](https://dev.epicgames.com/documentation/en-us/unreal-engine/depth-of-field-in-unreal-engine)
- [Wikipedia: F-number](https://en.wikipedia.org/wiki/F-number)

### Degrees of Freedom
- [YouTube: Degrees of Freedom](https://www.youtube.com/watch?v=8zbpHu_7FHc)
- [Wikipedia: Degrees of Freedom (Mechanics)](https://en.wikipedia.org/wiki/Degrees_of_freedom_(mechanics))
