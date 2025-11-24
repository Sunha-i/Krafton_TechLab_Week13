#include "pch.h"
#include "ParticleEditorBootstrap.h"
#include "ViewerState.h"
#include "CameraActor.h"
#include "FViewport.h"
#include "FViewportClient.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"
#include "ParticleSystemComponent.h"
#include "Grid/GridActor.h"
#include "Modules/ParticleModuleSpawn.h"
#include "Modules/ParticleModuleLifetime.h"
#include "Modules/ParticleModuleSize.h"
#include "Modules/ParticleModuleVelocity.h"
#include "Modules/ParticleModuleColor.h"

ViewerState* ParticleEditorBootstrap::CreateViewerState(const char* Name, UWorld* InWorld,
	ID3D11Device* InDevice, UEditorAssetPreviewContext* Context)
{
	if (!InDevice) return nullptr;

	// ParticleEditorState 생성
	ParticleEditorState* State = new ParticleEditorState();
	State->Name = Name ? Name : "Particle Editor";

	// Preview world 생성
	State->World = NewObject<UWorld>();
	State->World->SetWorldType(EWorldType::PreviewMinimal);
	State->World->Initialize();
	State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_EditorIcon);
	State->World->GetRenderSettings().DisableShowFlag(EEngineShowFlags::SF_Grid);

	// Viewport 생성
	State->Viewport = new FViewport();
	State->Viewport->Initialize(0.f, 0.f, 1.f, 1.f, InDevice);
	State->Viewport->SetUseRenderTarget(true);

	// ViewportClient 생성
	FViewportClient* Client = new FViewportClient();
	Client->SetWorld(State->World);
	Client->SetViewportType(EViewportType::Perspective);
	Client->SetViewMode(EViewMode::VMI_Lit_Phong);

	// 카메라 설정
	Client->GetCamera()->SetActorLocation(FVector(10.f, 0.f, 5.f));
	Client->GetCamera()->SetRotationFromEulerAngles(FVector(0.f, 20.f, 180.f));

	State->Client = Client;
	State->Viewport->SetViewportClient(Client);
	State->World->SetEditorCameraActor(Client->GetCamera());

	// 프리뷰 액터 생성 (파티클 시스템을 담을 액터)
	if (State->World)
	{
		AActor* PreviewActor = State->World->SpawnActor<AActor>();
		if (PreviewActor)
		{
			// ParticleSystemComponent 생성 및 연결
			UParticleSystemComponent* ParticleComp = NewObject<UParticleSystemComponent>();
			PreviewActor->AddOwnedComponent(ParticleComp);
			ParticleComp->RegisterComponent(State->World);
			ParticleComp->SetWorldLocation(FVector(0.f, 0.f, 0.f));

			State->PreviewActor = PreviewActor;
			State->PreviewComponent = ParticleComp;

			// 기본 파티클 시스템 생성
			UParticleSystem* DefaultTemplate = NewObject<UParticleSystem>();

			// 기본 이미터 1개 생성
			UParticleEmitter* DefaultEmitter = NewObject<UParticleEmitter>();
			UParticleLODLevel* LOD = NewObject<UParticleLODLevel>();
			LOD->bEnabled = true;

			// 1. Required 모듈 (필수)
			LOD->RequiredModule = NewObject<UParticleModuleRequired>();

			// 2. Spawn 모듈 (필수)
			UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
			SpawnModule->SpawnRate = 20.0f;
			SpawnModule->BurstCount = 0;
			LOD->SpawnModule = SpawnModule;
			LOD->Modules.Add(SpawnModule);

			// 3. Lifetime 모듈
			UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
			LifetimeModule->MinLifetime = 1.0f;
			LifetimeModule->MaxLifetime = 1.0f;
			LOD->Modules.Add(LifetimeModule);

			// 4. Initial Size 모듈
			UParticleModuleSize* SizeModule = NewObject<UParticleModuleSize>();
			SizeModule->StartSize = FVector(1.0f, 1.0f, 1.0f);
			SizeModule->StartSizeRange = FVector(1.0f, 1.0f, 1.0f);
			LOD->Modules.Add(SizeModule);

			// 5. Initial Velocity 모듈
			UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
			VelocityModule->StartVelocity = FVector(1.0f, 1.0f, 10.0f);
			VelocityModule->StartVelocityRange = FVector(1.0f, 1.0f, 11.0f);
			LOD->Modules.Add(VelocityModule);

			// 6. Color Over Life 모듈
			UParticleModuleColor* ColorModule = NewObject<UParticleModuleColor>();
			ColorModule->StartColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
			ColorModule->EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.0f);  // 페이드 아웃
			LOD->Modules.Add(ColorModule);

			// 모듈 캐싱
			LOD->CacheModuleInfo();

			DefaultEmitter->LODLevels.Add(LOD);
			DefaultEmitter->CacheEmitterModuleInfo();

			DefaultTemplate->Emitters.Add(DefaultEmitter);

			State->EditingTemplate = DefaultTemplate;
			State->PreviewComponent->SetTemplate(DefaultTemplate);
		}
	}

	return State;
}

void ParticleEditorBootstrap::DestroyViewerState(ViewerState*& State)
{
	if (!State) return;

	// ParticleEditorState로 캐스팅
	ParticleEditorState* ParticleState = static_cast<ParticleEditorState*>(State);

	// 리소스 정리
	if (ParticleState->Viewport)
	{
		delete ParticleState->Viewport;
		ParticleState->Viewport = nullptr;
	}

	if (ParticleState->Client)
	{
		delete ParticleState->Client;
		ParticleState->Client = nullptr;
	}

	if (ParticleState->World)
	{
		ObjectFactory::DeleteObject(ParticleState->World);
		ParticleState->World = nullptr;
	}

	// 파티클 관련 포인터는 World가 정리되면서 자동으로 정리됨
	ParticleState->EditingTemplate = nullptr;
	ParticleState->PreviewComponent = nullptr;
	ParticleState->PreviewActor = nullptr;
	ParticleState->SelectedModule = nullptr;

	delete State;
	State = nullptr;
}
