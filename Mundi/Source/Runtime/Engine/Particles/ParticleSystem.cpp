#include "pch.h"
#include "ParticleSystem.h"
#include "ObjectFactory.h"
#include "JsonSerializer.h"

UParticleSystem::~UParticleSystem()
{
	// 모든 이미터 삭제
	for (UParticleEmitter* Emitter : Emitters)
	{
		if (Emitter)
		{
			DeleteObject(Emitter);
		}
	}
	Emitters.Empty();
}

UParticleEmitter* UParticleSystem::GetEmitter(int32 EmitterIndex) const
{
	if (EmitterIndex >= 0 && EmitterIndex < Emitters.size())
	{
		return Emitters[EmitterIndex];
	}
	return nullptr;
}

void UParticleSystem::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	// 기본 타입 UPROPERTY 자동 직렬화
	UObject::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// === Emitters 배열 로드 ===
		JSON EmittersJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "Emitters", EmittersJson))
		{
			Emitters.Empty();
			for (size_t i = 0; i < EmittersJson.size(); ++i)
			{
				JSON& EmitterData = EmittersJson.at(i);
				UParticleEmitter* Emitter = NewObject<UParticleEmitter>();
				if (Emitter)
				{
					Emitter->Serialize(true, EmitterData);
					Emitters.Add(Emitter);
				}
			}
		}
	}
	else
	{
		// === Emitters 배열 저장 ===
		JSON EmittersJson = JSON::Make(JSON::Class::Array);
		for (UParticleEmitter* Emitter : Emitters)
		{
			if (Emitter)
			{
				JSON EmitterData = JSON::Make(JSON::Class::Object);
				Emitter->Serialize(false, EmitterData);
				EmittersJson.append(EmitterData);
			}
		}
		InOutHandle["Emitters"] = EmittersJson;
	}
}

void UParticleSystem::DuplicateSubObjects()
{
	UObject::DuplicateSubObjects();

	// 모든 이미터 복제
	TArray<UParticleEmitter*> NewEmitters;
	for (UParticleEmitter* Emitter : Emitters)
	{
		if (Emitter)
		{
			UParticleEmitter* NewEmitter = ObjectFactory::DuplicateObject<UParticleEmitter>(Emitter);
			if (NewEmitter)
			{
				NewEmitter->DuplicateSubObjects(); // 재귀적으로 서브 오브젝트 복제
				NewEmitters.Add(NewEmitter);
			}
		}
	}
	Emitters = NewEmitters;
}
