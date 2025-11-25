#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleRotationRate.generated.h"

// 언리얼 엔진 호환: 파티클별 회전 속도 페이로드 (16바이트, 16바이트 정렬)
struct FParticleRotationRatePayload
{
	float InitialRotationRate;      // 초기 회전 속도 (라디안/초) (4바이트)
	float TargetRotationRate;       // 목표 회전 속도 (RotationRateOverLife용) (4바이트)
	float RandomFactor;             // UniformCurve용 랜덤 비율 (0~1) (4바이트)
	float Padding;                  // 정렬 패딩 (4바이트)
	// 총 16바이트
};

UCLASS(DisplayName="회전 속도", Description="파티클의 회전 속도를 설정하는 모듈입니다")
class UParticleModuleRotationRate : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 시작 회전 속도 (라디안/초) - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="Rotation")
	FDistributionFloat StartRotationRate = FDistributionFloat(0.0f);

	// 수명에 따른 회전 속도 변화 활성화
	UPROPERTY(EditAnywhere, Category="Rotation")
	bool bUseRotationRateOverLife = false;

	// 수명 종료 시 회전 속도 (라디안/초) - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="Rotation")
	FDistributionFloat EndRotationRate = FDistributionFloat(0.0f);

	UParticleModuleRotationRate()
	{
		bSpawnModule = true;   // Spawn 시 초기화 필요
		bUpdateModule = false; // 기본적으로 Update 불필요
	}
	virtual ~UParticleModuleRotationRate() = default;

	// 언리얼 엔진 호환: 파티클별 페이로드 크기 반환
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = nullptr) override
	{
		return sizeof(FParticleRotationRatePayload);
	}

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FModuleUpdateContext& Context) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
