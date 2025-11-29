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

    // FAggreagateGeom.cpp에서 default 지정
    virtual ~FSphylElem() override = default;
    
    friend bool operator==(const FSphylElem& LHS, const FSphylElem& RHS)
    {
        return (LHS.Center == RHS.Center) &&
                (LHS.Rotation == RHS.Rotation) &&
                (LHS.Radius == RHS.Radius) &&
                (LHS.Length == RHS.Length);
    }

    FVector Center;
    FQuat Rotation;
    float Radius;
    float Length;

    // FAggreagateGeom.cpp에서 값 대입
    inline static EAggCollisionShape StaticShapeType = EAggCollisionShape::Sphyl;
};