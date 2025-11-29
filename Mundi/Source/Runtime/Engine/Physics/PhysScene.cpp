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
