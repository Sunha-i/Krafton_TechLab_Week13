#pragma once

#include "ResourceBase.h"
#include "ParticleEmitter.h"
#include "UParticleSystem.generated.h"

UCLASS(DisplayName="파티클 시스템", Description="파티클 시스템은 여러 이미터를 포함하는 에셋입니다")
class UParticleSystem : public UResourceBase
{
public:
	GENERATED_REFLECTION_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Particle System")
	TArray<UParticleEmitter*> Emitters;

	//=============================================================================
	// LOD 설정 (언리얼 엔진 호환)
	// 모든 이미터가 동일한 거리에서 LOD 전환
	//=============================================================================

	// LOD 사용 여부
	UPROPERTY(EditAnywhere, Category = "LOD")
	bool bUseLOD = false;

	// 각 LOD 레벨의 전환 거리
	// LODDistances[0] = LOD 0->1 전환 거리
	// LODDistances[1] = LOD 1->2 전환 거리
	// 예: [500, 1000] → 0~500: LOD0, 500~1000: LOD1, 1000+: LOD2
	UPROPERTY(EditAnywhere, Category = "LOD")
	TArray<float> LODDistances;

	UParticleSystem() = default;
	virtual ~UParticleSystem();

	// 리소스 로드 (ResourceManager에서 호출)
	void Load(const FString& InFilePath, ID3D11Device* InDevice);

	// 인덱스로 이미터 가져오기
	UParticleEmitter* GetEmitter(int32 EmitterIndex) const;

	// 직렬화
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// 복제
	virtual void DuplicateSubObjects() override;
};
