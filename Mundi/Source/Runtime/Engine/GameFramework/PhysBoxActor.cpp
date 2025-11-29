#include "pch.h"
#include "PhysBoxActor.h"
#include "StaticMeshComponent.h"
#include "BoxComponent.h"

APhysBoxActor::APhysBoxActor()
{
    ObjectName = "PhysBox";

    // 시각화용 StaticMeshComponent 생성
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
    MeshComponent->SetStaticMesh("Data/cube-tex.obj");
    RootComponent = MeshComponent;

    // 물리 충돌용 BoxComponent 생성 (MeshComponent에 부착)
    BoxComponent = CreateDefaultSubobject<UBoxComponent>("BoxComponent");
    BoxComponent->SetupAttachment(MeshComponent);
    BoxComponent->SetBoxExtent(FVector(0.5f, 0.5f, 0.5f));  // cube-tex.obj 크기에 맞춤

    // 물리 시뮬레이션 활성화
    BoxComponent->BodyInstance.bSimulatePhysics = true;
    BoxComponent->BodyInstance.bEnableGravity = true;
}

APhysBoxActor::~APhysBoxActor()
{
}

void APhysBoxActor::BeginPlay()
{
    Super::BeginPlay();  // CreatePhysicsState() 자동 호출
}

void APhysBoxActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 물리 결과를 MeshComponent에 반영
    if (BoxComponent && BoxComponent->BodyInstance.IsInitialized())
    {
        FTransform PhysTransform = BoxComponent->BodyInstance.GetWorldTransform();
        MeshComponent->SetWorldTransform(PhysTransform);
    }
}

FAABB APhysBoxActor::GetBounds() const
{
    if (MeshComponent)
    {
        return MeshComponent->GetWorldAABB();
    }
    return FAABB();
}

void APhysBoxActor::DuplicateSubObjects()
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
