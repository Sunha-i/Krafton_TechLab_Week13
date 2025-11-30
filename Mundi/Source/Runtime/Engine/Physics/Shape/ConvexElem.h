#pragma once
#include <geometry/PxConvexMesh.h>

#include "ShapeElem.h"

class PxConvexMesh;

struct FConvexElem : public FShapeElem
{
    FConvexElem()
        : FShapeElem(EAggCollisionShape::Convex),
          VertexData(),
          IndexData(),
          LocalAABB(FAABB()),
          ConvexMesh(nullptr)
    {}
    FConvexElem(const FConvexElem& Other)
        : FShapeElem(Other),
          VertexData(Other.VertexData),
          IndexData(Other.IndexData),
          LocalAABB(Other.LocalAABB),
          ConvexMesh(Other.ConvexMesh)
    {
        if (ConvexMesh)
        {
            ConvexMesh->acquireReference();
        }
    }
    virtual ~FConvexElem() override;

    FTransform GetTransform() const override
    {
        return Transform;
    }

    void SetTransform(const FTransform& InTransform)
    {
        Transform = InTransform;
    }
    
    FAABB CalcAABB(const FTransform& ParentTransform);

    void SetVertexData(const TArray<FVector>& InVertices)
    {
        VertexData = InVertices;
        UpdateLocalAABB();
    }

    void UpdateLocalAABB()
    {
        if (VertexData.IsEmpty())
        {
            LocalAABB = FAABB();
            return;
        }

        LocalAABB = FAABB(VertexData);
    }

    TArray<FVector> VertexData;
    TArray<int32> IndexData;
    FAABB LocalAABB;

    FTransform Transform;

    mutable physx::PxConvexMesh* ConvexMesh;

    static EAggCollisionShape StaticShapeType;
};