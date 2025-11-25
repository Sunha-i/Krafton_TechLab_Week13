#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleRotation.generated.h"

// 언리얼 엔진 호환: 파티클별 회전 페이로드 (16바이트, 16바이트 정렬)
struct FParticleRotationPayload
{
	float InitialRotation;      // 초기 회전값 (라디안) (4바이트)
	float TargetRotation;       // 목표 회전값 (RotationOverLife용) (4바이트)
	float Padding0;             // 정렬 패딩 (4바이트)
	float Padding1;             // 정렬 패딩 (4바이트)
	// 총 16바이트
};

UCLASS(DisplayName="초기 회전", Description="파티클의 초기 회전값을 설정하는 모듈입니다")
class UParticleModuleRotation : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 시작 회전값 (라디안) - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="Rotation")
	FDistributionFloat StartRotation = FDistributionFloat(0.0f);

	// 수명에 따른 회전 변화 활성화
	UPROPERTY(EditAnywhere, Category="Rotation")
	bool bUseRotationOverLife = false;

	// 수명 종료 시 회전값 (라디안) - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="Rotation")
	FDistributionFloat EndRotation = FDistributionFloat(0.0f);

	UParticleModuleRotation()
	{
		bSpawnModule = true;   // Spawn 시 초기화 필요
		bUpdateModule = false; // 기본적으로 Update 불필요
	}
	virtual ~UParticleModuleRotation() = default;

	// 언리얼 엔진 호환: 파티클별 페이로드 크기 반환
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = nullptr) override
	{
		return sizeof(FParticleRotationPayload);
	}

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FModuleUpdateContext& Context) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
