#pragma once
#include "Actor.h"
#include "APhysBoxActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

UCLASS(DisplayName="물리 박스", Description="물리 시뮬레이션이 적용된 박스 액터")
class APhysBoxActor : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    APhysBoxActor();

protected:
    ~APhysBoxActor() override;

public:
    void BeginPlay() override;
    void Tick(float DeltaTime) override;
    FAABB GetBounds() const override;

	void DuplicateSubObjects() override;

    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }
    UBoxComponent* GetBoxComponent() const { return BoxComponent; }

protected:
    // 시각화용 메시
    UStaticMeshComponent* MeshComponent = nullptr;

    // 물리 충돌용 박스
    UBoxComponent* BoxComponent = nullptr;
};
