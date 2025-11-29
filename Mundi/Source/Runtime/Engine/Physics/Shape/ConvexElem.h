#pragma once
#include "ShapeElem.h"

struct FConvexElem : public FShapeElem
{
    FConvexElem()
        : FShapeElem(EAggCollisionShape::Convex),
          VertexData(),
          IndexData(),
          LocalAABB(FAABB())
    {}
    FConvexElem(const FConvexElem& Other)
        : FShapeElem(Other),
          VertexData(Other.VertexData),
          IndexData(Other.IndexData),
          LocalAABB(Other.LocalAABB)
    {}
    virtual ~FConvexElem() override;

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

    static EAggCollisionShape StaticShapeType;
};