#pragma once

class ViewerState;
class UWorld;
struct ID3D11Device;
class UEditorAssetPreviewContext;
class UParticleSystem;

class ParticleEditorBootstrap
{
public:
	// ViewerState 생성 (8단계에서 구현)
	static ViewerState* CreateViewerState(const char* Name, UWorld* InWorld,
		ID3D11Device* InDevice, UEditorAssetPreviewContext* Context);

	// ViewerState 소멸 (8단계에서 구현)
	static void DestroyViewerState(ViewerState*& State);

	// 기본 파티클 템플릿 생성 (6개 기본 모듈 포함)
	static UParticleSystem* CreateDefaultParticleTemplate();

	// 파티클 시스템을 JSON 파일로 저장
	static bool SaveParticleSystem(UParticleSystem* System, const FString& FilePath);

	// JSON 파일에서 파티클 시스템 로드
	static UParticleSystem* LoadParticleSystem(const FString& FilePath);
};
