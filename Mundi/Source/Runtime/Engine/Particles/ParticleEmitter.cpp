#include "pch.h"
#include "ParticleEmitter.h"
#include "ObjectFactory.h"
#include "JsonSerializer.h"

UParticleEmitter::~UParticleEmitter()
{
	// 모든 LOD 레벨 삭제
	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		if (LODLevel)
		{
			DeleteObject(LODLevel);
		}
	}
	LODLevels.Empty();
}

void UParticleEmitter::CacheEmitterModuleInfo()
{
	// 파티클 크기 계산
	ParticleSize = sizeof(FBaseParticle);

	// 모듈별 추가 크기 계산을 여기에 추가 가능
	// 예를 들어, 모듈이 파티클에 페이로드 데이터를 추가할 수 있음

	// 모든 LOD 레벨의 모듈 정보 캐싱
	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		if (LODLevel)
		{
			LODLevel->CacheModuleInfo();
		}
	}
}

UParticleLODLevel* UParticleEmitter::GetLODLevel(int32 LODIndex) const
{
	if (LODIndex >= 0 && LODIndex < LODLevels.size())
	{
		return LODLevels[LODIndex];
	}
	return nullptr;
}

void UParticleEmitter::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	// 기본 타입 UPROPERTY 자동 직렬화 (EmitterName 등)
	UObject::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// === LODLevels 배열 로드 ===
		JSON LODLevelsJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "LODLevels", LODLevelsJson))
		{
			LODLevels.Empty();
			for (size_t i = 0; i < LODLevelsJson.size(); ++i)
			{
				JSON& LODData = LODLevelsJson.at(i);
				UParticleLODLevel* LOD = NewObject<UParticleLODLevel>();
				if (LOD)
				{
					LOD->Serialize(true, LODData);
					LODLevels.Add(LOD);
				}
			}
		}

		// 로딩 후 캐싱
		CacheEmitterModuleInfo();
	}
	else
	{
		// === LODLevels 배열 저장 ===
		JSON LODLevelsJson = JSON::Make(JSON::Class::Array);
		for (UParticleLODLevel* LOD : LODLevels)
		{
			if (LOD)
			{
				JSON LODData = JSON::Make(JSON::Class::Object);
				LOD->Serialize(false, LODData);
				LODLevelsJson.append(LODData);
			}
		}
		InOutHandle["LODLevels"] = LODLevelsJson;
	}
}

void UParticleEmitter::DuplicateSubObjects()
{
	UObject::DuplicateSubObjects();

	// 모든 LOD 레벨 복제
	TArray<UParticleLODLevel*> NewLODLevels;
	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		if (LODLevel)
		{
			UParticleLODLevel* NewLOD = ObjectFactory::DuplicateObject<UParticleLODLevel>(LODLevel);
			if (NewLOD)
			{
				NewLOD->DuplicateSubObjects(); // 재귀적으로 서브 오브젝트 복제
				NewLODLevels.Add(NewLOD);
			}
		}
	}
	LODLevels = NewLODLevels;

	// 이미터 정보 재캐싱
	CacheEmitterModuleInfo();
}
