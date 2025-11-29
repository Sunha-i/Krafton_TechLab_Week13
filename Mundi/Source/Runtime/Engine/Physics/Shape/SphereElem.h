#pragma once
#include "ShapeElem.h"

struct FSphereElem : public FShapeElem
{
    FSphereElem()
        : FShapeElem(EAggCollisionShape::Sphere),
          Center(FVector::Zero()),
          Radius(1.0f)
    {}

    FSphereElem(float InRadius)
        : FShapeElem(EAggCollisionShape::Sphere),
          Center(FVector::Zero()),
          Radius(InRadius)
    {}

    virtual ~FSphereElem() override = default;

    friend bool operator==(const FSphereElem& LHS, const FSphereElem& RHS)
    {
        return (LHS.Center == RHS.Center) &&
                (LHS.Radius == RHS.Radius);
    }


    FVector Center;
    float Radius;

    inline static EAggCollisionShape StaticShapeType = EAggCollisionShape::Sphere;
};