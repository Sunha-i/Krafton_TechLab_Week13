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

	// LOD 레벨 복제 (ScaleMultiplier로 SpawnRate 등 자동 스케일링)
	// 반환: 새로 생성된 LOD 레벨 (LODLevels에 추가되지 않음, 호출자가 관리)
	UParticleLODLevel* DuplicateLODLevel(int32 SourceLODIndex, float ScaleMultiplier = 1.0f);

	// LOD 레벨 삭제 (최소 1개는 유지)
	// 반환: 삭제 성공 여부
	bool RemoveLODLevel(int32 LODIndex);

	// LOD 레벨 삽입
	void InsertLODLevel(int32 InsertIndex, UParticleLODLevel* NewLOD);

	// 직렬화
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// 복제
	virtual void DuplicateSubObjects() override;
};
