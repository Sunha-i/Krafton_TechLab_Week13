#include "pch.h"
#include "ParticleModuleLifetime.h"
#include "ParticleEmitterInstance.h"

void UParticleModuleLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner)
	{
		return;
	}

	// Distribution 시스템을 사용하여 수명 계산
	// Time=0.0f (Spawn 시점), RandomStream 사용
	float LifetimeValue = Lifetime.GetValue(0.0f, Owner->RandomStream, Owner->Component);

	// 언리얼 엔진 방식: Lifetime 대신 OneOverMaxLifetime 사용 (나눗셈 최적화)
	ParticleBase->OneOverMaxLifetime = (LifetimeValue > 0.0f) ? (1.0f / LifetimeValue) : 0.0f;
	ParticleBase->RelativeTime = 0.0f;

	//UE_LOG("[Lifetime] Spawn: Lifetime=%.2f, OneOverMaxLifetime=%.4f", Lifetime, ParticleBase->OneOverMaxLifetime);
}

void UParticleModuleLifetime::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON LifetimeJson;
	if (bInIsLoading)
	{
		if (FJsonSerializer::ReadObject(InOutHandle, "Lifetime", LifetimeJson))
			Lifetime.Serialize(true, LifetimeJson);
	}
	else
	{
		LifetimeJson = JSON::Make(JSON::Class::Object);
		Lifetime.Serialize(false, LifetimeJson);
		InOutHandle["Lifetime"] = LifetimeJson;
	}
}
