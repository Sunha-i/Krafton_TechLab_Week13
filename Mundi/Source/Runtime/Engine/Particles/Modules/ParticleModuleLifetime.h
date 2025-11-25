#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleLifetime.generated.h"

UCLASS(DisplayName="라이프타임 모듈", Description="파티클의 수명을 설정하는 모듈입니다")
class UParticleModuleLifetime : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 파티클 수명 (Distribution 시스템 사용)
	UPROPERTY(EditAnywhere, Category="Lifetime")
	FDistributionFloat Lifetime = FDistributionFloat(1.0f);

	UParticleModuleLifetime()
	{
		bSpawnModule = true;  // 스폰 시 수명 설정
	}
	virtual ~UParticleModuleLifetime() = default;

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
