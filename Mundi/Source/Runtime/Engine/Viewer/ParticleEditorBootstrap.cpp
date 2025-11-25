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
#include "Modules/ParticleModuleTypeDataSprite.h"
#include "JsonSerializer.h"
#include "EditorAssetPreviewContext.h"

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

			// Context에 AssetPath가 있으면 파일에서 로드, 없으면 기본 템플릿 생성
			UParticleSystem* Template = nullptr;
			if (Context && !Context->AssetPath.empty())
			{
				// 파일에서 파티클 시스템 로드
				Template = LoadParticleSystem(Context->AssetPath);
				if (Template)
				{
					State->CurrentFilePath = Context->AssetPath;
					State->bIsDirty = false;
					UE_LOG("[ParticleEditorBootstrap] 파티클 시스템 로드: %s", Context->AssetPath.c_str());
				}
			}

			// 로드 실패하거나 AssetPath가 없으면 기본 템플릿 생성
			if (!Template)
			{
				Template = CreateDefaultParticleTemplate();
				UE_LOG("[ParticleEditorBootstrap] 기본 파티클 템플릿 생성");
			}

			State->EditingTemplate = Template;
			State->PreviewComponent->SetTemplate(Template);
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

UParticleSystem* ParticleEditorBootstrap::CreateDefaultParticleTemplate()
{
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

	// 스프라이트용 Material 설정
	UMaterial* SpriteMaterial = NewObject<UMaterial>();
	UShader* SpriteShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Particle/ParticleSprite.hlsl");
	SpriteMaterial->SetShader(SpriteShader);

	// 스프라이트 텍스처 설정 (불꽃/연기 등)
	FMaterialInfo MatInfo;
	MatInfo.DiffuseTextureFileName = GDataDir + "/Textures/Particles/OrientParticle.png";
	SpriteMaterial->SetMaterialInfo(MatInfo);
	SpriteMaterial->ResolveTextures();

	LOD->RequiredModule->Material = SpriteMaterial;

	// 스프라이트 타입 데이터 모듈 (TypeDataModule이 nullptr이면 기본적으로 스프라이트)
	UParticleModuleTypeDataSprite* SpriteTypeData = NewObject<UParticleModuleTypeDataSprite>();
	LOD->TypeDataModule = SpriteTypeData;

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
	ColorModule->EndColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	LOD->Modules.Add(ColorModule);

	// 모듈 캐싱
	LOD->CacheModuleInfo();

	DefaultEmitter->LODLevels.Add(LOD);
	DefaultEmitter->CacheEmitterModuleInfo();

	DefaultTemplate->Emitters.Add(DefaultEmitter);

	return DefaultTemplate;
}

bool ParticleEditorBootstrap::SaveParticleSystem(UParticleSystem* System, const FString& FilePath)
{
	// 입력 검증
	if (!System)
	{
		UE_LOG("[ParticleEditorBootstrap] SaveParticleSystem: System이 nullptr입니다");
		return false;
	}

	if (FilePath.empty())
	{
		UE_LOG("[ParticleEditorBootstrap] SaveParticleSystem: FilePath가 비어있습니다");
		return false;
	}

	// JSON 객체 생성
	JSON JsonHandle = JSON::Make(JSON::Class::Object);

	// ParticleSystem 직렬화 (false = 저장 모드)
	System->Serialize(false, JsonHandle);

	// FString을 FWideString으로 변환
	FWideString WidePath(FilePath.begin(), FilePath.end());

	// 파일로 저장
	if (!FJsonSerializer::SaveJsonToFile(JsonHandle, WidePath))
	{
		UE_LOG("[ParticleEditorBootstrap] SaveParticleSystem: 파일 저장 실패: %s", FilePath.c_str());
		return false;
	}

	UE_LOG("[ParticleEditorBootstrap] SaveParticleSystem: 저장 성공: %s", FilePath.c_str());
	return true;
}

UParticleSystem* ParticleEditorBootstrap::LoadParticleSystem(const FString& FilePath)
{
	// 입력 검증
	if (FilePath.empty())
	{
		UE_LOG("[ParticleEditorBootstrap] LoadParticleSystem: FilePath가 비어있습니다");
		return nullptr;
	}

	// FString을 FWideString으로 변환
	FWideString WidePath(FilePath.begin(), FilePath.end());

	// 파일에서 JSON 로드
	JSON JsonHandle;
	if (!FJsonSerializer::LoadJsonFromFile(JsonHandle, WidePath))
	{
		UE_LOG("[ParticleEditorBootstrap] LoadParticleSystem: 파일 로드 실패: %s", FilePath.c_str());
		return nullptr;
	}

	// 새로운 ParticleSystem 객체 생성
	UParticleSystem* LoadedSystem = NewObject<UParticleSystem>();
	if (!LoadedSystem)
	{
		UE_LOG("[ParticleEditorBootstrap] LoadParticleSystem: ParticleSystem 객체 생성 실패");
		return nullptr;
	}

	// ParticleSystem 역직렬화 (true = 로딩 모드)
	LoadedSystem->Serialize(true, JsonHandle);

	UE_LOG("[ParticleEditorBootstrap] LoadParticleSystem: 로드 성공: %s", FilePath.c_str());
	return LoadedSystem;
}
