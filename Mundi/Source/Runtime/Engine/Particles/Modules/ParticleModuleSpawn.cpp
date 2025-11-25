#include "pch.h"
#include "ParticleModuleSpawn.h"
#include "ParticleEmitterInstance.h"

// 언리얼 엔진 호환: 스폰 계산 로직 (책임 분리)
int32 UParticleModuleSpawn::CalculateSpawnCount(FParticleEmitterInstance* Owner, float DeltaTime, float& InOutSpawnFraction, bool& bInOutBurstFired)
{
	int32 TotalSpawnCount = 0;

	if (!Owner)
	{
		return TotalSpawnCount;
	}

	// 1. Burst 처리 (최초 1회만)
	// Distribution에서 값을 가져와서 int로 캐스팅
	int32 BurstCountValue = static_cast<int32>(BurstCount.GetValue(0.0f, Owner->RandomStream, Owner->Component));
	if (!bInOutBurstFired && BurstCountValue > 0)
	{
		bInOutBurstFired = true;
		TotalSpawnCount += BurstCountValue;
	}

	// 2. SpawnRate에 따른 정상 스폰 (부드러운 스폰)
	// Distribution에서 현재 SpawnRate 값 가져오기 (이미터 시간 기반 - 커브 타입 지원)
	float SpawnRateValue = SpawnRate.GetValue(Owner->EmitterTime, Owner->RandomStream, Owner->Component);
	if (SpawnRateValue > 0.0f)
	{
		// 언리얼 엔진 방식: 분수 누적으로 프레임 독립적 스폰
		float ParticlesToSpawn = SpawnRateValue * DeltaTime + InOutSpawnFraction;
		int32 Count = static_cast<int32>(ParticlesToSpawn);
		InOutSpawnFraction = ParticlesToSpawn - Count;  // 남은 분수 보존

		TotalSpawnCount += Count;
	}

	return TotalSpawnCount;
}

void UParticleModuleSpawn::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		if (FJsonSerializer::ReadObject(InOutHandle, "SpawnRate", TempJson))
			SpawnRate.Serialize(true, TempJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "BurstCount", TempJson))
			BurstCount.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		SpawnRate.Serialize(false, TempJson);
		InOutHandle["SpawnRate"] = TempJson;

		TempJson = JSON::Make(JSON::Class::Object);
		BurstCount.Serialize(false, TempJson);
		InOutHandle["BurstCount"] = TempJson;
	}
}
