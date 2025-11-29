#include "pch.h"
#include "SimpleWheeledVehicleMovementComponent.h"

#include "Actor.h"
#include "GlobalConsole.h"
#include "PhysicsCore.h"
#include "SceneComponent.h"
#include "StaticMeshComponent.h"

#include <PxPhysicsAPI.h>

IMPLEMENT_CLASS(USimpleWheeledVehicleMovementComponent)

USimpleWheeledVehicleMovementComponent::USimpleWheeledVehicleMovementComponent()
{
    bCanEverTick = true;
    ThrottleInput = 0.0f;
    SteeringInput = 0.0f;
    BrakeInput = 0.0f;
    HandbrakeInput = 0.0f;
}

USimpleWheeledVehicleMovementComponent::~USimpleWheeledVehicleMovementComponent() = default;

void USimpleWheeledVehicleMovementComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    CacheUpdatedComponent();

    if (!bVehicleInitialized)
    {
        bVehicleInitialized = InitVehiclePhysX();
        if (!bVehicleInitialized)
        {
            return;
        }
    }

    ApplyInputToPhysX();
    PerformSuspensionRaycasts();
    SimulateVehicle(DeltaTime);
    UpdateVehiclePoseFromPhysX();
}

void USimpleWheeledVehicleMovementComponent::SetThrottleInput(float Throttle)
{
    ThrottleInput = FMath::Clamp(Throttle, -1.0f, 1.0f);
}

void USimpleWheeledVehicleMovementComponent::SetSteeringInput(float Steering)
{
    SteeringInput = FMath::Clamp(Steering, -1.0f, 1.0f);
}

void USimpleWheeledVehicleMovementComponent::SetBrakeInput(float Brake)
{
    BrakeInput = FMath::Clamp(Brake, 0.0f, 1.0f);
}

void USimpleWheeledVehicleMovementComponent::SetHandbrakeInput(float Handbrake)
{
    HandbrakeInput = FMath::Clamp(Handbrake, 0.0f, 1.0f);
}

void USimpleWheeledVehicleMovementComponent::GearUp()
{
    if (!bVehicleInitialized || !PxVehicleDrive4WInstance)
    {
        return;
    }

    UE_LOG("[VehicleMovement] GearUp not implemented yet");
}

void USimpleWheeledVehicleMovementComponent::GearDown()
{
    if (!bVehicleInitialized || !PxVehicleDrive4WInstance)
    {
        return;
    }

    UE_LOG("[VehicleMovement] GearDown not implemented yet");
}

bool USimpleWheeledVehicleMovementComponent::InitVehiclePhysX()
{
    if (bVehicleInitialized)
    {
        return true;
    }

    if (!CachedBodyMesh)
    {
        CacheUpdatedComponent();
    }

    if (!CachedBodyMesh)
    {
        if (!bWarnedMissingBodyMesh)
        {
            UE_LOG("[VehicleMovement] Body mesh not found. Assign a StaticMeshComponent as the updated component.");
            bWarnedMissingBodyMesh = true;
        }
        return false;
    }

    if (WheelSetups.Num() == 0)
    {
        if (!bWarnedWheelSetup)
        {
            UE_LOG("[VehicleMovement] Wheel setup array is empty. Add at least one wheel before initializing.");
            bWarnedWheelSetup = true;
        }
        return false;
    }

    if (!FPhysicsCore::Get().IsInitialized())
    {
        if (!bWarnedPhysicsUninitialized)
        {
            UE_LOG("[VehicleMovement] FPhysicsCore is not initialized yet.");
            bWarnedPhysicsUninitialized = true;
        }
        return false;
    }

    // TODO: 실제 PhysX Vehicle 셋업 구현
    UE_LOG("[VehicleMovement] Vehicle PhysX initialization stub executed.");
    bVehicleInitialized = true;
    return true;
}

void USimpleWheeledVehicleMovementComponent::ApplyInputToPhysX()
{
    if (!bVehicleInitialized || !PxVehicleDrive4WInstance)
    {
        return;
    }

    // TODO: PxVehicleDrive4WRawInputData에 입력 적용
}

void USimpleWheeledVehicleMovementComponent::PerformSuspensionRaycasts()
{
    if (!bVehicleInitialized || !PxVehicleDrive4WInstance)
    {
        return;
    }

    // TODO: PxVehicleSuspensionRaycasts 호출
}

void USimpleWheeledVehicleMovementComponent::SimulateVehicle(float DeltaTime)
{
    if (!bVehicleInitialized || !PxVehicleDrive4WInstance)
    {
        return;
    }

    // TODO: PxVehicleUpdates 호출
}

void USimpleWheeledVehicleMovementComponent::UpdateVehiclePoseFromPhysX()
{
    if (!bVehicleInitialized || !PxVehicleDrive4WInstance || !CachedBodyMesh)
    {
        return;
    }

    // TODO: PhysX 결과를 월드 트랜스폼으로 복사
}

void USimpleWheeledVehicleMovementComponent::CacheUpdatedComponent()
{
    if (CachedBodyMesh && CachedBodyMesh->GetOwner() == Owner)
    {
        return;
    }

    CachedBodyMesh = Cast<UStaticMeshComponent>(UpdatedComponent);
    if (CachedBodyMesh)
    {
        return;
    }

    if (!Owner)
    {
        return;
    }

    if (AActor* OwnerActor = Cast<AActor>(Owner))
    {
        if (USceneComponent* RootComp = OwnerActor->GetRootComponent())
        {
            CachedBodyMesh = Cast<UStaticMeshComponent>(RootComp);
        }

        if (!CachedBodyMesh)
        {
            if (UActorComponent* FoundComp = OwnerActor->GetComponent(UStaticMeshComponent::StaticClass()))
            {
                CachedBodyMesh = Cast<UStaticMeshComponent>(FoundComp);
            }
        }
    }
}
