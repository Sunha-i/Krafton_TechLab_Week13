#include "pch.h"
#include "ParticleModuleLifetime.h"
#include "ParticleEmitterInstance.h"

void UParticleModuleLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner)
	{
		return;
	}

	// 언리얼 엔진 호환: 랜덤 스트림 사용 (결정론적 재현 가능)
	float Lifetime = Owner->RandomStream.GetRangeFloat(MinLifetime, MaxLifetime);

	// 언리얼 엔진 방식: Lifetime 대신 OneOverMaxLifetime 사용 (나눗셈 최적화)
	ParticleBase->OneOverMaxLifetime = (Lifetime > 0.0f) ? (1.0f / Lifetime) : 0.0f;
	ParticleBase->RelativeTime = 0.0f;

	//UE_LOG("[Lifetime] Spawn: Lifetime=%.2f, OneOverMaxLifetime=%.4f", Lifetime, ParticleBase->OneOverMaxLifetime);
}

void UParticleModuleLifetime::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);
	// UPROPERTY 속성은 자동으로 직렬화됨
}
