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

    virtual ~FSphereElem() override;

    friend bool operator==(const FSphereElem& LHS, const FSphereElem& RHS)
    {
        return (LHS.Center == RHS.Center) &&
                (LHS.Radius == RHS.Radius);
    }

    FTransform GetTransform() const override
    {
        return FTransform(Center, FQuat::Identity(), FVector::One());
    }

    void SetTransform(const FTransform& InTransform)
    {
        Center = InTransform.Translation;
    }

    FAABB CalcAABB(const FTransform& ParentTransform);

    // 부모의 원점에서 얼마나 떨어져 있는지를 나타내는 local offset
    FVector Center;
    float Radius;

    static EAggCollisionShape StaticShapeType;
};