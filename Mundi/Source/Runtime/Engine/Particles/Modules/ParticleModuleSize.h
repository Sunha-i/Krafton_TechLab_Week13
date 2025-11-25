#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleSize.generated.h"

// 언리얼 엔진 호환: 페이로드 시스템 - 크기 모듈 (48바이트, 16바이트 정렬)
// 파티클별 크기 데이터를 저장 (초기 크기, OverLife 배율 등)
struct FParticleSizePayload
{
	FVector InitialSize;          // 생성 시 초기 크기 (Distribution 샘플링 후) (12바이트)
	float SizeMultiplierOverLife; // 수명에 따른 크기 배율 (4바이트)
	FVector EndSize;              // 목표 크기 (OverLife 용) (12바이트)
	FVector RandomFactor;         // UniformCurve용 랜덤 비율 (0~1, 각 축별) (12바이트)
	float Padding[2];             // 정렬 패딩 (8바이트)
	// 총 48바이트
};

UCLASS(DisplayName="크기 모듈", Description="파티클의 크기를 설정하는 모듈입니다")
class UParticleModuleSize : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 파티클 시작 크기 (Distribution 시스템)
	UPROPERTY(EditAnywhere, Category="Size")
	FDistributionVector StartSize = FDistributionVector(FVector(1.0f, 1.0f, 1.0f));

	// 언리얼 엔진 호환: 크기 OverLife
	UPROPERTY(EditAnywhere, Category="Size")
	FDistributionVector EndSize = FDistributionVector(FVector(1.0f, 1.0f, 1.0f));

	UPROPERTY(EditAnywhere, Category="Size")
	bool bUseSizeOverLife = false;  // true이면 수명에 따라 크기 변화

	UParticleModuleSize()
	{
		bSpawnModule = true;   // 스폰 시 크기 설정
		bUpdateModule = false; // 기본적으로 업데이트 비활성 (bUseSizeOverLife가 true면 활성화)
	}
	virtual ~UParticleModuleSize() = default;

	// 언리얼 엔진 호환: 페이로드 시스템
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = nullptr) override;

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FModuleUpdateContext& Context) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
