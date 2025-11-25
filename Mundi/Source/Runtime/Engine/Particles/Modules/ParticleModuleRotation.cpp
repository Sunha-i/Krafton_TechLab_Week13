#include "pch.h"
#include "ParticleModuleRotation.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"

void UParticleModuleRotation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner || !Owner->Component)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleRotationPayload, Payload);

	// Distribution 시스템을 사용하여 초기 회전값 계산
	float InitialRot = StartRotation.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.InitialRotation = InitialRot;

	// Distribution 시스템을 사용하여 목표 회전값 계산 (RotationOverLife용)
	float TargetRot = EndRotation.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.TargetRotation = TargetRot;

	// UniformCurve용 랜덤 비율 저장 (0~1)
	Payload.RandomFactor = Owner->RandomStream.GetFraction();

	// 파티클의 초기 회전 설정
	ParticleBase->Rotation = InitialRot;

	// RotationOverLife 활성화 시 Update 모듈로 등록
	if (bUseRotationOverLife)
	{
		bUpdateModule = true;
	}
}

void UParticleModuleRotation::Update(FModuleUpdateContext& Context)
{
	// RotationOverLife가 비활성화되어 있으면 업데이트 불필요
	if (!bUseRotationOverLife)
	{
		return;
	}

	// Distribution 타입 확인
	const EDistributionType DistType = StartRotation.Type;

	// 언리얼 엔진 방식: 모든 파티클 업데이트
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleRotationPayload, Payload);

		float CurrentRotation;

		switch (DistType)
		{
		case EDistributionType::ConstantCurve:
			CurrentRotation = StartRotation.ConstantCurve.Eval(Particle.RelativeTime);
			break;

		case EDistributionType::UniformCurve:
			{
				float MinAtTime = StartRotation.MinCurve.Eval(Particle.RelativeTime);
				float MaxAtTime = StartRotation.MaxCurve.Eval(Particle.RelativeTime);
				CurrentRotation = FMath::Lerp(MinAtTime, MaxAtTime, Payload.RandomFactor);
			}
			break;

		default:
			// Constant, Uniform, ParticleParameter: Initial -> Target 선형 보간
			CurrentRotation = Payload.InitialRotation +
				(Payload.TargetRotation - Payload.InitialRotation) * Particle.RelativeTime;
			break;
		}

		// 회전값 업데이트 (RotationRate는 건드리지 않음)
		Particle.Rotation = CurrentRotation;
	END_UPDATE_LOOP;
}

void UParticleModuleRotation::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		if (FJsonSerializer::ReadObject(InOutHandle, "StartRotation", TempJson))
			StartRotation.Serialize(true, TempJson);
		FJsonSerializer::ReadBool(InOutHandle, "bUseRotationOverLife", bUseRotationOverLife);
		if (FJsonSerializer::ReadObject(InOutHandle, "EndRotation", TempJson))
			EndRotation.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		StartRotation.Serialize(false, TempJson);
		InOutHandle["StartRotation"] = TempJson;

		InOutHandle["bUseRotationOverLife"] = bUseRotationOverLife;

		TempJson = JSON::Make(JSON::Class::Object);
		EndRotation.Serialize(false, TempJson);
		InOutHandle["EndRotation"] = TempJson;
	}
}
