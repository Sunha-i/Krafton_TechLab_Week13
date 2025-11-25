#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleColor.generated.h"

// 언리얼 엔진 호환: 페이로드 시스템 - 색상 모듈 (64바이트, 16바이트 정렬)
// 파티클별 색상 데이터를 저장 (Distribution 샘플링 결과, OverLife 최적화 등)
struct FParticleColorPayload
{
	FLinearColor InitialColor;    // 생성 시 초기 색상 (Distribution 샘플링 후) (16바이트)
	FLinearColor TargetColor;     // 목표 색상 (Distribution 샘플링 후) (16바이트)
	float ColorChangeRate;        // 색상 변화 속도 (OverLife 최적화) (4바이트)
	FVector RGBRandomFactor;      // UniformCurve용 RGB 랜덤 비율 (0~1) (12바이트)
	float AlphaRandomFactor;      // UniformCurve용 Alpha 랜덤 비율 (0~1) (4바이트)
	float Padding[3];             // 정렬 패딩 (12바이트)
	// 총 64바이트
};

UCLASS(DisplayName="색상 모듈", Description="파티클의 색상을 정의하며, 시간에 따른 색상 변화를 설정할 수 있습니다")
class UParticleModuleColor : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 파티클 시작 색상 (Distribution 시스템: RGB + Alpha 분리)
	UPROPERTY(EditAnywhere, Category="Color")
	FDistributionColor StartColor = FDistributionColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));

	// 파티클 끝 색상 (Distribution 시스템)
	UPROPERTY(EditAnywhere, Category="Color")
	FDistributionColor EndColor = FDistributionColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));

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
