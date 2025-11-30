#include "pch.h"
#include "BodySetupImpl.h"
#include "BodySetup.h"
#include "PhysicsCore.h"
#include "GlobalConsole.h"

using namespace physx;

// ═══════════════════════════════════════════════════════════════════════════
// 내부 헬퍼 함수
// ═══════════════════════════════════════════════════════════════════════════

namespace
{
    /**
     * @brief Box 형상 생성
     */
    PxShape* CreateBoxShape(
        const UBodySetup* Setup,
        PxRigidActor* RigidActor,
        PxMaterial* Material,
        const FVector& Scale)
    {
        PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
        if (!Physics)
        {
            return nullptr;
        }

        // 스케일 적용된 Half Extent 계산
        // Mundi 좌표계의 BoxExtent를 PhysX 좌표계로 변환
        // Mundi: X=Forward, Y=Right, Z=Up
        // PhysX: X=Right, Y=Up, Z=Back
        FVector ScaledExtent = Setup->BoxExtent * Scale;

        // PhysX Box는 half extents 사용
        // 좌표계 변환: Mundi(X,Y,Z) → PhysX(Y, Z, X)
        PxBoxGeometry BoxGeom(
            ScaledExtent.Y,  // PhysX X (Right) = Mundi Y
            ScaledExtent.Z,  // PhysX Y (Up) = Mundi Z
            ScaledExtent.X   // PhysX Z (Back) = Mundi X (Forward)
        );

        // Shape 생성 및 Actor에 부착
        PxShape* Shape = Physics->createShape(BoxGeom, *Material, true);
        if (Shape)
        {
            RigidActor->attachShape(*Shape);
            Shape->release();  // Actor가 소유권 가짐
        }

        return Shape;
    }

    /**
     * @brief Sphere 형상 생성
     */
    PxShape* CreateSphereShape(
        const UBodySetup* Setup,
        PxRigidActor* RigidActor,
        PxMaterial* Material,
        const FVector& Scale)
    {
        PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
        if (!Physics)
        {
            return nullptr;
        }

        // 스케일 적용 (균등 스케일 사용 - 가장 큰 축 기준)
        float MaxScale = FMath::Max(Scale.X, FMath::Max(Scale.Y, Scale.Z));
        float ScaledRadius = Setup->SphereRadius * MaxScale;

        PxSphereGeometry SphereGeom(ScaledRadius);

        PxShape* Shape = Physics->createShape(SphereGeom, *Material, true);
        if (Shape)
        {
            RigidActor->attachShape(*Shape);
            Shape->release();
        }

        return Shape;
    }

    /**
     * @brief Capsule 형상 생성
     */
    PxShape* CreateCapsuleShape(
        const UBodySetup* Setup,
        PxRigidActor* RigidActor,
        PxMaterial* Material,
        const FVector& Scale)
    {
        PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
        if (!Physics)
        {
            return nullptr;
        }

        // 스케일 적용
        // 반지름은 XY 평면의 스케일, 높이는 Z 스케일
        float RadiusScale = FMath::Max(Scale.X, Scale.Y);
        float ScaledRadius = Setup->SphereRadius * RadiusScale;
        float ScaledHalfHeight = Setup->CapsuleHalfHeight * Scale.Z;

        PxCapsuleGeometry CapsuleGeom(ScaledRadius, ScaledHalfHeight);

        PxShape* Shape = Physics->createShape(CapsuleGeom, *Material, true);
        if (Shape)
        {
            // PhysX Capsule은 기본적으로 PhysX X축 방향으로 누워있음
            // Mundi Z축(Up) = PhysX Y축이므로, PhysX Y축 방향으로 세워야 함
            // PhysX Z축 기준 90도 회전: PhysX X축 → PhysX Y축 (= Mundi Z축)
            PxTransform LocalPose(PxQuat(PxHalfPi, PxVec3(0, 0, 1)));
            Shape->setLocalPose(LocalPose);

            RigidActor->attachShape(*Shape);
            Shape->release();
        }

        return Shape;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// BodySetupHelper 구현
// ═══════════════════════════════════════════════════════════════════════════

namespace BodySetupHelper
{
    PxShape* CreatePhysicsShape(
        const UBodySetup* Setup,
        PxRigidActor* RigidActor,
        PxMaterial* DefaultMaterial,
        const FVector& Scale)
    {
        if (!Setup || !RigidActor || !DefaultMaterial)
        {
            UE_LOG("BodySetupHelper::CreatePhysicsShape - Invalid parameters");
            return nullptr;
        }

        // 형상 타입에 따라 생성
        switch (Setup->BodyType)
        {
        case EBodySetupType::Box:
            return CreateBoxShape(Setup, RigidActor, DefaultMaterial, Scale);

        case EBodySetupType::Sphere:
            return CreateSphereShape(Setup, RigidActor, DefaultMaterial, Scale);

        case EBodySetupType::Capsule:
            return CreateCapsuleShape(Setup, RigidActor, DefaultMaterial, Scale);

        default:
            UE_LOG("BodySetupHelper::CreatePhysicsShape - Unknown body type");
            return nullptr;
        }
    }
}
