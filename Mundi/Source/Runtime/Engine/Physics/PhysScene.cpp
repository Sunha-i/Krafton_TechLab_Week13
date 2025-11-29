#include "pch.h"
#include "PhysScene.h"
#include "PhysSceneImpl.h"
#include "PhysicsCore.h"
#include "PhysicsTypes.h"
#include "PhysicsEventCallback.h"
#include "BodyInstance.h"
#include "World.h"
#include "GlobalConsole.h"

using namespace physx;

// ═══════════════════════════════════════════════════════════════════════════════
// 충돌 이벤트를 받기 위한 커스텀 필터 셰이더
// ═══════════════════════════════════════════════════════════════════════════════
static PxFilterFlags MundiFilterShader(
    PxFilterObjectAttributes Attributes0,
    PxFilterData FilterData0,
    PxFilterObjectAttributes Attributes1,
    PxFilterData FilterData1,
    PxPairFlags& PairFlags,
    const void* ConstantBlock,
    PxU32 ConstantBlockSize)
{
    // 트리거 처리
    if (PxFilterObjectIsTrigger(Attributes0) || PxFilterObjectIsTrigger(Attributes1))
    {
        PairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }

    // 기본 충돌 처리 + 이벤트 알림
    PairFlags = PxPairFlag::eCONTACT_DEFAULT
              | PxPairFlag::eNOTIFY_TOUCH_FOUND
              | PxPairFlag::eNOTIFY_TOUCH_PERSISTS
              | PxPairFlag::eNOTIFY_TOUCH_LOST
              | PxPairFlag::eNOTIFY_CONTACT_POINTS;

    return PxFilterFlag::eDEFAULT;
}

// ═══════════════════════════════════════════════════════════════════════════════
// FPhysScene - 공개 래퍼
// ═══════════════════════════════════════════════════════════════════════════════

FPhysScene::FPhysScene()
    : Impl(std::make_unique<FPhysSceneImpl>())
{
}

FPhysScene::~FPhysScene()
{
    TermPhysScene();
}

FPhysScene::FPhysScene(FPhysScene&&) noexcept = default;
FPhysScene& FPhysScene::operator=(FPhysScene&&) noexcept = default;

void FPhysScene::InitPhysScene(UWorld* InOwningWorld)
{
    OwningWorld = InOwningWorld;

    if (!FPhysicsCore::Get().IsInitialized())
    {
        UE_LOG("FPhysScene: FPhysicsCore not initialized");
        return;
    }

    if (!Impl->Initialize(this, InOwningWorld))
    {
        UE_LOG("FPhysScene: Failed to initialize");
        return;
    }

    UE_LOG("FPhysScene: Initialization complete");
}

void FPhysScene::TermPhysScene()
{
    if (Impl)
    {
        Impl->Terminate();
    }
    OwningWorld = nullptr;
}

bool FPhysScene::IsInitialized() const
{
    return Impl && Impl->IsInitialized();
}

void FPhysScene::StartFrame()
{
    if (Impl)
    {
        Impl->StartFrame();
    }
}

void FPhysScene::Tick(float DeltaSeconds)
{
    if (Impl)
    {
        Impl->Simulate(DeltaSeconds);
    }
}

void FPhysScene::EndFrame()
{
    if (Impl)
    {
        Impl->FetchResults();
    }
}

bool FPhysScene::IsSimulating() const
{
    return Impl && Impl->IsSimulating();
}

void FPhysScene::SetGravity(const FVector& InGravity)
{
    if (Impl)
    {
        // Mundi 좌표계 → PhysX 좌표계 변환
        Impl->SetGravity(PhysicsConversion::ToPxVec3(InGravity));
    }
}

FVector FPhysScene::GetGravity() const
{
    if (Impl)
    {
        // PhysX 좌표계 → Mundi 좌표계 변환
        return PhysicsConversion::ToFVector(Impl->GetGravity());
    }
    return FVector(0, 0, -981.0f);  // 기본 중력 (cm/s²)
}

UWorld* FPhysScene::GetOwningWorld() const
{
    return OwningWorld;
}

FPhysScene::FPhysSceneStats FPhysScene::GetStats() const
{
    FPhysSceneStats Stats;
    if (Impl)
    {
        Stats.NumDynamicActors = Impl->GetNumActors(PxActorTypeFlag::eRIGID_DYNAMIC);
        Stats.NumStaticActors = Impl->GetNumActors(PxActorTypeFlag::eRIGID_STATIC);
        Stats.NumActiveActors = Stats.NumDynamicActors;  // TODO: 실제 active 수 계산
    }
    return Stats;
}

// ═══════════════════════════════════════════════════════════════════════════════
// FPhysSceneImpl - PhysX 구현부
// ═══════════════════════════════════════════════════════════════════════════════

FPhysSceneImpl::FPhysSceneImpl()
{
}

FPhysSceneImpl::~FPhysSceneImpl()
{
    Terminate();
}

bool FPhysSceneImpl::Initialize(FPhysScene* InOwnerScene, UWorld* InOwningWorld)
{
    if (bInitialized)
        return true;

    OwnerScene = InOwnerScene;

    if (!CreateScene(InOwningWorld))
        return false;

    // 기본 Material 생성
    DefaultMaterial = FPhysicsCore::Get().CreateMaterial(
        DefaultStaticFriction, DefaultDynamicFriction, DefaultRestitution);

    // 테스트용 Actor 생성 (PVD 확인용)
    // CreateTestActors();

    bInitialized = true;
    return true;
}

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

    // EventCallback 해제
    if (EventCallback)
    {
        delete EventCallback;
        EventCallback = nullptr;
    }

    DefaultMaterial = nullptr;
    bInitialized = false;

    UE_LOG("FPhysSceneImpl: Terminated");
}

bool FPhysSceneImpl::CreateScene(UWorld* InOwningWorld)
{
    FPhysicsCore& Core = FPhysicsCore::Get();
    PxPhysics* Physics = Core.GetPhysics();

    PxSceneDesc SceneDesc(Physics->getTolerancesScale());

    // 중력 설정 (PhysX Y-Up 좌표계)
    SceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);

    // CPU Dispatcher 생성
    CpuDispatcher = PxDefaultCpuDispatcherCreate(NumPhysxThreads);
    SceneDesc.cpuDispatcher = CpuDispatcher;

    // 필터 셰이더 - 커스텀 셰이더로 충돌 이벤트 활성화
    SceneDesc.filterShader = MundiFilterShader;

    // EventCallback 생성 및 등록
    EventCallback = new FPhysicsEventCallback(OwnerScene);
    SceneDesc.simulationEventCallback = EventCallback;

    // Scene 플래그
    SceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;  // 활성 Actor 추적
    SceneDesc.flags |= PxSceneFlag::eENABLE_CCD;            // 연속 충돌 감지
    SceneDesc.flags |= PxSceneFlag::eENABLE_PCM;            // 지속적 접촉 다중점

    PScene = Physics->createScene(SceneDesc);
    if (!PScene)
    {
        delete EventCallback;
        EventCallback = nullptr;
        return false;
    }

    UE_LOG("FPhysSceneImpl: EventCallback 등록 완료");

    // PVD 연결
    PxPvd* Pvd = Core.GetPvd();
    if (Pvd && Pvd->isConnected())
    {
        PxPvdSceneClient* PvdClient = PScene->getScenePvdClient();
        if (PvdClient)
        {
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
            PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
        }
    }

    return true;
}

void FPhysSceneImpl::StartFrame()
{
    // Pre-simulation 작업 (추후 확장)
}

void FPhysSceneImpl::Simulate(float DeltaSeconds)
{
    if (!PScene)
        return;

    // 시간 제한
    DeltaSeconds = FMath::Clamp(DeltaSeconds, 1.0f / 240.0f, 1.0f / 30.0f);

    bIsSimulating = true;
    PScene->simulate(DeltaSeconds);
}

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

void FPhysSceneImpl::SetGravity(const PxVec3& InGravity)
{
    if (PScene)
    {
        PScene->setGravity(InGravity);
    }
}

PxVec3 FPhysSceneImpl::GetGravity() const
{
    if (PScene)
    {
        return PScene->getGravity();
    }
    return PxVec3(0.0f, -9.81f, 0.0f);
}

uint32 FPhysSceneImpl::GetNumActors(PxActorTypeFlags Types) const
{
    if (PScene)
    {
        return PScene->getNbActors(Types);
    }
    return 0;
}

PxRigidDynamic* FPhysSceneImpl::CreateDynamicBox(
    PxPhysics* Physics,
    const PxTransform& Pose,
    float HalfExtent,
    const char* DebugName)
{
    PxRigidDynamic* Box = Physics->createRigidDynamic(Pose);

    // Box 형상 생성
    PxBoxGeometry BoxGeometry(HalfExtent, HalfExtent, HalfExtent);
    PxShape* BoxShape = Physics->createShape(BoxGeometry, *DefaultMaterial);
    Box->attachShape(*BoxShape);
    BoxShape->release();

    // 질량 및 관성 자동 계산 (밀도 10 kg/m³)
    PxRigidBodyExt::updateMassAndInertia(*Box, 10.0f);

    PScene->addActor(*Box);

    if (DebugName)
    {
        UE_LOG("  Created: %s", DebugName);
    }

    return Box;
}

void FPhysSceneImpl::CreateTestActors()
{
    FPhysicsCore& Core = FPhysicsCore::Get();
    PxPhysics* Physics = Core.GetPhysics();

    UE_LOG("═══════════════════════════════════════════════════════════════════════");
    UE_LOG("FPhysSceneImpl: 좌표계 변환 테스트 시작");
    UE_LOG("═══════════════════════════════════════════════════════════════════════");

    // ═══════════════════════════════════════════════════════════════════════
    // 1. Ground Plane (정적 Actor)
    // ═══════════════════════════════════════════════════════════════════════
    // PhysX Y-Up 좌표계에서 Y축 위 방향의 평면 생성
    PxRigidStatic* GroundPlane = PxCreatePlane(
        *Physics,
        PxPlane(0, 1, 0, 0),  // Y-Up 평면 (원점에서 Y=0)
        *DefaultMaterial
    );
    PScene->addActor(*GroundPlane);
    UE_LOG("  Created: Ground Plane (PhysX Y=0)");

    // ═══════════════════════════════════════════════════════════════════════
    // 2. ToPxVec3 테스트 - 위치 변환
    // ═══════════════════════════════════════════════════════════════════════
    UE_LOG("───────────────────────────────────────────────────────────────────────");
    UE_LOG("ToPxVec3 테스트 (위치 변환)");
    UE_LOG("  변환 규칙: Mundi(X,Y,Z) → PhysX(Y, Z, -X)");
    UE_LOG("───────────────────────────────────────────────────────────────────────");

    // Box A: Mundi 기준 Forward(X=5) + Up(Z=3)
    // 예상 PhysX 위치: (0, 3, -5) - X=Right, Y=Up, Z=Back (Forward=-Z)
    {
        FVector MundiPos(5.0f, 0.0f, 3.0f);  // Forward 5m, Up 3m
        PxVec3 PhysXPos = PhysicsConversion::ToPxVec3(MundiPos);

        UE_LOG("Box A: Mundi(%.1f, %.1f, %.1f) → PhysX(%.1f, %.1f, %.1f)",
            MundiPos.X, MundiPos.Y, MundiPos.Z,
            PhysXPos.x, PhysXPos.y, PhysXPos.z);

        CreateDynamicBox(Physics, PxTransform(PhysXPos), 0.5f, "Box A - Forward+Up");
    }

    // Box B: Mundi 기준 Right(Y=5) + Up(Z=3)
    // 예상 PhysX 위치: (5, 3, 0)
    {
        FVector MundiPos(0.0f, 5.0f, 3.0f);  // Right 5m, Up 3m
        PxVec3 PhysXPos = PhysicsConversion::ToPxVec3(MundiPos);

        UE_LOG("Box B: Mundi(%.1f, %.1f, %.1f) → PhysX(%.1f, %.1f, %.1f)",
            MundiPos.X, MundiPos.Y, MundiPos.Z,
            PhysXPos.x, PhysXPos.y, PhysXPos.z);

        CreateDynamicBox(Physics, PxTransform(PhysXPos), 0.5f, "Box B - Right+Up");
    }

    // Box C: Mundi 기준 Up(Z=8)만
    // 예상 PhysX 위치: (0, 8, 0)
    {
        FVector MundiPos(0.0f, 0.0f, 8.0f);  // Up 8m
        PxVec3 PhysXPos = PhysicsConversion::ToPxVec3(MundiPos);

        UE_LOG("Box C: Mundi(%.1f, %.1f, %.1f) → PhysX(%.1f, %.1f, %.1f)",
            MundiPos.X, MundiPos.Y, MundiPos.Z,
            PhysXPos.x, PhysXPos.y, PhysXPos.z);

        CreateDynamicBox(Physics, PxTransform(PhysXPos), 0.5f, "Box C - Up Only (highest)");
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 3. ToPxQuat 테스트 - 회전 변환
    // ═══════════════════════════════════════════════════════════════════════
    UE_LOG("───────────────────────────────────────────────────────────────────────");
    UE_LOG("ToPxQuat 테스트 (회전 변환)");
    UE_LOG("  변환 규칙: 축 재배치 + Handedness 반전");
    UE_LOG("───────────────────────────────────────────────────────────────────────");

    // Box D: Mundi Z축 기준 45도 회전
    // Mundi에서 Z축 = Up축, PhysX에서는 Y축 = Up축
    {
        FVector MundiPos(-3.0f, 0.0f, 3.0f);  // Forward 방향 반대쪽
        float AngleDeg = 45.0f;
        float AngleRad = AngleDeg * 3.14159265f / 180.0f;

        // Z축 기준 45도 회전 쿼터니언 (Mundi 좌표계)
        FQuat MundiRot = FQuat::FromAxisAngle(FVector(0, 0, 1), AngleRad);
        PxQuat PhysXRot = PhysicsConversion::ToPxQuat(MundiRot);
        PxVec3 PhysXPos = PhysicsConversion::ToPxVec3(MundiPos);

        UE_LOG("Box D: Mundi Z축 %.0f도 회전", AngleDeg);
        UE_LOG("  Mundi Quat: (%.3f, %.3f, %.3f, %.3f)",
            MundiRot.X, MundiRot.Y, MundiRot.Z, MundiRot.W);
        UE_LOG("  PhysX Quat: (%.3f, %.3f, %.3f, %.3f)",
            PhysXRot.x, PhysXRot.y, PhysXRot.z, PhysXRot.w);

        CreateDynamicBox(Physics, PxTransform(PhysXPos, PhysXRot), 0.5f, "Box D - Z-Rot 45deg");
    }

    // Box E: Mundi X축 기준 30도 회전
    // Mundi에서 X축 = Forward축, PhysX에서는 Z축 = Forward축
    {
        FVector MundiPos(3.0f, 3.0f, 3.0f);  // 대각선 위치
        float AngleDeg = 30.0f;
        float AngleRad = AngleDeg * 3.14159265f / 180.0f;

        // X축 기준 30도 회전 쿼터니언 (Mundi 좌표계)
        FQuat MundiRot = FQuat::FromAxisAngle(FVector(1, 0, 0), AngleRad);
        PxQuat PhysXRot = PhysicsConversion::ToPxQuat(MundiRot);
        PxVec3 PhysXPos = PhysicsConversion::ToPxVec3(MundiPos);

        UE_LOG("Box E: Mundi X축 %.0f도 회전", AngleDeg);
        UE_LOG("  Mundi Quat: (%.3f, %.3f, %.3f, %.3f)",
            MundiRot.X, MundiRot.Y, MundiRot.Z, MundiRot.W);
        UE_LOG("  PhysX Quat: (%.3f, %.3f, %.3f, %.3f)",
            PhysXRot.x, PhysXRot.y, PhysXRot.z, PhysXRot.w);

        CreateDynamicBox(Physics, PxTransform(PhysXPos, PhysXRot), 0.5f, "Box E - X-Rot 30deg");
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 4. ToPxTransform 테스트 - 전체 Transform 변환
    // ═══════════════════════════════════════════════════════════════════════
    UE_LOG("───────────────────────────────────────────────────────────────────────");
    UE_LOG("ToPxTransform 테스트 (전체 Transform 변환)");
    UE_LOG("───────────────────────────────────────────────────────────────────────");

    // Box F: FTransform으로 위치+회전 동시 변환
    {
        FVector MundiPos(0.0f, -3.0f, 5.0f);  // Left 방향 + Up
        float AngleDeg = 60.0f;
        float AngleRad = AngleDeg * 3.14159265f / 180.0f;
        FQuat MundiRot = FQuat::FromAxisAngle(FVector(0, 1, 0), AngleRad);  // Y축(Right) 기준 회전

        FTransform MundiTransform(MundiPos, MundiRot, FVector::One());
        PxTransform PhysXTransform = PhysicsConversion::ToPxTransform(MundiTransform);

        UE_LOG("Box F: FTransform 변환 테스트");
        UE_LOG("  Mundi Pos: (%.1f, %.1f, %.1f)", MundiPos.X, MundiPos.Y, MundiPos.Z);
        UE_LOG("  Mundi Rot (Y축 %.0f도): (%.3f, %.3f, %.3f, %.3f)",
            AngleDeg, MundiRot.X, MundiRot.Y, MundiRot.Z, MundiRot.W);
        UE_LOG("  PhysX Pos: (%.1f, %.1f, %.1f)",
            PhysXTransform.p.x, PhysXTransform.p.y, PhysXTransform.p.z);
        UE_LOG("  PhysX Rot: (%.3f, %.3f, %.3f, %.3f)",
            PhysXTransform.q.x, PhysXTransform.q.y, PhysXTransform.q.z, PhysXTransform.q.w);

        CreateDynamicBox(Physics, PhysXTransform, 0.5f, "Box F - Full Transform");
    }

    UE_LOG("═══════════════════════════════════════════════════════════════════════");
    UE_LOG("FPhysSceneImpl: 좌표계 변환 테스트 완료 (6개 박스 생성)");
    UE_LOG("═══════════════════════════════════════════════════════════════════════");
    UE_LOG("PVD에서 확인 (PhysX: Right-Handed, Forward=-Z):");
    UE_LOG("  - Box A: PhysX Z- 방향 (Forward), Z=-5");
    UE_LOG("  - Box B: PhysX X+ 방향 (Right), X=5");
    UE_LOG("  - Box C: 가장 높이 위치 (Y=8)");
    UE_LOG("  - Box D: PhysX Y축 기준 45도 회전 (Up축)");
    UE_LOG("  - Box E: PhysX Z축 기준 30도 회전 (Back축)");
    UE_LOG("  - Box F: 복합 Transform 변환");
}
