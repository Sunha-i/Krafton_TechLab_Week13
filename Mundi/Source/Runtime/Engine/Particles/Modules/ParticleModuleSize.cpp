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

	// BEGIN_UPDATE_LOOP/END_UPDATE_LOOP 매크로 사용
	// 언리얼 엔진 표준 패턴: 역방향 순회, Freeze 자동 스킵
	BEGIN_UPDATE_LOOP
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleSizePayload, SizePayload);

		// 크기 보간 (OverLife 패턴)
		float Alpha = Particle.RelativeTime * SizePayload.SizeMultiplierOverLife;
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

		// 페이로드에 저장된 초기/목표 크기를 사용하여 보간
		Particle.Size.X = FMath::Lerp(SizePayload.InitialSize.X, SizePayload.EndSize.X, Alpha);
		Particle.Size.Y = FMath::Lerp(SizePayload.InitialSize.Y, SizePayload.EndSize.Y, Alpha);
		Particle.Size.Z = FMath::Lerp(SizePayload.InitialSize.Z, SizePayload.EndSize.Z, Alpha);
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
