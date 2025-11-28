# WEEK13 발제

## 학습 목표

### Understanding Focus and Force in 3D World
**(3차원 세상에서 초점과 힘의 이해)**

### 2개의 DOF : Depth of Field and Degrees of Freedom
**(피사계 심도와 운동 자유도)**

1. Degree of Freedom (Rigid body simulation)
2. Depth of Field (F-Stop)

---

## Editor & Rendering (눈에 보이는 세상)

### Physics Asset Editor 구현

### RagDoll Simulation 구현

```cpp
// Ragdoll

struct RagdollBone
{
    const char* name;
    PxVec3 offset;                // 부모로부터의 위치
    PxVec3 halfSize;              // Capsule or box 크기
    int parentIndex;              // -1이면 루트
    PxRigidDynamic* body = nullptr;
    PxJoint* joint = nullptr;
};

RagdollBone bones[] = {
    // name,         offset           size           parent
    {"pelvis",       PxVec3(0,  5, 0), PxVec3(0.2f, 0.3f, 0.2f), -1},
    {"spine",        PxVec3(0,  0.6f, 0), PxVec3(0.2f, 0.4f, 0.2f), 0},
    {"head",         PxVec3(0,  0.8f, 0), PxVec3(0.2f, 0.3f, 0.2f), 1},
};

void CreateRagdoll(const PxVec3& worldRoot)
{
    for (int i = 0; i < _countof(bones); ++i)
    {
        RagdollBone& bone = bones[i];

        // 부모의 위치 기준으로 위치 계산
        PxVec3 parentPos = (bone.parentIndex >= 0) ? bones[bone.parentIndex].body->getGlobalPose().p : worldRoot;
        PxVec3 bonePos = parentPos + bone.offset;

        // 바디 생성
        PxTransform pose(bonePos);
        PxRigidDynamic* body = gPhysics->createRigidDynamic(pose);
        PxShape* shape = gPhysics->createShape(PxCapsuleGeometry(bone.halfSize.x, bone.halfSize.y), *gMaterial);
        body->attachShape(*shape);
        PxRigidBodyExt::updateMassAndInertia(*body, 1.0f);
        gScene->addActor(*body);
        bone.body = body;

        // 조인트 연결
        if (bone.parentIndex >= 0)
        {
            RagdollBone& parent = bones[bone.parentIndex];

            PxTransform localFrameParent = PxTransform(parent.body->getGlobalPose().getInverse() * PxTransform(bonePos));
            PxTransform localFrameChild = PxTransform(PxVec3(0));

            PxD6Joint* joint = PxD6JointCreate(*gPhysics, parent.body, localFrameParent, bone.body, localFrameChild);

            // 각도 제한 설정
            joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eLIMITED);
            joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eLIMITED);
            joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLIMITED);
            joint->setTwistLimit(PxJointAngularLimitPair(-PxPi/4, PxPi/4));
            joint->setSwingLimit(PxJointLimitCone(PxPi/6, PxPi/6));

            bone.joint = joint;
        }
    }
}

void Simulate(float dt)
{
    gScene->simulate(dt);
    gScene->fetchResults(true);

    // Ragdoll 본들 위치 업데이트
    for (auto& bone : bones)
    {
        if (bone.body)
        {
            PxTransform t = bone.body->getGlobalPose();
            PxMat44 m(t);
            // 여기에 worldMatrix 계산해서 메시 렌더링용으로 넘겨주면 됨
        }
    }
}
```

### Show Flag, Stat 지원
- 디버깅용 Physic Body 렌더링 지원

### 나만의 자동차 구현

```cpp
// Car
// PxVehicleDrive4WRawInputData
// PxVehicleWheels
// PxVehicleDrive4W
// PxFixedSizeLookupTable
// PxVehicleSuspensionRaycasts
// PxVehicleUpdates
```

### Depth of Field 구현
- show flag 지원도 포함

```cpp
float blurAmount = saturate(abs(pixelDepth - focusDepth) / focusRange);
```

---

## Engine Core (눈에 안 보이는 세상)

### PhysX SDK 빌드

1. Clone PhysX 4.1 SDK: https://github.com/NVIDIAGameWorks/PhysX/tree/4.1
2. Visual Studio 2022 Support
3. Replace `physx/generate_projects.bat`
4. `generate_projects.bat`
5. Replace `physx/buildtools/cmake_generate_projects.py`
6. `cmake_generate_projects.py`
7. Run `generate_projects.bat`
8. Open `physx/compiler/vc17win64/PhysXSDK.sln`

### PhysX SDK 자체 엔진 통합

```cpp
// Integration code for PhysX 4.1

#include <PxPhysicsAPI.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace physx;
using namespace DirectX;

// PhysX 전역
PxDefaultAllocator      gAllocator;
PxDefaultErrorCallback  gErrorCallback;
PxFoundation*           gFoundation = nullptr;
PxPhysics*              gPhysics = nullptr;
PxScene*                gScene = nullptr;
PxMaterial*             gMaterial = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;

// 게임 오브젝트
struct GameObject {
    PxRigidDynamic* rigidBody = nullptr;
    XMMATRIX worldMatrix = XMMatrixIdentity();

    void UpdateFromPhysics() {
        PxTransform t = rigidBody->getGlobalPose();
        PxMat44 mat(t);
        worldMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&mat));
    }
};

std::vector<GameObject> gObjects;

void InitPhysX() {
    gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
    gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale());
    gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0, -9.81f, 0);
    gDispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = gDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    gScene = gPhysics->createScene(sceneDesc);
}

GameObject CreateBox(const PxVec3& pos, const PxVec3& halfExtents) {
    GameObject obj;
    PxTransform pose(pos);
    obj.rigidBody = gPhysics->createRigidDynamic(pose);
    PxShape* shape = gPhysics->createShape(PxBoxGeometry(halfExtents), *gMaterial);
    obj.rigidBody->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*obj.rigidBody, 10.0f);
    gScene->addActor(*obj.rigidBody);
    obj.UpdateFromPhysics();
    return obj;
}

void Simulate(float dt) {
    gScene->simulate(dt);
    gScene->fetchResults(true);
    for (auto& obj : gObjects) obj.UpdateFromPhysics();
}

// 렌더링은 생략 – worldMatrix를 사용해 D3D11에서 월드 행렬로 렌더링

int main() {
    InitPhysX();

    // 박스 생성
    gObjects.push_back(CreateBox(PxVec3(0, 5, 0), PxVec3(1,1,1)));

    // 메인 루프 예시
    while (true) {
        Simulate(1.0f / 60.0f);
        // Render(gObjects[i].worldMatrix); // ← 너의 렌더링 코드에 맞춰 사용
    }

    return 0;
}
```

### Data Structure and Wrapper

| PhysX | Unreal Engine |
|-------|---------------|
| `PxVec3` | `FVector` |
| `PxQuat` | `FQuat` |
| `PxTransform` | `FTransform` |
| `PxRaycastHit` | `FHitResult` |
| `PxOverlapHit` | `FOverlapResult` |
| `PxSweepHit` | `FSweepResult` |
| `PxFilterData` | `FMaskFilter` |
| `PxMaterial` | `UPhysicalMaterial` |
| `PxShape` | `FBodyInstance` |
| `PxRigidActor` | `FBodyInstance` |
| `PxRigidDynamic` | `FBodyInstance` |
| `PxRigidStatic` | `FBodyInstance` |
| `PxJoint` | `FConstraintInstance` |
| `PxScene` | `UWorld->GetPhysicsScene()` |

```cpp
FBodyInstance* Instance = new FBodyInstance(OwnerComp, ...);
PxActor* Body = gPhysics->createRigidDynamic(...);
Body->userData = (void*)Instance;
AActor* Actor = ((BodyInstance*)Body->userData)->OwnerComponent->GetOwner();

struct FBodyInstance
{
    UPrimitiveComponent* OwnerComponent;
};
```

### Event Callback

```cpp
class MySimulationEventCallback : public PxSimulationEventCallback
{
public:
    void onContact(const PxContactPairHeader& pairHeader,
                   const PxContactPair* pairs,
                   PxU32 nbPairs) override
    {
        PxRigidActor* actorA = pairHeader.actors[0];
        PxRigidActor* actorB = pairHeader.actors[1];

        void* dataA = actorA->userData;
        void* dataB = actorB->userData;

        for (PxU32 i = 0; i < nbPairs; ++i)
        {
            const PxContactPair& cp = pairs[i];
            // cp.shapes[0], cp.contactCount, cp.flags, etc...
        }
    }
    virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override {}
    virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override {}
    virtual void onWake(PxActor** actors, PxU32 count) override {}
    virtual void onSleep(PxActor** actors, PxU32 count) override {}
    virtual void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) override {}
};

MySimulationEventCallback* gMyCallback = new MySimulationEventCallback();

PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
sceneDesc.simulationEventCallback = gMyCallback;

gScene = gPhysics->createScene(sceneDesc);
```

### Multithread Support

```cpp
#define SCOPED_READ_LOCK(scene) PxSceneReadLock scopedReadLock(scene);
PxScene*                gScene = nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;

struct GameObject {
    PxRigidDynamic* rigidBody = nullptr;
    XMMATRIX worldMatrix = XMMatrixIdentity();

    void UpdateFromPhysics() {
        // Thread safe
        SCOPED_READ_LOCK(*gScene);
        PxTransform t = rigidBody->getGlobalPose();
        PxMat44 mat(t);
        worldMatrix = XMLoadFloat4x4(reinterpret_cast<const XMFLOAT4X4*>(&mat));
    }
};

// Thread pool
gDispatcher = PxDefaultCpuDispatcherCreate(4); // 4 threads

PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
// ...
sceneDesc.cpuDispatcher = gDispatcher;
// Flags regarding multithread
sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;

gScene = gPhysics->createScene(sceneDesc);

// Loop
void Simulate(float dt) {
    gScene->simulate(dt);
    gScene->fetchResults(true);
    for (auto& obj : gObjects) obj.UpdateFromPhysics();
}
```

### UPhysicsAsset 클래스 구현

```cpp
// UE_5.x/Engine/Source/Developer/PhysicsUtilities/Private/PhysicsAssetUtils.cpp
// UE_5.x/Engine/Source/Runtime/Engine/Classes/PhysicsEngine/SphereElem.h

struct FKAggregateGeom
{
    TArray<FKSphereElem> SphereElems;
    TArray<FKBoxElem> BoxElems;
    TArray<FKSphylElem> SphylElems;
    TArray<FKConvexElem> ConvexElems;
};

class UBodySetupCore : public UObject
{
    FName BoneName;
};

class UBodySetup : public UBodySetupCore
{
    // DisplayName = Primitives
    struct FKAggregateGeom AggGeom;
};

class UPhysicsAsset : public UObject
{
    TArray<UBodySetup*> BodySetup;
};
```

### StaticMesh와 SkeletalMesh에 Physic Body 구현

```cpp
class USkeletalMesh : public USkinnedAsset
{
    UPhysicsAsset* PhysicsAsset;
};

class UStaticMesh : public UStreamableRenderAsset
{
    UBodySetup* BodySetup;
};

class UPrimitiveComponent : public USceneComponent
{
    FBodyInstance BodyInstance;
};

class USkeletalMeshComponent : public USkinnedMeshComponent
{
    /** Array of FBodyInstance objects, storing per-instance state about about each body. */
    TArray<struct FBodyInstance*> Bodies;

    /** Array of FConstraintInstance structs, storing per-instance state about each constraint. */
    TArray<struct FConstraintInstance*> Constraints;
};

// USkeletalMeshComponent::InstantiatePhysicsAssetBodies_Internal
// void UActorComponent::CreatePhysicsState
```

### Cloth Simulation 구현

- NvCloth 1.1.6, CUDA 10.0
- https://nvidiagameworks.github.io/NvCloth/1.1/index.html

---

## 핵심 키워드

- Column-major, Row-major
- getGlobalPose
- Constraint
- Scene
- **Joints**
  - PxFixedJoint
  - PxDistanceJoint
  - PxRevoluteJoint
  - PxPrismaticJoint
  - PxSphericalJoint
  - PxD6Joint
- Degree of Freedom
- Depth of Field
- Physical Material, createMaterial
- NvCloth
- CUDA
- simulate
- Focal
- Aperture (F-Stop)
- Post Process
- PVD
- **Character Controller**
  - PxInitExtentions()

---

## 학습 자료

### PhysX Documentation
- [PhysX 4.1 Guide](https://nvidiagameworks.github.io/PhysX/4.1/documentation/physxguide/Index.html)
- [PhysX Geometry](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html)
- [PxActor API Reference](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/apireference/files/classPxActor.html)
- [PxSimulationEventCallback](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/apireference/files/classPxSimulationEventCallback.html)
- [APEX Destruction Module](https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/apexsdk/APEX_Destruction/Destruction_Module.html)
- [PhysX Visual Debugger](https://developer.nvidia.com/physx-visual-debugger)

### Degrees of Freedom
- [YouTube: Degrees of Freedom](https://www.youtube.com/watch?v=8zbpHu_7FHc&t=1s)
- [Wikipedia: Degrees of Freedom (Mechanics)](https://en.wikipedia.org/wiki/Degrees_of_freedom_(mechanics))

### Depth of Field
- [YouTube: Depth of Field Explained](https://www.youtube.com/watch?v=z29hYlagOYM&t=1s)
- [Wikipedia: Depth of Field](https://en.wikipedia.org/wiki/Depth_of_field)
- [Unreal Engine: Depth of Field](https://dev.epicgames.com/documentation/en-us/unreal-engine/depth-of-field-in-unreal-engine)
- What is F-Stop, How it Works and How to Use it in Photography
- [Wikipedia: F-number](https://en.wikipedia.org/wiki/F-number)
