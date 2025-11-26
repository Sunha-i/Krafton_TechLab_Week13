#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleRotationRate.generated.h"

// 언리얼 엔진 호환: 파티클별 회전 속도 페이로드 (16바이트, 16바이트 정렬)
struct FParticleRotationRatePayload
{
	float RandomFactor;             // UniformCurve용 랜덤 비율 (0~1) (4바이트)
	float Padding[3];               // 정렬 패딩 (12바이트)
	// 총 16바이트
};

UCLASS(DisplayName="회전 속도", Description="파티클의 회전 속도를 설정하는 모듈입니다")
class UParticleModuleRotationRate : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 회전 속도 (라디안/초) - Distribution 시스템: Curve로 시간에 따른 변화 정의
	UPROPERTY(EditAnywhere, Category="Rotation")
	FDistributionFloat RotationRateOverLife = FDistributionFloat(0.0f);

	UParticleModuleRotationRate()
	{
		bSpawnModule = true;   // Spawn 시 초기화 필요
		bUpdateModule = true;  // 매 프레임 업데이트 필요
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
