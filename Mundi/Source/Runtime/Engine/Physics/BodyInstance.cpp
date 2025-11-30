#include "pch.h"
#include "BodyInstance.h"
#include "BodyInstanceImpl.h"
#include "BodySetup.h"
#include "BodySetupImpl.h"
#include "PhysScene.h"
#include "PhysSceneImpl.h"
#include "PhysicsCore.h"
#include "PhysicsTypes.h"
#include "PrimitiveComponent.h"
#include "SceneComponent.h"
#include "GlobalConsole.h"

#include <PxPhysicsAPI.h>

using namespace physx;

// ═══════════════════════════════════════════════════════════════════════════
// 생성자/소멸자
// ═══════════════════════════════════════════════════════════════════════════

FBodyInstance::FBodyInstance()
    : Impl(std::make_unique<FBodyInstanceImpl>())
{
}

FBodyInstance::~FBodyInstance()
{
    TermBody();
}

FBodyInstance::FBodyInstance(const FBodyInstance& Other)
    : Impl(std::make_unique<FBodyInstanceImpl>())  // 새 Impl 생성
    , OwnerComponent(nullptr)  // 복사 시 소유자는 nullptr
    , CurrentSceneState(EBodyInstanceSceneState::NotAdded)
    , bSimulatePhysics(Other.bSimulatePhysics)
    , bEnableGravity(Other.bEnableGravity)
    , bIsTrigger(Other.bIsTrigger)
    , MassInKg(Other.MassInKg)
    , LinearDamping(Other.LinearDamping)
    , AngularDamping(Other.AngularDamping)
    , OwnerScene(nullptr)
{
    // PhysX Actor는 복사하지 않음 - 나중에 InitBody()로 새로 생성해야 함
}

FBodyInstance& FBodyInstance::operator=(const FBodyInstance& Other)
{
    if (this != &Other)
    {
        // 기존 PhysX Actor 정리
        TermBody();

        // 물리 파라미터 복사
        bSimulatePhysics = Other.bSimulatePhysics;
        bEnableGravity = Other.bEnableGravity;
        bIsTrigger = Other.bIsTrigger;
        MassInKg = Other.MassInKg;
        LinearDamping = Other.LinearDamping;
        AngularDamping = Other.AngularDamping;

        // 소유자/Scene은 복사하지 않음
        OwnerComponent = nullptr;
        OwnerScene = nullptr;
        CurrentSceneState = EBodyInstanceSceneState::NotAdded;

        // Impl은 이미 존재하므로 재생성하지 않음
        // PhysX Actor는 InitBody()로 새로 생성해야 함
    }
    return *this;
}

// ═══════════════════════════════════════════════════════════════════════════
// 생명주기
// ═══════════════════════════════════════════════════════════════════════════

void FBodyInstance::InitBody(
    UBodySetup* Setup,
    const FTransform& Transform,
    UPrimitiveComponent* Owner,
    FPhysScene* Scene)
{
    // 유효성 검사
    if (!Setup || !Owner || !Scene)
    {
        UE_LOG("FBodyInstance::InitBody - Invalid parameters (Setup=%p, Owner=%p, Scene=%p)",
            Setup, Owner, Scene);
        return;
    }

    // 이미 초기화되었으면 정리
    if (IsInitialized())
    {
        TermBody();
    }

    // 소유자 설정
    OwnerComponent = Owner;
    OwnerScene = Scene;
    CurrentSceneState = EBodyInstanceSceneState::AwaitingAdd;

    // PhysX 객체 가져오기
    FPhysSceneImpl* SceneImpl = Scene->GetImpl();
    if (!SceneImpl || !SceneImpl->IsInitialized())
    {
        UE_LOG("FBodyInstance::InitBody - PhysScene not initialized");
        CurrentSceneState = EBodyInstanceSceneState::NotAdded;
        return;
    }

    PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
    PxScene* PScene = SceneImpl->GetPxScene();
    PxMaterial* DefaultMaterial = SceneImpl->GetDefaultMaterial();

    if (!Physics || !PScene || !DefaultMaterial)
    {
        UE_LOG("FBodyInstance::InitBody - PhysX objects not available");
        CurrentSceneState = EBodyInstanceSceneState::NotAdded;
        return;
    }

    // Transform 변환 (Mundi → PhysX)
    PxTransform PxPose = PhysicsConversion::ToPxTransform(Transform);

    // Actor 생성 (Dynamic vs Static)
    if (bSimulatePhysics)
    {
        // Dynamic Actor 생성
        PxRigidDynamic* DynamicActor = Physics->createRigidDynamic(PxPose);
        if (!DynamicActor)
        {
            UE_LOG("FBodyInstance::InitBody - Failed to create PxRigidDynamic");
            CurrentSceneState = EBodyInstanceSceneState::NotAdded;
            return;
        }

        // Shape 생성 및 부착
        FVector Scale = Transform.Scale3D;
        PxShape* Shape = BodySetupHelper::CreatePhysicsShape(Setup, DynamicActor, DefaultMaterial, Scale);
        if (!Shape)
        {
            DynamicActor->release();
            UE_LOG("FBodyInstance::InitBody - Failed to create shape");
            CurrentSceneState = EBodyInstanceSceneState::NotAdded;
            return;
        }

        // 물리 파라미터 설정
        DynamicActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !bEnableGravity);
        DynamicActor->setLinearDamping(LinearDamping);
        DynamicActor->setAngularDamping(AngularDamping);

        // 질량 설정
        if (MassInKg > 0.0f)
        {
            PxRigidBodyExt::setMassAndUpdateInertia(*DynamicActor, MassInKg);
        }
        else
        {
            // 자동 질량 계산 (밀도 1000 kg/m³)
            PxRigidBodyExt::updateMassAndInertia(*DynamicActor, 1000.0f);
        }

        // 충돌 이벤트 플래그 설정
        Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !bIsTrigger);
        Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, bIsTrigger);

        Impl->RigidActorSync = DynamicActor;
    }
    else
    {
        // Static Actor 생성
        PxRigidStatic* StaticActor = Physics->createRigidStatic(PxPose);
        if (!StaticActor)
        {
            UE_LOG("FBodyInstance::InitBody - Failed to create PxRigidStatic");
            CurrentSceneState = EBodyInstanceSceneState::NotAdded;
            return;
        }

        // Shape 생성 및 부착
        FVector Scale = Transform.Scale3D;
        PxShape* Shape = BodySetupHelper::CreatePhysicsShape(Setup, StaticActor, DefaultMaterial, Scale);
        if (!Shape)
        {
            StaticActor->release();
            UE_LOG("FBodyInstance::InitBody - Failed to create shape");
            CurrentSceneState = EBodyInstanceSceneState::NotAdded;
            return;
        }

        // 트리거 플래그 설정
        Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !bIsTrigger);
        Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, bIsTrigger);

        Impl->RigidActorSync = StaticActor;
    }

    // userData 설정 (콜백에서 FBodyInstance로 접근)
    Impl->RigidActorSync->userData = this;

    // Scene에 추가
    PScene->addActor(*Impl->RigidActorSync);
    CurrentSceneState = EBodyInstanceSceneState::Added;

    UE_LOG("FBodyInstance::InitBody - Body created successfully (Simulate=%s, Trigger=%s)",
        bSimulatePhysics ? "true" : "false",
        bIsTrigger ? "true" : "false");
}

void FBodyInstance::TermBody()
{
    if (!Impl || !Impl->RigidActorSync)
    {
        return;
    }

    CurrentSceneState = EBodyInstanceSceneState::AwaitingRemove;

    // Scene에서 제거
    if (OwnerScene)
    {
        FPhysSceneImpl* SceneImpl = OwnerScene->GetImpl();
        if (SceneImpl && SceneImpl->IsInitialized())
        {
            PxScene* PScene = SceneImpl->GetPxScene();
            if (PScene)
            {
                PScene->removeActor(*Impl->RigidActorSync);
            }
        }
    }

    // Actor 해제
    Impl->RigidActorSync->release();
    Impl->RigidActorSync = nullptr;

    CurrentSceneState = EBodyInstanceSceneState::Removed;
    OwnerComponent = nullptr;
    OwnerScene = nullptr;

    UE_LOG("FBodyInstance::TermBody - Body destroyed");
}

bool FBodyInstance::IsInitialized() const
{
    return Impl && Impl->RigidActorSync != nullptr;
}

// ═══════════════════════════════════════════════════════════════════════════
// Transform 동기화
// ═══════════════════════════════════════════════════════════════════════════

FTransform FBodyInstance::GetWorldTransform() const
{
    if (!Impl || !Impl->RigidActorSync)
    {
        return FTransform();
    }

    PxTransform PxPose = Impl->RigidActorSync->getGlobalPose();
    FTransform Result = PhysicsConversion::ToFTransform(PxPose);

    // 스케일은 Component에서 가져옴
    if (OwnerComponent)
    {
        Result.Scale3D = OwnerComponent->GetWorldScale();
    }

    return Result;
}

void FBodyInstance::SetWorldTransform(const FTransform& NewTransform, bool bTeleport)
{
    if (!Impl || !Impl->RigidActorSync)
    {
        return;
    }

    PxTransform PxPose = PhysicsConversion::ToPxTransform(NewTransform);

    if (bSimulatePhysics && bTeleport)
    {
        // Dynamic Actor의 경우 setKinematicTarget 또는 setGlobalPose 사용
        PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
        if (Dynamic)
        {
            Dynamic->setGlobalPose(PxPose);
            // 텔레포트 시 속도 초기화
            Dynamic->setLinearVelocity(PxVec3(0));
            Dynamic->setAngularVelocity(PxVec3(0));
        }
    }
    else
    {
        Impl->RigidActorSync->setGlobalPose(PxPose);
    }
}

void FBodyInstance::SyncComponentToPhysics()
{
    if (!OwnerComponent || !Impl || !Impl->RigidActorSync)
    {
        return;
    }

    // 시뮬레이션 중이 아닐 때만 Component → PhysX 동기화
    if (!bSimulatePhysics)
    {
        FTransform CompTransform = OwnerComponent->GetWorldTransform();
        SetWorldTransform(CompTransform, false);
    }
}

void FBodyInstance::SyncPhysicsToComponent()
{
    if (!OwnerComponent || !Impl || !Impl->RigidActorSync)
    {
        return;
    }

    // 시뮬레이션 중일 때만 PhysX → Component 동기화
    if (bSimulatePhysics)
    {
        FTransform PhysTransform = GetWorldTransform();
        OwnerComponent->SetWorldTransform(PhysTransform);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 물리 제어
// ═══════════════════════════════════════════════════════════════════════════

void FBodyInstance::SetSimulatePhysics(bool bSimulate)
{
    // 현재 상태와 같으면 무시
    if (bSimulatePhysics == bSimulate)
    {
        return;
    }

    bSimulatePhysics = bSimulate;

    // 이미 초기화된 상태라면 Actor 타입 변경이 필요
    // (Static ↔ Dynamic 변환은 복잡하므로 재생성 권장)
    // 현재 구현에서는 런타임 변경 미지원
    if (Impl && Impl->RigidActorSync)
    {
        UE_LOG("FBodyInstance::SetSimulatePhysics - Runtime change requires re-initialization");
    }
}

void FBodyInstance::AddForce(const FVector& Force, bool bAccelChange)
{
    if (!Impl)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
    if (!Dynamic)
    {
        return;
    }

    PxVec3 PxForce = PhysicsConversion::ToPxVec3(Force);
    PxForceMode::Enum Mode = bAccelChange ? PxForceMode::eACCELERATION : PxForceMode::eFORCE;
    Dynamic->addForce(PxForce, Mode);
}

void FBodyInstance::AddImpulse(const FVector& Impulse, bool bVelChange)
{
    if (!Impl)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
    if (!Dynamic)
    {
        return;
    }

    PxVec3 PxImpulse = PhysicsConversion::ToPxVec3(Impulse);
    PxForceMode::Enum Mode = bVelChange ? PxForceMode::eVELOCITY_CHANGE : PxForceMode::eIMPULSE;
    Dynamic->addForce(PxImpulse, Mode);
}

void FBodyInstance::AddTorqueInRadians(const FVector& Torque, bool bAccelChange)
{
    if (!Impl)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
    if (!Dynamic)
    {
        return;
    }

    PxVec3 PxTorque = PhysicsConversion::ToPxVec3(Torque);
    PxForceMode::Enum Mode = bAccelChange ? PxForceMode::eACCELERATION : PxForceMode::eFORCE;
    Dynamic->addTorque(PxTorque, Mode);
}

// ═══════════════════════════════════════════════════════════════════════════
// 속도
// ═══════════════════════════════════════════════════════════════════════════

FVector FBodyInstance::GetLinearVelocity() const
{
    if (!Impl)
    {
        return FVector::Zero();
    }

    PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
    if (!Dynamic)
    {
        return FVector::Zero();
    }

    PxVec3 Vel = Dynamic->getLinearVelocity();
    return PhysicsConversion::ToFVector(Vel);
}

void FBodyInstance::SetLinearVelocity(const FVector& Velocity, bool bAddToCurrent)
{
    if (!Impl)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
    if (!Dynamic)
    {
        return;
    }

    PxVec3 NewVel = PhysicsConversion::ToPxVec3(Velocity);
    if (bAddToCurrent)
    {
        NewVel += Dynamic->getLinearVelocity();
    }
    Dynamic->setLinearVelocity(NewVel);
}

FVector FBodyInstance::GetAngularVelocityInRadians() const
{
    if (!Impl)
    {
        return FVector::Zero();
    }

    PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
    if (!Dynamic)
    {
        return FVector::Zero();
    }

    PxVec3 AngVel = Dynamic->getAngularVelocity();
    return PhysicsConversion::ToFVector(AngVel);
}

void FBodyInstance::SetAngularVelocityInRadians(const FVector& AngVel, bool bAddToCurrent)
{
    if (!Impl)
    {
        return;
    }

    PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
    if (!Dynamic)
    {
        return;
    }

    PxVec3 NewAngVel = PhysicsConversion::ToPxVec3(AngVel);
    if (bAddToCurrent)
    {
        NewAngVel += Dynamic->getAngularVelocity();
    }
    Dynamic->setAngularVelocity(NewAngVel);
}

// ═══════════════════════════════════════════════════════════════════════════
// 질량/관성
// ═══════════════════════════════════════════════════════════════════════════

float FBodyInstance::GetBodyMass() const
{
    if (!Impl)
    {
        return 0.0f;
    }

    PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
    if (!Dynamic)
    {
        return 0.0f;
    }

    return Dynamic->getMass();
}

FVector FBodyInstance::GetBodyInertiaTensor() const
{
    if (!Impl)
    {
        return FVector::Zero();
    }

    PxRigidDynamic* Dynamic = Impl->GetPxRigidDynamic();
    if (!Dynamic)
    {
        return FVector::Zero();
    }

    PxVec3 Inertia = Dynamic->getMassSpaceInertiaTensor();
    return PhysicsConversion::ToFVector(Inertia);
}

// ═══════════════════════════════════════════════════════════════════════════
// PIMPL 접근자
// ═══════════════════════════════════════════════════════════════════════════

FBodyInstanceImpl* FBodyInstance::GetImpl() const
{
    return Impl.get();
}
