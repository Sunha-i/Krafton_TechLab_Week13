#pragma once

#include "FParticleBurst.generated.h"

/**
 * FParticleBurst - 버스트 정보 구조체
 * 특정 시점에 한 번에 많은 파티클을 생성하는 버스트 설정
 */
USTRUCT(DisplayName="파티클 버스트", Description="버스트 시점과 생성량을 설정합니다")
struct FParticleBurst
{
	GENERATED_REFLECTION_BODY()

public:
	// 버스트 시 생성할 파티클 개수
	UPROPERTY(EditAnywhere, Category="Burst", Tooltip="버스트 시 생성할 파티클 개수")
	int32 Count = 0;

	// 랜덤 범위 최소값 (-1이면 Count 사용, 아니면 CountLow~Count 랜덤)
	UPROPERTY(EditAnywhere, Category="Burst", Tooltip="랜덤 범위 최소값 (-1이면 Count 고정)")
	int32 CountLow = -1;

	// 버스트 발생 시점 (0..1: 이미터 수명 비율, Duration=0이면 절대 초)
	UPROPERTY(EditAnywhere, Category="Burst", Range="0.0, 1.0", Tooltip="버스트 발생 시점 (0~1)")
	float Time = 0.0f;

	FParticleBurst() = default;
	FParticleBurst(int32 InCount, float InTime) : Count(InCount), CountLow(-1), Time(InTime) {}
	FParticleBurst(int32 InCount, int32 InCountLow, float InTime) : Count(InCount), CountLow(InCountLow), Time(InTime) {}

	void Serialize(bool bIsLoading, JSON& InOutHandle);
};
