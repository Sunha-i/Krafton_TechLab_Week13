#pragma once
#include "ShapeElem.h"

// shape의 범위를 결정하는 요소만 가지고 있음 eg., Radius, Length
// 다른 shapeelem 구조체도 동일하다
// 충돌 검사에는 Shape을 정의하는 요소만 필요하고 메시는 불필요
// 메시는 Factory class에서 생성함
struct FSphylElem : public FShapeElem
{
    FSphylElem()
        : FShapeElem(EAggCollisionShape::Sphyl),
          Center(FVector::Zero()),
          Rotation(FQuat::Identity()),
          Radius(1.0f),
          Length(1.0f)
    {}

    FSphylElem(float InRadius, float InLength)
        : FShapeElem(EAggCollisionShape::Sphyl),
          Center(FVector::Zero()),
          Rotation(FQuat::Identity()),
          Radius(InRadius),
          Length(InLength)
    {}

    virtual ~FSphylElem() override;
    
    friend bool operator==(const FSphylElem& LHS, const FSphylElem& RHS)
    {
        return (LHS.Center == RHS.Center) &&
                (LHS.Rotation == RHS.Rotation) &&
                (LHS.Radius == RHS.Radius) &&
                (LHS.Length == RHS.Length);
    }

    FAABB CalcAABB(const FTransform& ParentTransform);

    FVector Center;
    FQuat Rotation;
    float Radius;
    float Length;

    static EAggCollisionShape StaticShapeType;
};