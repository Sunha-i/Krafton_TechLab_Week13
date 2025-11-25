#pragma once

#include "ParticleModule.h"
#include "Source/Runtime/Engine/Particles/Distribution.h"
#include "UParticleModuleMeshRotation.generated.h"

// 메시 파티클용 3D 회전 페이로드 (80바이트)
// 스프라이트는 Z축만 사용하지만, 메시는 3축 회전 필요
struct FMeshRotationPayload
{
	FVector InitialRotation;      // 초기 회전값 (라디안) - X:Roll, Y:Pitch, Z:Yaw (12바이트)
	FVector TargetRotation;       // 목표 회전값 (OverLife용) (12바이트)
	FVector Rotation;             // 현재 회전값 (라디안) - X:Roll, Y:Pitch, Z:Yaw (12바이트)
	FVector InitialRotationRate;  // 초기 회전 속도 (라디안/초) (12바이트)
	FVector TargetRotationRate;   // 목표 회전 속도 (OverLife용) (12바이트)
	FVector RotationRate;         // 현재 회전 속도 (라디안/초) (12바이트)
	FVector RotationRandomFactor; // UniformCurve용 랜덤 비율 (0~1) (12바이트)
	FVector RateRandomFactor;     // RotationRate UniformCurve용 랜덤 비율 (12바이트)
	// 총 96바이트
};

UCLASS(DisplayName="메시 초기 회전", Description="메시 파티클의 3축 초기 회전값을 설정하는 모듈입니다")
class UParticleModuleMeshRotation : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 시작 회전값 (라디안) - X: Roll, Y: Pitch, Z: Yaw
	// Distribution 시스템: Constant, Uniform(랜덤), Curve 등 지원
	UPROPERTY(EditAnywhere, Category="Rotation")
	FDistributionVector StartRotation;

	// 수명에 따른 회전 변화 활성화
	UPROPERTY(EditAnywhere, Category="Rotation")
	bool bUseRotationOverLife = false;

	// 수명 종료 시 회전값 (라디안) - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="Rotation")
	FDistributionVector EndRotation;

	// 회전 속도 (라디안/초)
	// Distribution 시스템: Constant, Uniform(랜덤), Curve 등 지원
	UPROPERTY(EditAnywhere, Category="Rotation")
	FDistributionVector StartRotationRate;

	// 수명에 따른 회전 속도 변화 활성화
	UPROPERTY(EditAnywhere, Category="RotationRate")
	bool bUseRotationRateOverLife = false;

	// 수명 종료 시 회전 속도 (라디안/초) - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="RotationRate")
	FDistributionVector EndRotationRate;

	UParticleModuleMeshRotation()
	{
		bSpawnModule = true;   // Spawn 시 초기화 필요
		bUpdateModule = true;  // Update에서 회전 적용
	}
	virtual ~UParticleModuleMeshRotation() = default;

	// 파티클별 페이로드 크기 반환
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = nullptr) override
	{
		return sizeof(FMeshRotationPayload);
	}

	// 메시 회전을 건드리는 모듈임을 표시
	virtual bool TouchesMeshRotation() const { return true; }

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FModuleUpdateContext& Context) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
