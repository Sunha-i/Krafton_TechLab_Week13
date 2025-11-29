#include "pch.h"
#include "BodySetup.h"
#include "PhysicsCore.h"
#include "GlobalConsole.h"

#include <PxPhysicsAPI.h>

using namespace physx;

// ═══════════════════════════════════════════════════════════════════════════
// 생성자
// ═══════════════════════════════════════════════════════════════════════════

UBodySetup::UBodySetup()
{
}

// ═══════════════════════════════════════════════════════════════════════════
// Shape 생성
// ═══════════════════════════════════════════════════════════════════════════

PxShape* UBodySetup::CreatePhysicsShape(
    PxRigidActor* RigidActor,
    PxMaterial* DefaultMaterial,
    const FVector& Scale) const
{
    if (!RigidActor || !DefaultMaterial)
    {
        UE_LOG("UBodySetup::CreatePhysicsShape - Invalid actor or material");
        return nullptr;
    }

    // 형상 타입에 따라 생성
    switch (BodyType)
    {
    case EBodySetupType::Box:
        return CreateBoxShape(RigidActor, DefaultMaterial, Scale);

    case EBodySetupType::Sphere:
        return CreateSphereShape(RigidActor, DefaultMaterial, Scale);

    case EBodySetupType::Capsule:
        return CreateCapsuleShape(RigidActor, DefaultMaterial, Scale);

    default:
        UE_LOG("UBodySetup::CreatePhysicsShape - Unknown body type");
        return nullptr;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 형상별 생성 함수
// ═══════════════════════════════════════════════════════════════════════════

PxShape* UBodySetup::CreateBoxShape(
    PxRigidActor* RigidActor,
    PxMaterial* Material,
    const FVector& Scale) const
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
    FVector ScaledExtent = BoxExtent * Scale;

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

PxShape* UBodySetup::CreateSphereShape(
    PxRigidActor* RigidActor,
    PxMaterial* Material,
    const FVector& Scale) const
{
    PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
    if (!Physics)
    {
        return nullptr;
    }

    // 스케일 적용 (균등 스케일 사용 - 가장 큰 축 기준)
    float MaxScale = FMath::Max(Scale.X, FMath::Max(Scale.Y, Scale.Z));
    float ScaledRadius = SphereRadius * MaxScale;

    PxSphereGeometry SphereGeom(ScaledRadius);

    PxShape* Shape = Physics->createShape(SphereGeom, *Material, true);
    if (Shape)
    {
        RigidActor->attachShape(*Shape);
        Shape->release();
    }

    return Shape;
}

PxShape* UBodySetup::CreateCapsuleShape(
    PxRigidActor* RigidActor,
    PxMaterial* Material,
    const FVector& Scale) const
{
    PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
    if (!Physics)
    {
        return nullptr;
    }

    // 스케일 적용
    // 반지름은 XY 평면의 스케일, 높이는 Z 스케일
    float RadiusScale = FMath::Max(Scale.X, Scale.Y);
    float ScaledRadius = SphereRadius * RadiusScale;
    float ScaledHalfHeight = CapsuleHalfHeight * Scale.Z;

    PxCapsuleGeometry CapsuleGeom(ScaledRadius, ScaledHalfHeight);

    PxShape* Shape = Physics->createShape(CapsuleGeom, *Material, true);
    if (Shape)
    {
        // Capsule은 기본적으로 X축 방향으로 누워있음
        // Mundi에서는 Z축 방향(세로)이 기본이므로 회전 필요
        // Local pose: X축 → Z축 회전 (Y축 기준 90도)
        PxTransform LocalPose(PxQuat(PxHalfPi, PxVec3(0, 1, 0)));
        Shape->setLocalPose(LocalPose);

        RigidActor->attachShape(*Shape);
        Shape->release();
    }

    return Shape;
}
