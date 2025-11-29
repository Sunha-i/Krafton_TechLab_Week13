#include "pch.h"
#include "PhysGroundActor.h"
#include "StaticMeshComponent.h"
#include "BoxComponent.h"

APhysGroundActor::APhysGroundActor()
{
    ObjectName = "PhysGround";

    // 시각화용 StaticMeshComponent 생성 (스케일 조정으로 평면처럼)
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
    MeshComponent->SetStaticMesh("Data/cube-tex.obj");
    MeshComponent->SetWorldScale(FVector(10.0f, 10.0f, 0.1f));  // 넓고 얇은 바닥
    RootComponent = MeshComponent;

    // 물리 충돌용 BoxComponent 생성
    BoxComponent = CreateDefaultSubobject<UBoxComponent>("BoxComponent");
    BoxComponent->SetupAttachment(MeshComponent);
    BoxComponent->SetBoxExtent(FVector(5.0f, 5.0f, 0.05f));  // 스케일에 맞춤

    // 정적 물리 (시뮬레이션 비활성화)
    BoxComponent->BodyInstance.bSimulatePhysics = false;
    BoxComponent->BodyInstance.bEnableGravity = false;
}

APhysGroundActor::~APhysGroundActor()
{
}

void APhysGroundActor::BeginPlay()
{
    Super::BeginPlay();  // CreatePhysicsState() 호출 → Static Actor 생성
}

FAABB APhysGroundActor::GetBounds() const
{
    if (MeshComponent)
    {
        return MeshComponent->GetWorldAABB();
    }
    return FAABB();
}

void APhysGroundActor::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();

    // 복제된 컴포넌트로 멤버 포인터 업데이트
    for (UActorComponent* Component : OwnedComponents)
    {
        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
        {
            MeshComponent = Mesh;
        }
        else if (UBoxComponent* Box = Cast<UBoxComponent>(Component))
        {
            BoxComponent = Box;
        }
    }
}
