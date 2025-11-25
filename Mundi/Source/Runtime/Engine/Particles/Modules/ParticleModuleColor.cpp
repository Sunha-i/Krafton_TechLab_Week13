#include "pch.h"
#include "ParticleModuleColor.h"
#include "ParticleEmitterInstance.h"  // BEGIN_UPDATE_LOOP 매크로에서 필요

// 언리얼 엔진 호환: 페이로드 크기 반환
uint32 UParticleModuleColor::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FParticleColorPayload);
}

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner)
	{
		return;
	}

	// Distribution 시스템을 사용하여 색상 계산
	FLinearColor InitialColor = StartColor.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	FLinearColor TargetColor = EndColor.GetValue(0.0f, Owner->RandomStream, Owner->Component);

	// 초기 색상 설정
	ParticleBase->Color = InitialColor;
	ParticleBase->BaseColor = InitialColor;

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
	PARTICLE_ELEMENT(FParticleColorPayload, ColorPayload);

	ColorPayload.InitialColor = InitialColor;
	ColorPayload.TargetColor = TargetColor;
	ColorPayload.ColorChangeRate = 1.0f;  // 기본 변화 속도

	// UniformCurve용 랜덤 비율 저장 (0~1)
	ColorPayload.RGBRandomFactor = FVector(
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction()
	);
	ColorPayload.AlphaRandomFactor = Owner->RandomStream.GetFraction();
}

// 언리얼 엔진 호환: Context를 사용한 업데이트 (페이로드 시스템)
void UParticleModuleColor::Update(FModuleUpdateContext& Context)
{
	// Distribution 타입 확인 (RGB와 Alpha 각각)
	const EDistributionType RGBDistType = StartColor.RGB.Type;
	const EDistributionType AlphaDistType = StartColor.Alpha.Type;

	BEGIN_UPDATE_LOOP
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleColorPayload, ColorPayload);

		FLinearColor CurrentColor;

		// RGB 처리
		switch (RGBDistType)
		{
		case EDistributionType::ConstantCurve:
			{
				FVector RGB = StartColor.RGB.ConstantCurve.Eval(Particle.RelativeTime);
				CurrentColor.R = RGB.X;
				CurrentColor.G = RGB.Y;
				CurrentColor.B = RGB.Z;
			}
			break;

		case EDistributionType::UniformCurve:
			{
				FVector MinRGB = StartColor.RGB.MinCurve.Eval(Particle.RelativeTime);
				FVector MaxRGB = StartColor.RGB.MaxCurve.Eval(Particle.RelativeTime);
				CurrentColor.R = FMath::Lerp(MinRGB.X, MaxRGB.X, ColorPayload.RGBRandomFactor.X);
				CurrentColor.G = FMath::Lerp(MinRGB.Y, MaxRGB.Y, ColorPayload.RGBRandomFactor.Y);
				CurrentColor.B = FMath::Lerp(MinRGB.Z, MaxRGB.Z, ColorPayload.RGBRandomFactor.Z);
			}
			break;

		default:
			{
				float Alpha = FMath::Clamp(Particle.RelativeTime, 0.0f, 1.0f);
				CurrentColor.R = FMath::Lerp(ColorPayload.InitialColor.R, ColorPayload.TargetColor.R, Alpha);
				CurrentColor.G = FMath::Lerp(ColorPayload.InitialColor.G, ColorPayload.TargetColor.G, Alpha);
				CurrentColor.B = FMath::Lerp(ColorPayload.InitialColor.B, ColorPayload.TargetColor.B, Alpha);
			}
			break;
		}

		// Alpha 처리
		switch (AlphaDistType)
		{
		case EDistributionType::ConstantCurve:
			CurrentColor.A = StartColor.Alpha.ConstantCurve.Eval(Particle.RelativeTime);
			break;

		case EDistributionType::UniformCurve:
			{
				float MinA = StartColor.Alpha.MinCurve.Eval(Particle.RelativeTime);
				float MaxA = StartColor.Alpha.MaxCurve.Eval(Particle.RelativeTime);
				CurrentColor.A = FMath::Lerp(MinA, MaxA, ColorPayload.AlphaRandomFactor);
			}
			break;

		default:
			{
				float Alpha = FMath::Clamp(Particle.RelativeTime, 0.0f, 1.0f);
				CurrentColor.A = FMath::Lerp(ColorPayload.InitialColor.A, ColorPayload.TargetColor.A, Alpha);
			}
			break;
		}

		Particle.Color = CurrentColor;
	END_UPDATE_LOOP
}

void UParticleModuleColor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		if (FJsonSerializer::ReadObject(InOutHandle, "StartColor", TempJson))
			StartColor.Serialize(true, TempJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "EndColor", TempJson))
			EndColor.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		StartColor.Serialize(false, TempJson);
		InOutHandle["StartColor"] = TempJson;

		TempJson = JSON::Make(JSON::Class::Object);
		EndColor.Serialize(false, TempJson);
		InOutHandle["EndColor"] = TempJson;
	}
}
