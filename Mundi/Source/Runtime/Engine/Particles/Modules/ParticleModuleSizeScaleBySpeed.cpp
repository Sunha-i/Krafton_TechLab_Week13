#include "pch.h"
#include "ParticleModuleSizeScaleBySpeed.h"
#include "ParticleEmitterInstance.h"

void UParticleModuleSizeScaleBySpeed::Update(FModuleUpdateContext& Context)
{
	// SpeedScale을 3D 벡터로 변환 (Z는 1.0 유지)
	FVector Scale(SpeedScale.X, SpeedScale.Y, 1.0f);
	FVector ScaleMax(MaxScale.X, MaxScale.Y, 1.0f);

	BEGIN_UPDATE_LOOP
		// 속도 크기 계산
		float Speed = Particle.Velocity.Size();

		// 속도에 비례한 크기 계산
		FVector Size = Scale * Speed;

		// 최소값 적용 (최소 1.0)
		Size.X = (Size.X < 1.0f) ? 1.0f : Size.X;
		Size.Y = (Size.Y < 1.0f) ? 1.0f : Size.Y;
		Size.Z = (Size.Z < 1.0f) ? 1.0f : Size.Z;

		// 최대값 적용
		Size.X = (Size.X > ScaleMax.X) ? ScaleMax.X : Size.X;
		Size.Y = (Size.Y > ScaleMax.Y) ? ScaleMax.Y : Size.Y;
		Size.Z = (Size.Z > ScaleMax.Z) ? ScaleMax.Z : Size.Z;

		// 기본 크기에 스케일 적용
		Particle.Size = GetParticleBaseSize(Particle) * Size;
	END_UPDATE_LOOP
}

void UParticleModuleSizeScaleBySpeed::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "SpeedScaleX", SpeedScale.X);
		FJsonSerializer::ReadFloat(InOutHandle, "SpeedScaleY", SpeedScale.Y);
		FJsonSerializer::ReadFloat(InOutHandle, "MaxScaleX", MaxScale.X);
		FJsonSerializer::ReadFloat(InOutHandle, "MaxScaleY", MaxScale.Y);
	}
	else
	{
		InOutHandle["SpeedScaleX"] = SpeedScale.X;
		InOutHandle["SpeedScaleY"] = SpeedScale.Y;
		InOutHandle["MaxScaleX"] = MaxScale.X;
		InOutHandle["MaxScaleY"] = MaxScale.Y;
	}
}
