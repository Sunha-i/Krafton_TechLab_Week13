#include "pch.h"
#include "PhysSphereActor.h"
#include "StaticMeshComponent.h"
#include "SphereComponent.h"

APhysSphereActor::APhysSphereActor()
{
    ObjectName = "PhysSphere";

    // 시각화용 StaticMeshComponent 생성
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
    MeshComponent->SetStaticMesh("Data/Model/Sphere.obj");
    RootComponent = MeshComponent;

    // 물리 충돌용 SphereComponent 생성 (MeshComponent에 부착)
    SphereComponent = CreateDefaultSubobject<USphereComponent>("SphereComponent");
    SphereComponent->SetupAttachment(MeshComponent);

    // 물리 시뮬레이션 활성화
    SphereComponent->BodyInstance.bSimulatePhysics = true;
    SphereComponent->BodyInstance.bEnableGravity = true;
}

APhysSphereActor::~APhysSphereActor()
{
}

void APhysSphereActor::BeginPlay()
{
    Super::BeginPlay();  // CreatePhysicsState() 자동 호출
}

void APhysSphereActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 물리 결과를 MeshComponent에 반영
    if (SphereComponent && SphereComponent->BodyInstance.IsInitialized())
    {
        FTransform PhysTransform = SphereComponent->BodyInstance.GetWorldTransform();
        MeshComponent->SetWorldTransform(PhysTransform);
    }
}

FAABB APhysSphereActor::GetBounds() const
{
    if (MeshComponent)
    {
        return MeshComponent->GetWorldAABB();
    }
    return FAABB();
}

void APhysSphereActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // OwnedComponents에서 컴포넌트 포인터 재설정
        for (UActorComponent* Component : OwnedComponents)
        {
            if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
            {
                MeshComponent = Mesh;
            }
            else if (USphereComponent* Sphere = Cast<USphereComponent>(Component))
            {
                SphereComponent = Sphere;
            }
        }
    }
}

void APhysSphereActor::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    // 복제된 컴포넌트로 멤버 포인터 업데이트
    for (UActorComponent* Component : OwnedComponents)
    {
        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
        {
            MeshComponent = Mesh;
        }
        else if (USphereComponent* Sphere = Cast<USphereComponent>(Component))
        {
            SphereComponent = Sphere;
        }
	}
}
