#pragma once
#include "ShapeElem.h"

struct FConvexElem : public FShapeElem
{
    FConvexElem() {}
    FConvexElem(const FConvexElem& Other) {}
    virtual ~FConvexElem() override = default;

    TArray<FVector> VertexData;
    TArray<int32> IndexData;
    FAABB Bounds;    

    inline static EAggCollisionShape StaticShapeType = EAggCollisionShape::Convex;
};