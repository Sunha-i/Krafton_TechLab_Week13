#pragma once
#include "Actor.h"
#include "APhysGroundActor.generated.h"

class UStaticMeshComponent;
class UBoxComponent;

UCLASS(DisplayName="물리 바닥", Description="물리 충돌용 바닥 액터")
class APhysGroundActor : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    APhysGroundActor();

protected:
    ~APhysGroundActor() override;

public:
    void BeginPlay() override;
    FAABB GetBounds() const override;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void DuplicateSubObjects() override;

    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }
    UBoxComponent* GetBoxComponent() const { return BoxComponent; }

protected:
    // 시각화용 메시
    UStaticMeshComponent* MeshComponent = nullptr;

    // 물리 충돌용 박스
    UBoxComponent* BoxComponent = nullptr;
};
