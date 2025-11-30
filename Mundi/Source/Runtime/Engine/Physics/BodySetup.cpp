#include "pch.h"
#include "BodySetup.h"
#include "PhysicsCore.h"
#include "GlobalConsole.h"
#include "AggregateGeom.h"

#include <PxPhysicsAPI.h>

#include "PhysicsTypes.h"
#include "Shape/ConvexElem.h"

using namespace physx;

// ═══════════════════════════════════════════════════════════════════════════
// 생성자
// ═══════════════════════════════════════════════════════════════════════════

UBodySetup::UBodySetup()
{
}

int32 UBodySetup::AddShapesToRigidActor(physx::PxRigidActor* RigidActor, physx::PxMaterial* DefaultMaterial, const FVector& Scale) const
{
    if (!RigidActor)
    {
        return 0;
    }

    int32 ShapeCount = 0;

    AddBoxElems(RigidActor, DefaultMaterial, Scale);
    AddSphereElems(RigidActor, DefaultMaterial, Scale);
    AddSphylElems(RigidActor, DefaultMaterial, Scale);
    AddConvexElems(RigidActor, DefaultMaterial, Scale);

    return static_cast<int32>(RigidActor->getNbShapes());
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

void UBodySetup::AddBoxElems(physx::PxRigidActor* RigidActor, physx::PxMaterial* DefaultMaterial, const FVector& Scale) const
{
    PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
    for (const auto& BoxElem : AggGeom.BoxElems)
    {
        float HalfExtentX = (BoxElem.X * 0.5f) * FMath::Abs(Scale.X);
        float HalfExtentY = (BoxElem.Y * 0.5f) * FMath::Abs(Scale.Y);
        float HalfExtentZ = (BoxElem.Z * 0.5f) * FMath::Abs(Scale.Z);

        PxBoxGeometry BoxGeom(HalfExtentX, HalfExtentY, HalfExtentZ);

        PxShape* Shape = Physics->createShape(BoxGeom, *DefaultMaterial);
        if (Shape)
        {
            FTransform BoxTransform(BoxElem.Center, BoxElem.Rotation, FVector::One());
            Shape->setLocalPose(PhysicsConversion::ToPxTransform(BoxTransform));

            RigidActor->attachShape(*Shape);
            // 액터가 Shape의 소유권을 가짐
            Shape->release();
        }
    }
}

void UBodySetup::AddSphereElems(physx::PxRigidActor* RigidActor, physx::PxMaterial* DefaultMaterial, const FVector& Scale) const
{
    PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
    for (const auto& SphereElem : AggGeom.SphereElems)
    {
        float MaxScale = Scale.GetMaxValue();
        float Radius = SphereElem.Radius * MaxScale;

        if (Radius <= 0.0f)
        {
            continue;
        }

        PxSphereGeometry SphereGeom(Radius);

        PxShape* Shape = Physics->createShape(SphereGeom, *DefaultMaterial);
        if (Shape)
        {
            PxTransform LocalPose(PhysicsConversion::ToPxVec3(SphereElem.Center));
            Shape->setLocalPose(LocalPose);

            RigidActor->attachShape(*Shape);
            Shape->release();
        }
    }
}

void UBodySetup::AddSphylElems(physx::PxRigidActor* RigidActor, physx::PxMaterial* DefaultMaterial, const FVector& Scale) const
{
    PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
    for (const auto& SphylElem : AggGeom.SphylElems)
    {
        float MaxScale = Scale.GetMaxValue();
        float Radius = SphylElem.Radius * MaxScale;
        float ScaledHalfLength = SphylElem.Length * 0.5f * MaxScale;

        if (Radius <= 0.0f)
        {
            continue;
        }

        PxCapsuleGeometry CapsuleGeom(Radius, ScaledHalfLength);
        
        PxShape* Shape = Physics->createShape(CapsuleGeom, *DefaultMaterial);
        if (Shape)
        {
            FTransform SphylTransform(SphylElem.Center, SphylElem.Rotation, FVector::One());
            Shape->setLocalPose(PhysicsConversion::ToPxTransform(SphylTransform));

            RigidActor->attachShape(*Shape);
            Shape->release();
        }        
    }
}

void UBodySetup::AddConvexElems(physx::PxRigidActor* RigidActor, physx::PxMaterial* DefaultMaterial, const FVector& Scale) const
{
    physx::PxPhysics* Physics = FPhysicsCore::Get().GetPhysics();
    physx::PxCooking* Cooking = FPhysicsCore::Get().GetCooking();
    if (!Physics || !Cooking)
    {
        return;
    }

    for (const auto& ConvexElem : AggGeom.ConvexElems)
    {
        physx::PxConvexMesh* TargetMesh = ConvexElem.ConvexMesh;
        // 캐싱이 없으면 쿠킹 실행
        if (!TargetMesh)
        {
            if (ConvexElem.VertexData.IsEmpty())
            {
                continue;
            }

            TArray<physx::PxVec3> PxVertices;
            PxVertices.Reserve(ConvexElem.VertexData.Num());
            // FVector -> PxVec3
            for (const auto& Vertex : ConvexElem.VertexData)
            {
                PxVertices.Add(PhysicsConversion::ToPxVec3(Vertex));
            }

            // Convex Mesh Desciptor 설정
            physx::PxConvexMeshDesc ConvexDesc;
            ConvexDesc.points.count = PxVertices.Num();
            ConvexDesc.points.stride = sizeof(physx::PxVec3);
            ConvexDesc.points.data = PxVertices.GetData();

            // 정점만 넣고 면은 PhysX가 담당
            ConvexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

            // 쿠킹
            physx::PxDefaultMemoryOutputStream Ostream;
            physx::PxConvexMeshCookingResult::Enum Result;
            if (!Cooking->cookConvexMesh(ConvexDesc, Ostream, &Result))
            {
                UE_LOG("[UBodySetup/AddConvexElems] fail convex cooking");
                continue;
            }

            // PxConvexMesh 생성
            physx::PxDefaultMemoryInputData Input(Ostream.getData(), Ostream.getSize());
            TargetMesh = Physics->createConvexMesh(Input);

            // 캐싱
            if (TargetMesh)
            {
                ConvexElem.ConvexMesh = TargetMesh;
            }
        }

        if (TargetMesh)
        {
            physx::PxMeshScale PxScale(
                PhysicsConversion::ToPxVec3(Scale),
                physx::PxQuat(physx::PxIdentity)
            );

            PxConvexMeshGeometry ConvexGeom(TargetMesh, PxScale);
            PxShape* Shape = Physics->createShape(ConvexGeom, *DefaultMaterial);
            if (Shape)
            {
                Shape->setLocalPose(PhysicsConversion::ToPxTransform(ConvexElem.Transform));

                RigidActor->attachShape(*Shape);
                Shape->release();
            }
        }        
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
