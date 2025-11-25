#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleVelocity.generated.h"

// 언리얼 엔진 호환: 페이로드 시스템 예시
// 파티클별 추가 데이터를 저장하는 구조체
struct FParticleVelocityPayload
{
	FVector InitialVelocity;      // 생성 시 초기 속도 (Distribution 샘플링 후)
	float VelocityMagnitude;      // 속도 크기 (최적화: 매 프레임 계산 대신 저장)
	float Padding[2];             // 16바이트 정렬 유지 (32바이트 총 크기)
};

UCLASS(DisplayName="속도 모듈", Description="파티클의 초기 속도와 방향을 설정하는 모듈입니다")
class UParticleModuleVelocity : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 파티클 초기 속도 (Distribution 시스템)
	UPROPERTY(EditAnywhere, Category="Velocity", meta=(ToolTip="파티클 생성 시 초기 속도 (cm/s)"))
	FDistributionVector StartVelocity = FDistributionVector(FVector(0.0f, 0.0f, 0.0f));

	// 언리얼 엔진 호환: 속도 감쇠 (OverLife 패턴)
	UPROPERTY(EditAnywhere, Category="Velocity", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="수명에 따른 속도 감쇠 (0.0 = 감쇠 없음, 1.0 = 빠른 감쇠)"))
	float VelocityDamping = 0.0f;

	UParticleModuleVelocity()
	{
		bSpawnModule = true;   // 스폰 시 속도 설정
		bUpdateModule = true;  // 매 프레임 업데이트 (페이로드 활용)
	}
	virtual ~UParticleModuleVelocity() = default;

	// 언리얼 엔진 호환: 페이로드 시스템
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = nullptr) override;

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FModuleUpdateContext& Context) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
