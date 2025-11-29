#pragma once

class UPrimitiveComponent;
class UPhysicalMaterial;
class FPhysScene;
struct FBodyInstance;
struct FConstraintInstance;
struct FShapeElem;

enum EUserDataType : uint8
{
    Invalid,
    PrimitiveComponent,
    PhysicalMaterial,
    PhysScene,    
    BodyInstance,
    ConstraintInstance,
    AggShape,
    // 언리얼에서는 CustomPayload인데 우린 댕글링포인터 검출용
    Max
};

struct FUserData
{
public:
    FUserData() : Type(Invalid), Payload(nullptr) {}
    FUserData(UPrimitiveComponent* InPayload) : Type(EUserDataType::PrimitiveComponent), Payload(InPayload) {}
    FUserData(UPhysicalMaterial* InPayload) : Type(EUserDataType::PhysicalMaterial), Payload(InPayload) {}
    FUserData(FPhysScene* InPayload) : Type(EUserDataType::PhysScene), Payload(InPayload) {}
    FUserData(FBodyInstance* InPayload) : Type(EUserDataType::BodyInstance), Payload(InPayload) {}
    FUserData(FConstraintInstance* InPayload) : Type(EUserDataType::ConstraintInstance), Payload(InPayload) {}
    FUserData(FShapeElem* InPayload) : Type(EUserDataType::AggShape), Payload(InPayload) {}

    template <class T>
    static T* Get(void* UserData);

    template <class T>
    static void Set(void* UserData, T* Payload);

private:
    // Set함수의 헬퍼
    static void SetInternal(void* UserData, void* InPayload, EUserDataType InType)
    {
        FUserData* Data = static_cast<FUserData*>(UserData);
        Data->Type = InType;
        Data->Payload = InPayload;
    }

protected:
    EUserDataType Type;
    void* Payload;
};

template <>
inline UPrimitiveComponent* FUserData::Get(void* UserData)
{
    if (!UserData)
    {
        return nullptr;
    }

    FUserData* Data = static_cast<FUserData*>(UserData);
    if (Data->Type != EUserDataType::PrimitiveComponent)
    {
        return nullptr;
    }

    return static_cast<UPrimitiveComponent*>(Data->Payload);
}

template <>
inline UPhysicalMaterial* FUserData::Get(void* UserData)
{
    if (!UserData)
    {
        return nullptr;
    }

    FUserData* Data = static_cast<FUserData*>(UserData);
    if (Data->Type != EUserDataType::PhysicalMaterial)
    {
        return nullptr;
    }

    return static_cast<UPhysicalMaterial*>(Data->Payload);
}

template <>
inline FPhysScene* FUserData::Get(void* UserData)
{
    if (!UserData)
    {
        return nullptr;
    }

    FUserData* Data = static_cast<FUserData*>(UserData);
    if (Data->Type != EUserDataType::PhysScene)
    {
        return nullptr;
    }

    return static_cast<FPhysScene*>(Data->Payload);
}

template <>
inline FBodyInstance* FUserData::Get(void* UserData)
{
    if (!UserData)
    {
        return nullptr;
    }

    FUserData* Data = static_cast<FUserData*>(UserData);
    if (Data->Type != EUserDataType::BodyInstance)
    {
        return nullptr;
    }

    return static_cast<FBodyInstance*>(Data->Payload);
}

template <>
inline FConstraintInstance* FUserData::Get(void* UserData)
{
    if (!UserData)
    {
        return nullptr;
    }

    FUserData* Data = static_cast<FUserData*>(UserData);
    if (Data->Type != EUserDataType::PhysScene)
    {
        return nullptr;
    }

    return static_cast<FConstraintInstance*>(Data->Payload);
}

template <>
inline FShapeElem* FUserData::Get(void* UserData)
{
    if (!UserData)
    {
        return nullptr;
    }

    FUserData* Data = static_cast<FUserData*>(UserData);
    if (Data->Type != EUserDataType::AggShape)
    {
        return nullptr;
    } 

    return static_cast<FShapeElem*>(Data->Payload);
}

template <>
inline void FUserData::Set(void* UserData, UPrimitiveComponent* InPayload)
{
    assert(UserData && "Userdata is null");
    SetInternal(UserData, InPayload, EUserDataType::PrimitiveComponent);
}

template <>
inline void FUserData::Set(void* UserData, UPhysicalMaterial* InPayload)
{
    assert(UserData && "Userdata is null");
    SetInternal(UserData, InPayload, EUserDataType::PhysicalMaterial);
}

template <>
inline void FUserData::Set(void* UserData, FPhysScene* InPayload)
{
    assert(UserData && "Userdata is null");
    SetInternal(UserData, InPayload, EUserDataType::PhysScene);
}

template <>
inline void FUserData::Set(void* UserData, FBodyInstance* InPayload)
{
    assert(UserData && "Userdata is null");
    SetInternal(UserData, InPayload, EUserDataType::BodyInstance);
}

template <>
inline void FUserData::Set(void* UserData, FConstraintInstance* InPayload)
{
    assert(UserData && "Userdata is null");
    SetInternal(UserData, InPayload, EUserDataType::ConstraintInstance);
}

template <>
inline void FUserData::Set(void* UserData, FShapeElem* InPayload)
{
    assert(UserData && "Userdata is null");
    SetInternal(UserData, InPayload, EUserDataType::AggShape);
}