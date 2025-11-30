#include "pch.h"
#include "PhysicsCore.h"
#include "GlobalConsole.h"

using namespace physx;

FPhysicsCore& FPhysicsCore::Get()
{
    static FPhysicsCore Instance;
    return Instance;
}

FPhysicsCore::~FPhysicsCore()
{
    if (bInitialized)
    {
        Shutdown();
    }
}

void FPhysicsCore::Init()
{
    if (bInitialized)
    {
        UE_LOG("FPhysicsCore: Already initialized");
        return;
    }

    // 1. Foundation 생성
    if (!CreateFoundation())
    {
        UE_LOG("FPhysicsCore: Failed to create Foundation");
        return;
    }

    // 2. PVD 연결 (디버그 빌드)
#ifdef _DEBUG
    ConnectPvd();
#endif

    // 3. Physics 생성
    if (!CreatePhysics())
    {
        UE_LOG("FPhysicsCore: Failed to create Physics");
        Shutdown();
        return;
    }

    // 4. Extensions 초기화
    if (!PxInitExtensions(*GPhysics, GPvd))
    {
        UE_LOG("FPhysicsCore: Failed to initialize Extensions");
        Shutdown();
        return;
    }

    // 5. Vehicle SDK 초기화
    if (!InitVehicleSDK())
    {
        UE_LOG("FPhysicsCore: Failed to initialize Vehicle SDK");
        Shutdown();
        return;
    }

    // 6. Cooking 생성
    if (!CreateCooking())
    {
        UE_LOG("FPhysicsCore: Failed to create Cooking");
        Shutdown();
        return;
    }

    bInitialized = true;
    UE_LOG("FPhysicsCore: Initialization complete");
}

void FPhysicsCore::Shutdown()
{
    if (!GFoundation)
        return;

    // Cooking 해제
    if (GCooking)
    {
        GCooking->release();
        GCooking = nullptr;
    }

    // Vehicle SDK 해제
    ShutdownVehicleSDK();

    // Extensions 종료
    if (GPhysics)
    {
        PxCloseExtensions();
    }

    // Physics 해제
    if (GPhysics)
    {
        GPhysics->release();
        GPhysics = nullptr;
    }

    // PVD 해제 (Physics 다음에 해야 함)
    DisconnectPvd();

    // Foundation 마지막
    if (GFoundation)
    {
        GFoundation->release();
        GFoundation = nullptr;
    }

    bInitialized = false;
    UE_LOG("FPhysicsCore: Shutdown complete");
}

bool FPhysicsCore::CreateFoundation()
{
    GFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, GAllocator, GErrorCallback);
    return GFoundation != nullptr;
}

bool FPhysicsCore::CreatePhysics()
{
    GPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *GFoundation, PxTolerancesScale(), true, GPvd);
    return GPhysics != nullptr;
}

bool FPhysicsCore::CreateCooking()
{
    GCooking = PxCreateCooking(PX_PHYSICS_VERSION, *GFoundation, PxCookingParams(GPhysics->getTolerancesScale()));
    return GCooking != nullptr;
}

bool FPhysicsCore::InitVehicleSDK()
{
    if (!GPhysics)
    {
        return false;
    }

    if (bVehicleSDKInitialized)
    {
        return true;
    }

    if (!PxInitVehicleSDK(*GPhysics))
    {
        return false;
    }

    bVehicleSDKInitialized = true;
    return true;
}

void FPhysicsCore::ShutdownVehicleSDK()
{
    if (!bVehicleSDKInitialized)
    {
        return;
    }

    PxCloseVehicleSDK();
    bVehicleSDKInitialized = false;
}

bool FPhysicsCore::ConnectPvd()
{
    GPvd = PxCreatePvd(*GFoundation);
    if (!GPvd)
        return false;

    GPvdTransport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    if (!GPvdTransport)
    {
        GPvd->release();
        GPvd = nullptr;
        return false;
    }

    bool bConnected = GPvd->connect(*GPvdTransport, PxPvdInstrumentationFlag::eALL);
    if (bConnected)
    {
        UE_LOG("FPhysicsCore: PVD connected (127.0.0.1:5425)");
    }
    else
    {
        UE_LOG("FPhysicsCore: PVD connection failed - Make sure PVD app is running");
    }
    return bConnected;
}

void FPhysicsCore::DisconnectPvd()
{
    if (GPvd)
    {
        if (GPvd->isConnected())
            GPvd->disconnect();
        GPvd->release();
        GPvd = nullptr;
    }

    if (GPvdTransport)
    {
        GPvdTransport->release();
        GPvdTransport = nullptr;
    }
}

PxMaterial* FPhysicsCore::CreateMaterial(float StaticFriction, float DynamicFriction, float Restitution)
{
    if (!GPhysics)
        return nullptr;
    return GPhysics->createMaterial(StaticFriction, DynamicFriction, Restitution);
}
