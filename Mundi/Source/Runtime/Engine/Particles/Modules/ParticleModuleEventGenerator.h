#pragma once

#include "ParticleModule.h"
#include "ParticleEventGeneratorTypes.h"
#include "UParticleModuleEventGenerator.generated.h"

/**
 * UParticleModuleEventGenerator
 *
 * 파티클 이벤트를 생성하는 모듈입니다.
 * 이 모듈은 파티클의 Spawn, Death, Collision 이벤트를 생성하여
 * 다른 이미터나 외부 시스템에서 반응할 수 있도록 합니다.
 *
 * 사용법:
 * 1. 이미터에 이 모듈을 추가합니다.
 * 2. Events 배열에 생성할 이벤트 타입을 설정합니다.
 * 3. EventReceiver 모듈이 있는 다른 이미터에서 이벤트를 수신합니다.
 */
UCLASS(DisplayName="이벤트 생성 모듈", Description="파티클 이벤트(Spawn/Death/Collision)를 생성하는 모듈입니다")
class UParticleModuleEventGenerator : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 생성할 이벤트 목록
	UPROPERTY(EditAnywhere, Category="Events", Tooltip="생성할 이벤트 목록")
	TArray<FParticleEventGeneratorInfo> Events;

	UParticleModuleEventGenerator()
	{
		bSpawnModule = true;   // 스폰 이벤트 생성을 위해
		bUpdateModule = true;  // 사망/충돌 이벤트 체크를 위해
	}

	virtual ~UParticleModuleEventGenerator() = default;

	// 스폰 시 이벤트 생성
	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

	// 업데이트 시 사망/충돌 이벤트 체크
	virtual void Update(FModuleUpdateContext& Context) override;

	// 직렬화
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
	// 특정 타입의 이벤트 생성 정보 가져오기
	FParticleEventGeneratorInfo* GetEventInfo(EParticleEventType Type);

	// 이벤트 데이터 생성 헬퍼
	FParticleEventData CreateEventData(EParticleEventType Type, const FBaseParticle& Particle, float EmitterTime);
};
