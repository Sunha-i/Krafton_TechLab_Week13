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

	// 초기 가속도 저장
	Payload.InitialAcceleration = BaseAcceleration;

	// 목표 가속도 계산 (OverLife용)
	FVector TargetAccel = EndAcceleration.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.TargetAcceleration = TargetAccel;

	// UniformCurve용 랜덤 비율 저장
	Payload.RandomFactor = FVector(
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction()
	);

	// 중력 Z 성분 저장
	Payload.GravityZ = bApplyGravity ? (-9.8f * GravityScale) : 0.0f;

	// AccelerationOverLife 초기화
	Payload.AccelerationMultiplierOverLife = AccelerationMultiplierAtStart;
}

void UParticleModuleAcceleration::Update(FModuleUpdateContext& Context)
{
	// Distribution 타입 확인
	const EDistributionType DistType = Acceleration.Type;

	// 언리얼 엔진 방식: 모든 파티클 업데이트 (Context 사용)
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleAccelerationPayload, Payload);

		FVector CurrentAcceleration;

		// AccelerationOverLife가 활성화되어 있으면 커브/보간 평가
		if (bUseAccelerationOverLife)
		{
			switch (DistType)
			{
			case EDistributionType::ConstantCurve:
				CurrentAcceleration = Acceleration.ConstantCurve.Eval(Particle.RelativeTime);
				break;

			case EDistributionType::UniformCurve:
				{
					FVector MinAtTime = Acceleration.MinCurve.Eval(Particle.RelativeTime);
					FVector MaxAtTime = Acceleration.MaxCurve.Eval(Particle.RelativeTime);
					CurrentAcceleration.X = FMath::Lerp(MinAtTime.X, MaxAtTime.X, Payload.RandomFactor.X);
					CurrentAcceleration.Y = FMath::Lerp(MinAtTime.Y, MaxAtTime.Y, Payload.RandomFactor.Y);
					CurrentAcceleration.Z = FMath::Lerp(MinAtTime.Z, MaxAtTime.Z, Payload.RandomFactor.Z);
				}
				break;

			default:
				// Constant, Uniform, ParticleParameter: Initial -> Target 선형 보간
				{
					float Alpha = FMath::Clamp(Particle.RelativeTime, 0.0f, 1.0f);
					CurrentAcceleration.X = FMath::Lerp(Payload.InitialAcceleration.X, Payload.TargetAcceleration.X, Alpha);
					CurrentAcceleration.Y = FMath::Lerp(Payload.InitialAcceleration.Y, Payload.TargetAcceleration.Y, Alpha);
					CurrentAcceleration.Z = FMath::Lerp(Payload.InitialAcceleration.Z, Payload.TargetAcceleration.Z, Alpha);
				}
				break;
			}

			// Multiplier 적용
			float CurrentMultiplier = AccelerationMultiplierAtStart +
				(AccelerationMultiplierAtEnd - AccelerationMultiplierAtStart) * Particle.RelativeTime;
			CurrentAcceleration = CurrentAcceleration * CurrentMultiplier;
		}
		else
		{
			// OverLife 비활성화 시 초기 가속도 사용
			CurrentAcceleration = Payload.InitialAcceleration;
		}

		// 중력 추가
		CurrentAcceleration.Z += Payload.GravityZ;

		// 속도에 가속도 적용
		Particle.Velocity += CurrentAcceleration * DeltaTime;
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
		if (FJsonSerializer::ReadObject(InOutHandle, "EndAcceleration", TempJson))
			EndAcceleration.Serialize(true, TempJson);
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

		TempJson = JSON::Make(JSON::Class::Object);
		EndAcceleration.Serialize(false, TempJson);
		InOutHandle["EndAcceleration"] = TempJson;

		InOutHandle["bApplyGravity"] = bApplyGravity;
		InOutHandle["GravityScale"] = GravityScale;
		InOutHandle["bUseAccelerationOverLife"] = bUseAccelerationOverLife;
		InOutHandle["AccelerationMultiplierAtStart"] = AccelerationMultiplierAtStart;
		InOutHandle["AccelerationMultiplierAtEnd"] = AccelerationMultiplierAtEnd;
	}
}
