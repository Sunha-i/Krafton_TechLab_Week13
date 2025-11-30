#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// PhysicsCore.h
// PhysX 전역 객체 관리자 (싱글톤)
// ─────────────────────────────────────────────────────────────────────────────

#include <PxPhysicsAPI.h>

/**
 * @brief PhysX 전역 객체 관리자 (싱글톤)
 *
 * 언리얼 엔진의 FPhysicsCore와 유사한 역할.
 * Foundation, Physics, Cooking을 전역으로 관리.
 * 모든 FPhysScene이 이 객체를 공유.
 */
class FPhysicsCore
{
public:
    // 싱글톤 접근
    static FPhysicsCore& Get();

    // 초기화/종료
    void Init();
    void Shutdown();
    
    // PhysX 객체 접근자
    physx::PxFoundation* GetFoundation() const { return GFoundation; }
    physx::PxPhysics* GetPhysics() const { return GPhysics; }
    physx::PxCooking* GetCooking() const { return GCooking; }
    physx::PxPvd* GetPvd() const { return GPvd; }

    // Material 생성 (전역)
    physx::PxMaterial* CreateMaterial(float StaticFriction, float DynamicFriction, float Restitution);

    bool IsInitialized() const { return bInitialized; }

private:
    FPhysicsCore() = default;
    ~FPhysicsCore();

    // 복사/이동 금지
    FPhysicsCore(const FPhysicsCore&) = delete;
    FPhysicsCore& operator=(const FPhysicsCore&) = delete;

    // 내부 초기화
    bool CreateFoundation();
    bool CreatePhysics();
    bool CreateCooking();
    bool InitVehicleSDK();
    void ShutdownVehicleSDK();
    bool ConnectPvd();
    void DisconnectPvd();

    // PhysX 전역 객체
    physx::PxDefaultAllocator GAllocator;
    physx::PxDefaultErrorCallback GErrorCallback;

    physx::PxFoundation* GFoundation = nullptr;
    physx::PxPhysics* GPhysics = nullptr;
    physx::PxCooking* GCooking = nullptr;
    physx::PxPvd* GPvd = nullptr;
    physx::PxPvdTransport* GPvdTransport = nullptr;

    bool bInitialized = false;
    bool bVehicleSDKInitialized = false;
};
