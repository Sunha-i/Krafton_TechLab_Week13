#include "pch.h"
#include "PhysCapsuleActor.h"
#include "StaticMeshComponent.h"
#include "CapsuleComponent.h"

APhysCapsuleActor::APhysCapsuleActor()
{
    ObjectName = "PhysCapsule";

    // 시각화용 StaticMeshComponent 생성
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("MeshComponent");
    MeshComponent->SetStaticMesh("Data/Model/Capsule.fbx");
    RootComponent = MeshComponent;

    // 물리 충돌용 CapsuleComponent 생성 (MeshComponent에 부착)
    CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
    CapsuleComponent->SetupAttachment(MeshComponent);

    // Capsule.fbx가 Y축으로 90도 회전된 상태이므로, CapsuleComponent도 맞춰서 회전
    // CapsuleComponent->SetRelativeRotation(FQuat::MakeFromEulerZYX(FVector(0.0f, 90.0f, 0.0f)));

    // 물리 시뮬레이션 활성화
    CapsuleComponent->BodyInstance.bSimulatePhysics = true;
    CapsuleComponent->BodyInstance.bEnableGravity = true;
}

APhysCapsuleActor::~APhysCapsuleActor()
{
}

void APhysCapsuleActor::BeginPlay()
{
    Super::BeginPlay();  // CreatePhysicsState() 자동 호출
}

void APhysCapsuleActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 물리 결과를 MeshComponent에 반영
    if (CapsuleComponent && CapsuleComponent->BodyInstance.IsInitialized())
    {
        FTransform PhysTransform = CapsuleComponent->BodyInstance.GetWorldTransform();
        MeshComponent->SetWorldTransform(PhysTransform);
    }
}

FAABB APhysCapsuleActor::GetBounds() const
{
    if (MeshComponent)
    {
        return MeshComponent->GetWorldAABB();
    }
    return FAABB();
}

void APhysCapsuleActor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
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
            else if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Component))
            {
                CapsuleComponent = Capsule;
            }
        }
    }
}

void APhysCapsuleActor::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    // 복제된 컴포넌트로 멤버 포인터 업데이트
    for (UActorComponent* Component : OwnedComponents)
    {
        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
        {
            MeshComponent = Mesh;
        }
        else if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Component))
        {
            CapsuleComponent = Capsule;
        }
	}
}
