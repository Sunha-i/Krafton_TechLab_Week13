#pragma once

#include "Object.h"
#include "ParticleLODLevel.h"
#include "UParticleEmitter.generated.h"

UCLASS(DisplayName="파티클 이미터", Description="파티클 이미터는 파티클 시스템의 개별 방출기입니다")
class UParticleEmitter : public UObject
{
public:
	GENERATED_REFLECTION_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Emitter")
	TArray<UParticleLODLevel*> LODLevels;

	// 캐시된 파티클 크기
	int32 ParticleSize = 0;

	UParticleEmitter() = default;
	virtual ~UParticleEmitter();

	// 이미터 모듈 정보 캐싱
	void CacheEmitterModuleInfo();

	// 인덱스로 LOD 레벨 가져오기
	UParticleLODLevel* GetLODLevel(int32 LODIndex) const;

	// 직렬화
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// 복제
	virtual void DuplicateSubObjects() override;

	UPROPERTY(EditAnywhere, Category = "LOD")
	bool bUseLOD = true;

	UPROPERTY(EditAnywhere, Category = "LOD")
	TArray<float> LODDistances;
};
