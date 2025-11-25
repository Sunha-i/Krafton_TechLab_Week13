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

	// Distribution 시스템을 사용하여 회전 속도 계산
	FVector InitialRotRate = StartRotationRate.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	Payload.RotationRate = InitialRotRate;

	// 기본 Z축 회전도 동기화 (스프라이트 호환)
	ParticleBase->Rotation = Payload.Rotation.Z;
	ParticleBase->RotationRate = Payload.RotationRate.Z;
}

void UParticleModuleMeshRotation::Update(FModuleUpdateContext& Context)
{
	BEGIN_UPDATE_LOOP;
		// 페이로드 가져오기
		PARTICLE_ELEMENT(FMeshRotationPayload, Payload);

		// 회전 속도 적용
		Payload.Rotation.X += Payload.RotationRate.X * DeltaTime;
		Payload.Rotation.Y += Payload.RotationRate.Y * DeltaTime;
		Payload.Rotation.Z += Payload.RotationRate.Z * DeltaTime;

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
		if (FJsonSerializer::ReadObject(InOutHandle, "StartRotationRate", TempJson))
			StartRotationRate.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		StartRotation.Serialize(false, TempJson);
		InOutHandle["StartRotation"] = TempJson;

		TempJson = JSON::Make(JSON::Class::Object);
		StartRotationRate.Serialize(false, TempJson);
		InOutHandle["StartRotationRate"] = TempJson;
	}
}
