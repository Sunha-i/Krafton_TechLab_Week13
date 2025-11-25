#include "pch.h"
#include "ParticleModuleSize.h"
#include "Source/Runtime/Engine/Particles/ParticleEmitterInstance.h" // BEGIN_UPDATE_LOOP 매크로에서 필요
#include "ParticleSystemComponent.h"

// 언리얼 엔진 호환: 페이로드 크기 반환
uint32 UParticleModuleSize::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FParticleSizePayload);
}

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner)
	{
		return;
	}

	// Distribution 시스템을 사용하여 크기 계산
	FVector LocalInitialSize = StartSize.GetValue(0.0f, Owner->RandomStream, Owner->Component);

	// 음수 크기 방지
	LocalInitialSize.X = FMath::Max(LocalInitialSize.X, 0.01f);
	LocalInitialSize.Y = FMath::Max(LocalInitialSize.Y, 0.01f);
	LocalInitialSize.Z = FMath::Max(LocalInitialSize.Z, 0.01f);

	const float ComponentScaleX = Owner->Component->GetWorldScale().X;
	FVector FinalWorldScale = LocalInitialSize * ComponentScaleX;  // 균일 스케일 적용

	ParticleBase->Size = FinalWorldScale;
	ParticleBase->BaseSize = FinalWorldScale;

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
	PARTICLE_ELEMENT(FParticleSizePayload, SizePayload);

	SizePayload.InitialSize = FinalWorldScale;
	// EndSize도 Distribution에서 샘플링
	FVector LocalEndSize = EndSize.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	SizePayload.EndSize = LocalEndSize * ComponentScaleX;
	SizePayload.SizeMultiplierOverLife = 1.0f;  // 기본 배율

	// UniformCurve용 랜덤 비율 저장 (0~1, 각 축별)
	SizePayload.RandomFactor = FVector(
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction()
	);

	// SizeOverLife 활성화 시 Update 모듈로 동작
	if (bUseSizeOverLife)
	{
		// bUpdateModule은 const가 아니므로 동적으로 설정 가능 (실제로는 생성자에서 설정하는 것이 더 좋음)
		const_cast<UParticleModuleSize*>(this)->bUpdateModule = true;
	}
}

// 언리얼 엔진 호환: Context를 사용한 업데이트 (페이로드 시스템)
void UParticleModuleSize::Update(FModuleUpdateContext& Context)
{
	// SizeOverLife가 비활성화면 업데이트 불필요
	if (!bUseSizeOverLife)
	{
		return;
	}

	// Distribution 타입 확인
	const EDistributionType DistType = StartSize.Type;
	const float ComponentScaleX = Context.Owner.Component->GetWorldScale().X;

	BEGIN_UPDATE_LOOP
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleSizePayload, SizePayload);

		FVector CurrentSizeVec;

		switch (DistType)
		{
		case EDistributionType::ConstantCurve:
			// ConstantCurve: RelativeTime에 따라 커브 평가
			CurrentSizeVec = StartSize.ConstantCurve.Eval(Particle.RelativeTime);
			CurrentSizeVec = CurrentSizeVec * ComponentScaleX;
			break;

		case EDistributionType::UniformCurve:
			{
				// UniformCurve: Min/Max 커브 평가 후 저장된 랜덤 비율로 보간
				FVector MinAtTime = StartSize.MinCurve.Eval(Particle.RelativeTime);
				FVector MaxAtTime = StartSize.MaxCurve.Eval(Particle.RelativeTime);
				CurrentSizeVec.X = FMath::Lerp(MinAtTime.X, MaxAtTime.X, SizePayload.RandomFactor.X);
				CurrentSizeVec.Y = FMath::Lerp(MinAtTime.Y, MaxAtTime.Y, SizePayload.RandomFactor.Y);
				CurrentSizeVec.Z = FMath::Lerp(MinAtTime.Z, MaxAtTime.Z, SizePayload.RandomFactor.Z);
				CurrentSizeVec = CurrentSizeVec * ComponentScaleX;
			}
			break;

		default:
			// Constant, Uniform, ParticleParameter: Initial -> End 선형 보간
			{
				float Alpha = FMath::Clamp(Particle.RelativeTime, 0.0f, 1.0f);
				CurrentSizeVec.X = FMath::Lerp(SizePayload.InitialSize.X, SizePayload.EndSize.X, Alpha);
				CurrentSizeVec.Y = FMath::Lerp(SizePayload.InitialSize.Y, SizePayload.EndSize.Y, Alpha);
				CurrentSizeVec.Z = FMath::Lerp(SizePayload.InitialSize.Z, SizePayload.EndSize.Z, Alpha);
			}
			break;
		}

		// 음수 크기 방지
		Particle.Size.X = FMath::Max(CurrentSizeVec.X, 0.01f);
		Particle.Size.Y = FMath::Max(CurrentSizeVec.Y, 0.01f);
		Particle.Size.Z = FMath::Max(CurrentSizeVec.Z, 0.01f);
	END_UPDATE_LOOP
}

void UParticleModuleSize::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		if (FJsonSerializer::ReadObject(InOutHandle, "StartSize", TempJson))
			StartSize.Serialize(true, TempJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "EndSize", TempJson))
			EndSize.Serialize(true, TempJson);
		FJsonSerializer::ReadBool(InOutHandle, "bUseSizeOverLife", bUseSizeOverLife);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		StartSize.Serialize(false, TempJson);
		InOutHandle["StartSize"] = TempJson;

		TempJson = JSON::Make(JSON::Class::Object);
		EndSize.Serialize(false, TempJson);
		InOutHandle["EndSize"] = TempJson;

		InOutHandle["bUseSizeOverLife"] = bUseSizeOverLife;
	}
}
