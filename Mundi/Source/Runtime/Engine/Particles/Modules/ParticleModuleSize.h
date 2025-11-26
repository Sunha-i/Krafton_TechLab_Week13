#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleSize.generated.h"

// 언리얼 엔진 호환: 페이로드 시스템 - 크기 모듈 (32바이트, 16바이트 정렬)
// 파티클별 크기 데이터를 저장 (UniformCurve용 랜덤 비율)
struct FParticleSizePayload
{
	FVector RandomFactor;         // UniformCurve용 랜덤 비율 (0~1, 각 축별) (12바이트)
	float Padding[5];             // 정렬 패딩 (20바이트)
	// 총 32바이트
};

UCLASS(DisplayName="크기 모듈", Description="파티클의 크기를 설정하는 모듈입니다")
class UParticleModuleSize : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 파티클 크기 (Distribution 시스템: Curve로 시간에 따른 변화 정의)
	UPROPERTY(EditAnywhere, Category="Size")
	FDistributionVector SizeOverLife = FDistributionVector(FVector(1.0f, 1.0f, 1.0f));

	UParticleModuleSize()
	{
		bSpawnModule = true;   // 스폰 시 크기 설정
		bUpdateModule = true;  // 매 프레임 크기 업데이트
	}
	virtual ~UParticleModuleSize() = default;

	// 언리얼 엔진 호환: 페이로드 시스템
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = nullptr) override;

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FModuleUpdateContext& Context) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
