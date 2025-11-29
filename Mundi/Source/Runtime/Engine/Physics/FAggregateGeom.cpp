#include "pch.h"
#include "FAggregateGeom.h"
#include "../Collision/AABB.h"

EAggCollisionShape FShapeElem::StaticShapeType = EAggCollisionShape::Unknown;
EAggCollisionShape FSphylElem::StaticShapeType = EAggCollisionShape::Sphyl;
EAggCollisionShape FSphereElem::StaticShapeType = EAggCollisionShape::Sphere;
EAggCollisionShape FBoxElem::StaticShapeType = EAggCollisionShape::Box;
EAggCollisionShape FConvexElem::StaticShapeType = EAggCollisionShape::Convex;

FShapeElem::~FShapeElem() = default;
FSphylElem::~FSphylElem() = default;
FSphereElem::~FSphereElem() = default;
FBoxElem::~FBoxElem() = default;
FConvexElem::~FConvexElem() = default;

FAABB FSphylElem::CalcAABB(const FTransform& ParentTransform)
{
    // z축 방향으로 세워져 있다고 가정
    FVector HalfHeightVec(0.0f, 0.0f, this->Length * 0.5f);

    // 회전을 적용해서 Top, Bottom 계산
    FVector LocalTop = Center + this->Rotation.RotateVector(HalfHeightVec);
    FVector LocalBottom = Center - this->Rotation.RotateVector(HalfHeightVec);

    FVector WorldTop = ParentTransform.TransformPosition(LocalTop);
    FVector WorldBottom = ParentTransform.TransformPosition(LocalBottom);

    FVector Min, Max;
    Min.X = FMath::Min(WorldTop.X, WorldBottom.X);
    Min.Y = FMath::Min(WorldTop.Y, WorldBottom.Y);
    Min.Z = FMath::Min(WorldTop.Z, WorldBottom.Z);
    Max.X = FMath::Max(WorldTop.X, WorldBottom.X);
    Max.Y = FMath::Max(WorldTop.Y, WorldBottom.Y);
    Max.Z = FMath::Max(WorldTop.Z, WorldBottom.Z);

    float Scale = FMath::Max(FMath::Max(
        FMath::Abs(ParentTransform.Scale3D.X),
         FMath::Abs(ParentTransform.Scale3D.Y)),
         FMath::Abs(ParentTransform.Scale3D.Z));
    float WorldRadius = this->Radius * Scale;

    FVector WorldMin = Min - WorldRadius;
    FVector WorldMax = Max + WorldRadius;

    return FAABB(WorldMin, WorldMax);
}

FAABB FSphereElem::CalcAABB(const FTransform& ParentTransform)
{
    FVector WorldCenter = ParentTransform.TransformPosition(Center);
    
    float Scale = FMath::Max(FMath::Max(
        FMath::Abs(ParentTransform.Scale3D.X),
         FMath::Abs(ParentTransform.Scale3D.Y)),
         FMath::Abs(ParentTransform.Scale3D.Z));
    
    FVector WorldRadius = Radius * Scale;
    FVector WorldMin = WorldCenter - WorldRadius;
    FVector WorldMax = WorldCenter + WorldRadius;

    return FAABB(WorldMin, WorldMax);
}

FAABB FBoxElem::CalcAABB(const FTransform& ParentTransform)
{
    FVector Extent(this->X * 0.5f, this->Y * 0.5f, this->Z * 0.5f);

    FVector LocalVertices[] = {
        FVector(Extent.X, Extent.Y, Extent.Z),
        FVector(Extent.X, Extent.Y, -Extent.Z),
        FVector(Extent.X, -Extent.Y, Extent.Z),
        FVector(Extent.X, -Extent.Y, -Extent.Z),
        FVector(-Extent.X, Extent.Y, Extent.Z),
        FVector(-Extent.X, Extent.Y, -Extent.Z),
        FVector(-Extent.X, -Extent.Y, Extent.Z),
        FVector(-Extent.X, -Extent.Y, -Extent.Z)
    };
    TArray<FVector> WorldVertices;
    WorldVertices.SetNum(8);

    for (int i = 0; i < 8; i++)
    {
        FVector LocalPosition = Center + this->Rotation.RotateVector(LocalVertices[i]);

        WorldVertices[i] = ParentTransform.TransformPosition(LocalPosition);
    }    

    return FAABB(WorldVertices);
}


FAABB FConvexElem::CalcAABB(const FTransform& ParentTransform)
{
    TArray<FVector> Vertices = LocalAABB.GetVertices();
    for (int i = 0; i < Vertices.Num(); i++)
    {
        Vertices[i] = ParentTransform.TransformPosition(Vertices[i]);
    }

    return FAABB(Vertices);
}

FAggregateGeom::~FAggregateGeom()
{
    EmptyElements();
    // FreeRenderInfo()
}

FAABB FAggregateGeom::CalcAABB(const FTransform& Transform)
{
    int32 ElementCount = GetElementCount();
    if (ElementCount <= 0)
    {
        return FAABB();
    }
    
    FAABB TotalBound = FAABB(FVector(FLT_MAX, FLT_MAX, FLT_MAX),
        FVector(-FLT_MAX, -FLT_MAX, -FLT_MAX));

    for (auto& Elem : SphereElems)
    {        
        TotalBound = FAABB::Union(TotalBound, Elem.CalcAABB(Transform));
    }

    for (auto& Elem : BoxElems)
    {        
        TotalBound = FAABB::Union(TotalBound, Elem.CalcAABB(Transform));
    }

    for (auto& Elem : SphylElems)
    {        
        TotalBound = FAABB::Union(TotalBound, Elem.CalcAABB(Transform));
    }

    for (auto& Elem : ConvexElems)
    {        
        TotalBound = FAABB::Union(TotalBound, Elem.CalcAABB(Transform));
    }

    return TotalBound;
}
