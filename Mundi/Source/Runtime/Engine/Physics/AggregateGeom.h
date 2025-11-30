#pragma once
#include "Shape/BoxElem.h"
#include "Shape/SphereElem.h"
#include "Shape/SphylElem.h"
#include "Shape/ConvexElem.h"

// 더미 클래스 나중에 렌더인포 클래스 구현 필요
class FAggregateRenderInfo;

struct FAggregateGeom
{
    FAggregateGeom() : RenderInfo(nullptr) {}
    FAggregateGeom(const FAggregateGeom& Other) : RenderInfo(nullptr)
    {
        CloneGeom(Other);
    }
    ~FAggregateGeom();

    FAggregateGeom& operator=(const FAggregateGeom& Other)
    {
        if (this != &Other)
        {
            // FreeRenderInfo()
            CloneGeom(Other);
        }
        return *this;
    }

    FAABB CalcAABB(const FTransform& Transform);
    
    void EmptyElements()
    {
        SphereElems.Empty();
        BoxElems.Empty();
        SphylElems.Empty();
        ConvexElems.Empty();
    }

    int32 GetElementCount() const
    {
        return SphereElems.Num() + BoxElems.Num() + ConvexElems.Num() + SphylElems.Num();
    }

    //TODO RenderInfo 해제 구현
    // void FreeRenderInfo();

    TArray<FSphereElem> SphereElems;
    TArray<FBoxElem> BoxElems;
    TArray<FSphylElem> SphylElems;
    TArray<FConvexElem> ConvexElems;

private:
    // 헬퍼 함수
    void CloneGeom(const FAggregateGeom& Other)
    {
        SphereElems = Other.SphereElems;
        BoxElems = Other.BoxElems;
        SphylElems = Other.SphylElems;
        ConvexElems = Other.ConvexElems;
    }

    FAggregateRenderInfo* RenderInfo;
};