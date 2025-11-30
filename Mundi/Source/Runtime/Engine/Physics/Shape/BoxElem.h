#pragma once
#include "ShapeElem.h"

struct FBoxElem : public FShapeElem
{
    FBoxElem()
        : FShapeElem(EAggCollisionShape::Box),
          Center(FVector::Zero()),
          Rotation(FQuat::Identity()),
          X(1.0f), Y(1.0f), Z(1.0f)
    {}

    FBoxElem(float Size)
        : FShapeElem(EAggCollisionShape::Box),
          Center(FVector::Zero()),
          Rotation(FQuat::Identity()),
          X(Size), Y(Size), Z(Size)
    {}

    FBoxElem(float InX, float InY, float InZ)
        : FShapeElem(EAggCollisionShape::Box),
          Center(FVector::Zero()),
          Rotation(FQuat::Identity()),
          X(InX), Y(InY), Z(InZ)
    {}
    
    virtual ~FBoxElem() override;

    friend bool operator==(const FBoxElem& LHS, const  FBoxElem& RHS)
    {
        return (LHS.Center == RHS.Center) &&
                (LHS.Rotation == RHS.Rotation) &&
                (LHS.X == RHS.X) && (LHS.Y == RHS.Y) && (LHS.Z == RHS.Z);
    }

    FTransform GetTransform() const override
    {
        return FTransform(Center, Rotation, FVector(X, Y, Z));
    }

    void SetTransform(const FTransform& InTransform)
    {
        Center = InTransform.Translation;
        Rotation = InTransform.Rotation;
    }

    FAABB CalcAABB(const FTransform& ParentTransform);
    
    FVector Center;
    FQuat Rotation;

    // 언리얼에서는 box의 전체 길이를 저장하고 Chaos로 넘길 때 반으로 나눠서 Extent로 변환
    // 언리얼 방식대로 전체 길이 저장
    float X, Y, Z;

    static EAggCollisionShape StaticShapeType;
};