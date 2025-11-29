#pragma once
#include "../Physics/PhysicsUserData.h"
#include "../Misc/Macros.h"

// 언리얼에 더 많은 타입이 있지만 발제에 나온 것만 추가
enum class EAggCollisionShape : uint8
{
    Sphere,
    Box,
    Sphyl,
    Convex,

    Unknown
};

// Probe는 제외
enum class ECollisionEnabled : uint8
{
    // 물리 엔진 입장에서 물체가 존재하지 않음
    NoCollision,
    // 충돌체가 존재하지만 시뮬레이션되지는 않음.
    // 레이캐스트에 검사된다.
    QueryOnly,
    // 레이캐스트 X, 시뮬레이션 O
    PhysicsOnly,
    QueryAndPhysics    
};

struct FShapeElem
{
public:
    FShapeElem()
        : RestOffset(0.0f),
          bIsGenerated(false),
          ShapeType(EAggCollisionShape::Unknown),
          bContributeToMass(true),
          CollisionEnable(ECollisionEnabled::QueryAndPhysics),
          UserData(this)
    {}

    FShapeElem(EAggCollisionShape InShape)
        : RestOffset(0.0f),
          bIsGenerated(false),
          ShapeType(InShape),
          bContributeToMass(true),
          CollisionEnable(ECollisionEnabled::QueryAndPhysics),
          UserData(this)
    {}

    FShapeElem(const FShapeElem& Copy)
        : RestOffset(Copy.RestOffset),
          bIsGenerated(Copy.bIsGenerated),
          Name(Copy.Name),
          ShapeType(Copy.ShapeType),
          bContributeToMass(Copy.bContributeToMass),
          CollisionEnable(Copy.CollisionEnable),
          UserData(this)
    {}
    
    virtual ~FShapeElem();

    FShapeElem& operator=(const FShapeElem& Other)
    {
        if (this != &Other)
        {
            CloneElem(Other);
        }
        return *this;
    }

    template<typename T>
    T* GetShapeCheck()
    {        
        CHECK_MSG(T::StaticShapeType == ShapeType, "FKShapeElem type check fail");
        return static_cast<T*>(this);
    }

    const FName& GetName() const { return Name; }
    void SetName(const FName& InName) { Name = InName; }
    EAggCollisionShape GetShapeType() const { return ShapeType; }
    bool GetContributeToMass() const { return bContributeToMass; }
    void SetContributeToMass(bool bInContributesToMass) { bContributeToMass = bInContributesToMass; }
    void SetCollisionEnabled(ECollisionEnabled InCOllisionEnabled) { CollisionEnable = InCOllisionEnabled; }
    ECollisionEnabled GetCollisionEnabled() const { return CollisionEnable; }

    virtual FTransform GetTransform() const
    {
        return FTransform();
    }

    // RTTI용 static 변수
    static EAggCollisionShape StaticShapeType;

    // 충돌 판정 여유 거리, 0보다 크게 한다
    // 물체가 떨리는 jitter 방지
    // 모서리를 둥글게(Minkowski sum)해서 부드러운 이동 지원
    // 모서리에서 RestOffset만큼 떨어진 점의 집합은 구(Sphere)
    // 요철이 존재하는 메시를 날카로운 모서리가 지나면서 충돌 처리를 하면 요철에 의해 튐
    // 둥근 모서리의 경우 요철을 타고 넘어감
    // 터널링(뚫고 지나가는 현상) 방지
    float RestOffset;

    bool bIsGenerated;

protected:
    // 대입연산자 헬퍼함수
    void CloneElem(const FShapeElem& Other)
    {
        RestOffset = Other.RestOffset;
        ShapeType = Other.ShapeType;
        Name = Other.Name;
        bContributeToMass = Other.bContributeToMass;
        CollisionEnable = Other.CollisionEnable;
        bIsGenerated = Other.bIsGenerated;        
    }    

private:

    EAggCollisionShape ShapeType;

    FName Name;

    // shape가 body에 질량을 더하는 지의 여부
    // 충돌체(shape)은 존재하지만 물리 식에 영향을 주지 않음(질량 0)
    bool bContributeToMass;

    ECollisionEnabled CollisionEnable;

    FUserData UserData;
};