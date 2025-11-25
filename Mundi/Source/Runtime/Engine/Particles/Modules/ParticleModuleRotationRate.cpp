#include "pch.h"
#include "ParticleModuleRotationRate.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"

void UParticleModuleRotationRate::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner || !Owner->Component)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleRotationRatePayload, Payload);

	// Distribution 시스템을 사용하여 초기 회전 속도 계산
	float InitialRate = StartRotationRate.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.InitialRotationRate = InitialRate;

	// Distribution 시스템을 사용하여 목표 회전 속도 계산 (RotationRateOverLife용)
	float TargetRate = EndRotationRate.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.TargetRotationRate = TargetRate;

	// 파티클의 초기 회전 속도 설정
	ParticleBase->RotationRate = InitialRate;
	ParticleBase->BaseRotationRate = InitialRate;

	// RotationRateOverLife 활성화 시 Update 모듈로 등록
	if (bUseRotationRateOverLife)
	{
		bUpdateModule = true;
	}
}

void UParticleModuleRotationRate::Update(FModuleUpdateContext& Context)
{
	// RotationRateOverLife가 비활성화되어 있으면 업데이트 불필요
	if (!bUseRotationRateOverLife)
	{
		return;
	}

	// 언리얼 엔진 방식: 모든 파티클 업데이트
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleRotationRatePayload, Payload);

		// 수명에 따라 회전 속도 선형 보간 (InitialRotationRate -> TargetRotationRate)
		float InterpolatedRotationRate = Payload.InitialRotationRate +
			(Payload.TargetRotationRate - Payload.InitialRotationRate) * Particle.RelativeTime;

		// 회전 속도 업데이트
		Particle.RotationRate = InterpolatedRotationRate;
	END_UPDATE_LOOP;
}

void UParticleModuleRotationRate::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		if (FJsonSerializer::ReadObject(InOutHandle, "StartRotationRate", TempJson))
			StartRotationRate.Serialize(true, TempJson);
		FJsonSerializer::ReadBool(InOutHandle, "bUseRotationRateOverLife", bUseRotationRateOverLife);
		if (FJsonSerializer::ReadObject(InOutHandle, "EndRotationRate", TempJson))
			EndRotationRate.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		StartRotationRate.Serialize(false, TempJson);
		InOutHandle["StartRotationRate"] = TempJson;

		InOutHandle["bUseRotationRateOverLife"] = bUseRotationRateOverLife;

		TempJson = JSON::Make(JSON::Class::Object);
		EndRotationRate.Serialize(false, TempJson);
		InOutHandle["EndRotationRate"] = TempJson;
	}
}
