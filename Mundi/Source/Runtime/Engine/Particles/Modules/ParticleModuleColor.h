#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleColor.generated.h"

// 언리얼 엔진 호환: 페이로드 시스템 - 색상 모듈 (32바이트, 16바이트 정렬)
// 파티클별 색상 데이터를 저장 (UniformCurve용 랜덤 비율)
struct FParticleColorPayload
{
	FVector RGBRandomFactor;      // UniformCurve용 RGB 랜덤 비율 (0~1) (12바이트)
	float AlphaRandomFactor;      // UniformCurve용 Alpha 랜덤 비율 (0~1) (4바이트)
	float Padding[4];             // 정렬 패딩 (16바이트)
	// 총 32바이트
};
static_assert(sizeof(FParticleColorPayload) == 32, "FParticleColorPayload must be 32 bytes");
// WARNING : 페이로드는 항상 16바이트의 배수여야 함!!

UCLASS(DisplayName="색상 모듈", Description="파티클의 색상을 정의하며, 시간에 따른 색상 변화를 설정할 수 있습니다")
class UParticleModuleColor : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 파티클 색상 (Distribution 시스템: Curve로 시간에 따른 변화 정의)
	UPROPERTY(EditAnywhere, Category="Color")
	FDistributionColor ColorOverLife = FDistributionColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));

	UParticleModuleColor()
	{
		bSpawnModule = true;   // 스폰 시 초기 색상 설정
		bUpdateModule = true;  // 매 프레임 색상 보간
	}
	virtual ~UParticleModuleColor() = default;

	// 언리얼 엔진 호환: 페이로드 시스템
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = nullptr) override;

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FModuleUpdateContext& Context) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
