#include "pch.h"
#include "PhysicsManager.h"
#include "World.h"
#include "GlobalConsole.h"

IMPLEMENT_CLASS(UPhysicsManager)

using namespace physx;

UPhysicsManager::UPhysicsManager()
{
}

UPhysicsManager::~UPhysicsManager()
{
    if (Foundation)
    {
        Shutdown();
    }
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
    // Foundation이 없으면 이미 정리됨 또는 초기화 안됨
    if (!Foundation)
        return;

    // PhysX 샘플 코드 기준 올바른 해제 순서:
    // Scene → Dispatcher → Cooking → Extensions → Physics → PVD → Foundation

    // 1. Scene 해제
    if (Scene)
    {
        Scene->release();
        Scene = nullptr;
    }

    // 2. Dispatcher 해제
    if (Dispatcher)
    {
        Dispatcher->release();
        Dispatcher = nullptr;
    }

    // 3. Cooking 해제
    if (Cooking)
    {
        Cooking->release();
        Cooking = nullptr;
    }

    // 4. Extensions 종료
    if (Physics)
    {
        PxCloseExtensions();
    }

    // 5. Material은 Physics가 관리하므로 별도 해제 불필요
    DefaultMaterial = nullptr;

    // 6. Physics 해제
    if (Physics)
    {
        Physics->release();
        Physics = nullptr;
    }

    // 7. PVD 해제 (Physics 해제 후에 해야 함 - PVD가 Physics에 연결되어 있기 때문)
    DisconnectPVD();

    // 8. Foundation 마지막 해제
    if (Foundation)
    {
        Foundation->release();
        Foundation = nullptr;
    }

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
