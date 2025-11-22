#pragma once

#include "ParticleModule.h"
#include "UParticleModuleSpawn.generated.h"

UCLASS(DisplayName="스폰 모듈", Description="파티클의 생성 빈도와 수량을 제어하는 모듈입니다")
class UParticleModuleSpawn : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Spawn")
	float SpawnRate = 10.0f;

	UPROPERTY(EditAnywhere, Category="Spawn")
	int32 BurstCount = 0;

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
	int32 CalculateSpawnCount(float DeltaTime, float& InOutSpawnFraction, bool& bInOutBurstFired);

	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
