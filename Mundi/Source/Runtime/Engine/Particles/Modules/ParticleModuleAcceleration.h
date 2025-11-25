#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleAcceleration.generated.h"

// 언리얼 엔진 호환: 파티클별 가속도 페이로드 (64바이트, 16바이트 정렬)
struct FParticleAccelerationPayload
{
	FVector InitialAcceleration;          // 초기 가속도 (Distribution 샘플링 후) (12바이트)
	FVector TargetAcceleration;           // 목표 가속도 (OverLife용) (12바이트)
	FVector RandomFactor;                 // UniformCurve용 랜덤 비율 (0~1) (12바이트)
	float AccelerationMultiplierOverLife; // 수명에 따른 가속도 배율 (4바이트)
	float GravityZ;                       // 중력 Z 성분 (캐싱) (4바이트)
	float Padding[5];                     // 정렬 패딩 (20바이트)
	// 총 64바이트
};

UCLASS(DisplayName="가속도 모듈", Description="파티클에 가속도(중력 포함)를 적용하는 모듈입니다")
class UParticleModuleAcceleration : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 기본 가속도 벡터 (Distribution 시스템)
	UPROPERTY(EditAnywhere, Category="Acceleration")
	FDistributionVector Acceleration = FDistributionVector(FVector(0.0f, 0.0f, 0.0f));

	// 수명 종료 시 가속도 (Distribution 시스템)
	UPROPERTY(EditAnywhere, Category="Acceleration")
	FDistributionVector EndAcceleration = FDistributionVector(FVector(0.0f, 0.0f, 0.0f));

	// 중력 적용 여부
	UPROPERTY(EditAnywhere, Category="Acceleration")
	bool bApplyGravity = false;

	// 중력 스케일 (1.0 = 기본 중력)
	UPROPERTY(EditAnywhere, Category="Acceleration")
	float GravityScale = 1.0f;

	// 언리얼 엔진 호환: 수명에 따른 가속도 변화 활성화
	UPROPERTY(EditAnywhere, Category="Acceleration")
	bool bUseAccelerationOverLife = false;

	// 수명 시작 시 가속도 배율 (0.0 ~ 1.0)
	UPROPERTY(EditAnywhere, Category="Acceleration")
	float AccelerationMultiplierAtStart = 1.0f;

	// 수명 종료 시 가속도 배율 (0.0 ~ 1.0)
	UPROPERTY(EditAnywhere, Category="Acceleration")
	float AccelerationMultiplierAtEnd = 1.0f;

	UParticleModuleAcceleration()
	{
		bUpdateModule = true;  // 매 프레임 업데이트 필요
		bSpawnModule = true;   // Spawn 시 초기화 필요
	}
	virtual ~UParticleModuleAcceleration() = default;

	// 언리얼 엔진 호환: 파티클별 페이로드 크기 반환
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = nullptr) override
	{
		return sizeof(FParticleAccelerationPayload);
	}

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FModuleUpdateContext& Context) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
