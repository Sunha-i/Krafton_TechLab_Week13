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

    bool operator==(const FSphylElem& RHS) const
    {
        return (this->Center == RHS.Center) &&
                (this->Rotation == RHS.Rotation) &&
                (this->Radius == RHS.Radius) &&
                (this->Length == RHS.Length);
    }

    virtual ~FSphylElem() override = default;
    FVector Center;
    FQuat Rotation;
    float Radius;
    float Length;

    inline static EAggCollisionShape StaticShapeType = EAggCollisionShape::Sphyl;
};