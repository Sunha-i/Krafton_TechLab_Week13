#pragma once
#include "Actor.h"
#include "APhysSphereActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;

UCLASS(DisplayName="물리 구체", Description="물리 시뮬레이션이 적용된 구체 액터")
class APhysSphereActor : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    APhysSphereActor();

protected:
    ~APhysSphereActor() override;

public:
    void BeginPlay() override;
    void Tick(float DeltaTime) override;
    FAABB GetBounds() const override;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void DuplicateSubObjects() override;

    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }
    USphereComponent* GetSphereComponent() const { return SphereComponent; }

protected:
    // 시각화용 메시
    UStaticMeshComponent* MeshComponent = nullptr;

    // 물리 충돌용 구체
    USphereComponent* SphereComponent = nullptr;
};
