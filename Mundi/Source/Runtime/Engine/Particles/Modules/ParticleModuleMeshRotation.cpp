#include "pch.h"
#include "ParticleModuleMeshRotation.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"

void UParticleModuleMeshRotation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner || !Owner->Component)
	{
		return;
	}

	// CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FMeshRotationPayload, Payload);

	// Distribution 시스템을 사용하여 초기 회전값 계산
	FVector InitialRot = StartRotation.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.InitialRotation = InitialRot;
	Payload.Rotation = InitialRot;

	// 목표 회전값 계산 (OverLife용)
	FVector TargetRot = EndRotation.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.TargetRotation = TargetRot;

	// UniformCurve용 랜덤 비율 저장
	Payload.RotationRandomFactor = FVector(
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction()
	);

	// Distribution 시스템을 사용하여 회전 속도 계산
	FVector InitialRotRate = StartRotationRate.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.InitialRotationRate = InitialRotRate;
	Payload.RotationRate = InitialRotRate;

	// 목표 회전 속도 계산 (OverLife용)
	FVector TargetRotRate = EndRotationRate.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.TargetRotationRate = TargetRotRate;

	// RotationRate UniformCurve용 랜덤 비율 저장
	Payload.RateRandomFactor = FVector(
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction()
	);

	// 기본 Z축 회전도 동기화 (스프라이트 호환)
	ParticleBase->Rotation = Payload.Rotation.Z;
	ParticleBase->RotationRate = Payload.RotationRate.Z;
}

void UParticleModuleMeshRotation::Update(FModuleUpdateContext& Context)
{
	// Distribution 타입 확인
	const EDistributionType RotDistType = StartRotation.Type;
	const EDistributionType RateDistType = StartRotationRate.Type;

	BEGIN_UPDATE_LOOP;
		// 페이로드 가져오기
		PARTICLE_ELEMENT(FMeshRotationPayload, Payload);

		// RotationOverLife 처리
		if (bUseRotationOverLife)
		{
			FVector CurrentRotation;

			switch (RotDistType)
			{
			case EDistributionType::ConstantCurve:
				CurrentRotation = StartRotation.ConstantCurve.Eval(Particle.RelativeTime);
				break;

			case EDistributionType::UniformCurve:
				{
					FVector MinAtTime = StartRotation.MinCurve.Eval(Particle.RelativeTime);
					FVector MaxAtTime = StartRotation.MaxCurve.Eval(Particle.RelativeTime);
					CurrentRotation.X = FMath::Lerp(MinAtTime.X, MaxAtTime.X, Payload.RotationRandomFactor.X);
					CurrentRotation.Y = FMath::Lerp(MinAtTime.Y, MaxAtTime.Y, Payload.RotationRandomFactor.Y);
					CurrentRotation.Z = FMath::Lerp(MinAtTime.Z, MaxAtTime.Z, Payload.RotationRandomFactor.Z);
				}
				break;

			default:
				// Constant, Uniform, ParticleParameter: Initial -> Target 선형 보간
				{
					float Alpha = FMath::Clamp(Particle.RelativeTime, 0.0f, 1.0f);
					CurrentRotation.X = FMath::Lerp(Payload.InitialRotation.X, Payload.TargetRotation.X, Alpha);
					CurrentRotation.Y = FMath::Lerp(Payload.InitialRotation.Y, Payload.TargetRotation.Y, Alpha);
					CurrentRotation.Z = FMath::Lerp(Payload.InitialRotation.Z, Payload.TargetRotation.Z, Alpha);
				}
				break;
			}

			Payload.Rotation = CurrentRotation;
		}

		// RotationRateOverLife 처리
		FVector CurrentRotationRate = Payload.RotationRate;
		if (bUseRotationRateOverLife)
		{
			switch (RateDistType)
			{
			case EDistributionType::ConstantCurve:
				CurrentRotationRate = StartRotationRate.ConstantCurve.Eval(Particle.RelativeTime);
				break;

			case EDistributionType::UniformCurve:
				{
					FVector MinAtTime = StartRotationRate.MinCurve.Eval(Particle.RelativeTime);
					FVector MaxAtTime = StartRotationRate.MaxCurve.Eval(Particle.RelativeTime);
					CurrentRotationRate.X = FMath::Lerp(MinAtTime.X, MaxAtTime.X, Payload.RateRandomFactor.X);
					CurrentRotationRate.Y = FMath::Lerp(MinAtTime.Y, MaxAtTime.Y, Payload.RateRandomFactor.Y);
					CurrentRotationRate.Z = FMath::Lerp(MinAtTime.Z, MaxAtTime.Z, Payload.RateRandomFactor.Z);
				}
				break;

			default:
				// Constant, Uniform, ParticleParameter: Initial -> Target 선형 보간
				{
					float Alpha = FMath::Clamp(Particle.RelativeTime, 0.0f, 1.0f);
					CurrentRotationRate.X = FMath::Lerp(Payload.InitialRotationRate.X, Payload.TargetRotationRate.X, Alpha);
					CurrentRotationRate.Y = FMath::Lerp(Payload.InitialRotationRate.Y, Payload.TargetRotationRate.Y, Alpha);
					CurrentRotationRate.Z = FMath::Lerp(Payload.InitialRotationRate.Z, Payload.TargetRotationRate.Z, Alpha);
				}
				break;
			}

			Payload.RotationRate = CurrentRotationRate;
		}

		// RotationOverLife가 아니면 회전 속도로 회전 적용
		if (!bUseRotationOverLife)
		{
			Payload.Rotation.X += CurrentRotationRate.X * DeltaTime;
			Payload.Rotation.Y += CurrentRotationRate.Y * DeltaTime;
			Payload.Rotation.Z += CurrentRotationRate.Z * DeltaTime;
		}

		// 기본 Z축 회전도 동기화
		Particle.Rotation = Payload.Rotation.Z;
	END_UPDATE_LOOP;
}

void UParticleModuleMeshRotation::Serialize(const bool bInIsLoading, JSON& InOutHandle)
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
		if (FJsonSerializer::ReadObject(InOutHandle, "StartRotationRate", TempJson))
			StartRotationRate.Serialize(true, TempJson);
		FJsonSerializer::ReadBool(InOutHandle, "bUseRotationRateOverLife", bUseRotationRateOverLife);
		if (FJsonSerializer::ReadObject(InOutHandle, "EndRotationRate", TempJson))
			EndRotationRate.Serialize(true, TempJson);
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

		TempJson = JSON::Make(JSON::Class::Object);
		StartRotationRate.Serialize(false, TempJson);
		InOutHandle["StartRotationRate"] = TempJson;

		InOutHandle["bUseRotationRateOverLife"] = bUseRotationRateOverLife;

		TempJson = JSON::Make(JSON::Class::Object);
		EndRotationRate.Serialize(false, TempJson);
		InOutHandle["EndRotationRate"] = TempJson;
	}
}
