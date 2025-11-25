#include "pch.h"
#include "ParticleModuleAcceleration.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"

void UParticleModuleAcceleration::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!Owner || !Owner->Component)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleAccelerationPayload, Payload);

	// Distribution 시스템을 사용하여 가속도 계산
	FVector BaseAcceleration = Acceleration.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	if (bApplyGravity)
	{
		const float GravityZ = -980.0f; // cm/s^2
		BaseAcceleration.Z += GravityZ * GravityScale;
	}

	// 초기 가속도 저장
	Payload.InitialAcceleration = BaseAcceleration;

	// Distribution이 랜덤을 처리하므로 AccelerationRandomOffset은 사용 안함
	Payload.AccelerationRandomOffset = FVector(0.0f, 0.0f, 0.0f);

	// AccelerationOverLife 초기화
	Payload.AccelerationMultiplierOverLife = AccelerationMultiplierAtStart;
}

void UParticleModuleAcceleration::Update(FModuleUpdateContext& Context)
{
	// 언리얼 엔진 방식: 모든 파티클 업데이트 (Context 사용)
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleAccelerationPayload, Payload);

		// AccelerationOverLife 계산
		float CurrentMultiplier = Payload.AccelerationMultiplierOverLife;
		if (bUseAccelerationOverLife)
		{
			// 수명에 따라 선형 보간 (Start -> End)
			CurrentMultiplier = AccelerationMultiplierAtStart +
				(AccelerationMultiplierAtEnd - AccelerationMultiplierAtStart) * Particle.RelativeTime;
			Payload.AccelerationMultiplierOverLife = CurrentMultiplier;
		}

		// 최종 가속도 = (초기 가속도 + 랜덤 오프셋) * OverLife 배율
		FVector FinalAcceleration = (Payload.InitialAcceleration + Payload.AccelerationRandomOffset) * CurrentMultiplier;

		// 속도에 가속도 적용
		Particle.Velocity += FinalAcceleration * DeltaTime;
	END_UPDATE_LOOP;
}

void UParticleModuleAcceleration::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		if (FJsonSerializer::ReadObject(InOutHandle, "Acceleration", TempJson))
			Acceleration.Serialize(true, TempJson);
		FJsonSerializer::ReadBool(InOutHandle, "bApplyGravity", bApplyGravity);
		FJsonSerializer::ReadFloat(InOutHandle, "GravityScale", GravityScale);
		FJsonSerializer::ReadBool(InOutHandle, "bUseAccelerationOverLife", bUseAccelerationOverLife);
		FJsonSerializer::ReadFloat(InOutHandle, "AccelerationMultiplierAtStart", AccelerationMultiplierAtStart);
		FJsonSerializer::ReadFloat(InOutHandle, "AccelerationMultiplierAtEnd", AccelerationMultiplierAtEnd);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		Acceleration.Serialize(false, TempJson);
		InOutHandle["Acceleration"] = TempJson;

		InOutHandle["bApplyGravity"] = bApplyGravity;
		InOutHandle["GravityScale"] = GravityScale;
		InOutHandle["bUseAccelerationOverLife"] = bUseAccelerationOverLife;
		InOutHandle["AccelerationMultiplierAtStart"] = AccelerationMultiplierAtStart;
		InOutHandle["AccelerationMultiplierAtEnd"] = AccelerationMultiplierAtEnd;
	}
}
