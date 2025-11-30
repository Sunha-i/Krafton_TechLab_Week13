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
// 기본값 상수 정의 (단일 정의 지점)
// ═══════════════════════════════════════════════════════════════════════════

const FVector UBodySetup::DefaultBoxExtent = FVector(0.5f, 0.5f, 0.5f);
const float UBodySetup::DefaultSphereRadius = 0.5f;
const float UBodySetup::DefaultCapsuleRadius = 0.5f;
const float UBodySetup::DefaultCapsuleHalfHeight = 1.0f;

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
// 생성자
// ═══════════════════════════════════════════════════════════════════════════

UBodySetup::UBodySetup()
    : BoxExtent(DefaultBoxExtent)
    , SphereRadius(DefaultSphereRadius)
    , CapsuleHalfHeight(DefaultCapsuleHalfHeight)
{
}
