#include "pch.h"
#include "BodyInstance.h"
#include "BodySetup.h"
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
    FPhysSceneImpl* Impl = Scene->GetImpl();
    if (!Impl || !Impl->IsInitialized())
    {
        UE_LOG("FBodyInstance::InitBody - PhysScene not initialized");
        CurrentSceneState = EBodyInstanceSceneState::NotAdded;
        return;
    }

    PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
    PxScene* PScene = Impl->GetPxScene();
    PxMaterial* DefaultMaterial = Impl->GetDefaultMaterial();

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
        PxShape* Shape = Setup->CreatePhysicsShape(DynamicActor, DefaultMaterial, Scale);
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

        RigidActorSync = DynamicActor;
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
        PxShape* Shape = Setup->CreatePhysicsShape(StaticActor, DefaultMaterial, Scale);
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

        RigidActorSync = StaticActor;
    }

    // userData 설정 (콜백에서 FBodyInstance로 접근)
    RigidActorSync->userData = this;

    // Scene에 추가
    PScene->addActor(*RigidActorSync);
    CurrentSceneState = EBodyInstanceSceneState::Added;

    UE_LOG("FBodyInstance::InitBody - Body created successfully (Simulate=%s, Trigger=%s)",
        bSimulatePhysics ? "true" : "false",
        bIsTrigger ? "true" : "false");
}

void FBodyInstance::TermBody()
{
    if (!RigidActorSync)
    {
        return;
    }

    CurrentSceneState = EBodyInstanceSceneState::AwaitingRemove;

    // Scene에서 제거
    if (OwnerScene)
    {
        FPhysSceneImpl* Impl = OwnerScene->GetImpl();
        if (Impl && Impl->IsInitialized())
        {
            PxScene* PScene = Impl->GetPxScene();
            if (PScene)
            {
                PScene->removeActor(*RigidActorSync);
            }
        }
    }

    // Actor 해제
    RigidActorSync->release();
    RigidActorSync = nullptr;

    CurrentSceneState = EBodyInstanceSceneState::Removed;
    OwnerComponent = nullptr;
    OwnerScene = nullptr;

    UE_LOG("FBodyInstance::TermBody - Body destroyed");
}

// ═══════════════════════════════════════════════════════════════════════════
// Transform 동기화
// ═══════════════════════════════════════════════════════════════════════════

FTransform FBodyInstance::GetWorldTransform() const
{
    if (!RigidActorSync)
    {
        return FTransform();
    }

    PxTransform PxPose = RigidActorSync->getGlobalPose();
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
    if (!RigidActorSync)
    {
        return;
    }

    PxTransform PxPose = PhysicsConversion::ToPxTransform(NewTransform);

    if (bSimulatePhysics && bTeleport)
    {
        // Dynamic Actor의 경우 setKinematicTarget 또는 setGlobalPose 사용
        PxRigidDynamic* Dynamic = GetPxRigidDynamic();
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
        RigidActorSync->setGlobalPose(PxPose);
    }
}

void FBodyInstance::SyncComponentToPhysics()
{
    if (!OwnerComponent || !RigidActorSync)
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
    if (!OwnerComponent || !RigidActorSync)
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
    if (RigidActorSync)
    {
        UE_LOG("FBodyInstance::SetSimulatePhysics - Runtime change requires re-initialization");
    }
}

void FBodyInstance::AddForce(const FVector& Force, bool bAccelChange)
{
    PxRigidDynamic* Dynamic = GetPxRigidDynamic();
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
    PxRigidDynamic* Dynamic = GetPxRigidDynamic();
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
    PxRigidDynamic* Dynamic = GetPxRigidDynamic();
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
    PxRigidDynamic* Dynamic = GetPxRigidDynamic();
    if (!Dynamic)
    {
        return FVector::Zero();
    }

    PxVec3 Vel = Dynamic->getLinearVelocity();
    return PhysicsConversion::ToFVector(Vel);
}

void FBodyInstance::SetLinearVelocity(const FVector& Velocity, bool bAddToCurrent)
{
    PxRigidDynamic* Dynamic = GetPxRigidDynamic();
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
    PxRigidDynamic* Dynamic = GetPxRigidDynamic();
    if (!Dynamic)
    {
        return FVector::Zero();
    }

    PxVec3 AngVel = Dynamic->getAngularVelocity();
    return PhysicsConversion::ToFVector(AngVel);
}

void FBodyInstance::SetAngularVelocityInRadians(const FVector& AngVel, bool bAddToCurrent)
{
    PxRigidDynamic* Dynamic = GetPxRigidDynamic();
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
    PxRigidDynamic* Dynamic = GetPxRigidDynamic();
    if (!Dynamic)
    {
        return 0.0f;
    }

    return Dynamic->getMass();
}

FVector FBodyInstance::GetBodyInertiaTensor() const
{
    PxRigidDynamic* Dynamic = GetPxRigidDynamic();
    if (!Dynamic)
    {
        return FVector::Zero();
    }

    PxVec3 Inertia = Dynamic->getMassSpaceInertiaTensor();
    return PhysicsConversion::ToFVector(Inertia);
}

// ═══════════════════════════════════════════════════════════════════════════
// 내부 헬퍼
// ═══════════════════════════════════════════════════════════════════════════

PxRigidDynamic* FBodyInstance::GetPxRigidDynamic() const
{
    if (!RigidActorSync)
    {
        return nullptr;
    }

    return RigidActorSync->is<PxRigidDynamic>();
}
