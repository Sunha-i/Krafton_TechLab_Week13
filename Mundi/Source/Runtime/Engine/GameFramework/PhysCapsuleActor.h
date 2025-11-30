#pragma once
#include "Actor.h"
#include "APhysCapsuleActor.generated.h"

class UStaticMeshComponent;
class UCapsuleComponent;

UCLASS(DisplayName="물리 캡슐", Description="물리 시뮬레이션이 적용된 캡슐 액터")
class APhysCapsuleActor : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    APhysCapsuleActor();

protected:
    ~APhysCapsuleActor() override;

public:
    void BeginPlay() override;
    void Tick(float DeltaTime) override;
    FAABB GetBounds() const override;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void DuplicateSubObjects() override;

    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }
    UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

protected:
    // 시각화용 메시
    UStaticMeshComponent* MeshComponent = nullptr;

    // 물리 충돌용 캡슐
    UCapsuleComponent* CapsuleComponent = nullptr;
};
