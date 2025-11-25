#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleSpawn.generated.h"

// Forward declaration
class FParticleEmitterInstance;

UCLASS(DisplayName="스폰 모듈", Description="파티클의 생성 빈도와 수량을 제어하는 모듈입니다")
class UParticleModuleSpawn : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 초당 파티클 스폰 비율 (Distribution 시스템)
	UPROPERTY(EditAnywhere, Category="Spawn")
	FDistributionFloat SpawnRate = FDistributionFloat(10.0f);

	// 최초 버스트 개수 (Distribution 시스템)
	UPROPERTY(EditAnywhere, Category="Burst")
	FDistributionFloat BurstCount = FDistributionFloat(0.0f);

	UParticleModuleSpawn()
	{
		// 스폰 모듈은 항상 bSpawnModule = true로 설정되어야
		// CacheModuleInfo()에서 SpawnModules 배열에 추가됨
		bSpawnModule = true;
	}
	virtual ~UParticleModuleSpawn() = default;

	// 언리얼 엔진 호환: 스폰 계산 로직을 모듈로 캡슐화
	// 반환값: 이번 프레임에 생성할 파티클 수
	// InOutSpawnFraction: 누적된 스폰 분수 (부드러운 스폰용)
	// bInOutBurstFired: Burst 발생 여부 (1회만 발생)
	int32 CalculateSpawnCount(FParticleEmitterInstance* Owner, float DeltaTime, float& InOutSpawnFraction, bool& bInOutBurstFired);

	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
