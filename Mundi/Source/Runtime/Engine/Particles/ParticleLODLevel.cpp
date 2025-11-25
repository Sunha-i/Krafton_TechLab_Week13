#include "pch.h"
#include "ParticleLODLevel.h"
#include "ObjectFactory.h"
#include "JsonSerializer.h"
#include "Modules/ParticleModuleRequired.h"
#include "Modules/ParticleModuleTypeDataBase.h"

UParticleLODLevel::~UParticleLODLevel()
{
	// RequiredModule 삭제 (Modules 배열에 포함되지 않음)
	if (RequiredModule)
	{
		DeleteObject(RequiredModule);
		RequiredModule = nullptr;
	}

	// TypeDataModule 삭제 (Modules 배열에 포함되지 않음)
	if (TypeDataModule)
	{
		DeleteObject(TypeDataModule);
		TypeDataModule = nullptr;
	}

	// Modules 배열의 모든 모듈 삭제 (SpawnModule 포함)
	for (UParticleModule* Module : Modules)
	{
		if (Module)
		{
			DeleteObject(Module);
		}
	}
	Modules.Empty();

	// SpawnModule은 Modules에 포함되어 있으므로 포인터만 정리
	SpawnModule = nullptr;

	// 캐시 배열 정리 (소유권 없음, 포인터만 정리)
	SpawnModules.Empty();
	UpdateModules.Empty();
}

void UParticleLODLevel::CacheModuleInfo()
{
	// RequiredModule 보장
	if (RequiredModule == nullptr)
	{
		RequiredModule = ObjectFactory::NewObject<UParticleModuleRequired>();
	}

	SpawnModules.clear();
	UpdateModules.clear();

	// 언리얼 엔진 방식: 모듈 타입 플래그에 따라 분류
	for (UParticleModule* Module : Modules)
	{
		if (Module && Module->bEnabled)
		{
			// 스폰 모듈인 경우 SpawnModules에 추가
			if (Module->bSpawnModule)
			{
				SpawnModules.Add(Module);
			}

			// 업데이트 모듈인 경우 UpdateModules에 추가
			if (Module->bUpdateModule)
			{
				UpdateModules.Add(Module);
			}
		}
	}
}

void UParticleLODLevel::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	// 기본 타입 UPROPERTY 자동 직렬화 (Level, bEnabled)
	UObject::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		// === RequiredModule 로드 ===
		JSON RequiredJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "RequiredModule", RequiredJson))
		{
			RequiredModule = NewObject<UParticleModuleRequired>();
			RequiredModule->Serialize(true, RequiredJson);
		}

		// NOTE: SpawnModule은 Modules 배열에서 로드 후 찾아서 할당 (아래 참조)

		// === TypeDataModule 로드 (다형성 처리) ===
		JSON TypeDataJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "TypeDataModule", TypeDataJson))
		{
			FString TypeString;
			FJsonSerializer::ReadString(TypeDataJson, "Type", TypeString);
			UClass* TypeDataClass = UClass::FindClass(TypeString);
			if (TypeDataClass)
			{
				TypeDataModule = Cast<UParticleModuleTypeDataBase>(
					ObjectFactory::NewObject(TypeDataClass));
				if (TypeDataModule)
				{
					TypeDataModule->Serialize(true, TypeDataJson);
				}
			}
		}

		// === Modules 배열 로드 (다형성 처리) ===
		JSON ModulesJson;
		if (FJsonSerializer::ReadArray(InOutHandle, "Modules", ModulesJson))
		{
			Modules.Empty();
			for (size_t i = 0; i < ModulesJson.size(); ++i)
			{
				JSON& ModuleData = ModulesJson.at(i);
				FString TypeString;
				FJsonSerializer::ReadString(ModuleData, "Type", TypeString);

				UClass* ModuleClass = UClass::FindClass(TypeString);
				if (ModuleClass)
				{
					UParticleModule* Module = Cast<UParticleModule>(ObjectFactory::NewObject(ModuleClass));
					if (Module)
					{
						Module->Serialize(true, ModuleData);
						Modules.Add(Module);
					}
				}
			}
		}

		// Modules 배열에서 SpawnModule 찾아서 포인터 할당
		// SpawnModule은 Modules 배열에 소유권이 있으며, 편의를 위한 참조 포인터
		SpawnModule = nullptr;
		for (UParticleModule* Module : Modules)
		{
			if (UParticleModuleSpawn* Spawn = Cast<UParticleModuleSpawn>(Module))
			{
				SpawnModule = Spawn;
				break;
			}
		}

		// 로딩 후 캐싱
		CacheModuleInfo();
	}
	else
	{
		// === RequiredModule 저장 ===
		if (RequiredModule)
		{
			JSON RequiredJson = JSON::Make(JSON::Class::Object);
			RequiredModule->Serialize(false, RequiredJson);
			InOutHandle["RequiredModule"] = RequiredJson;
		}

		// NOTE: SpawnModule은 Modules 배열에 포함되어 있으므로 별도 저장하지 않음
		// 로드 시 Modules에서 UParticleModuleSpawn 타입을 찾아 SpawnModule 포인터에 할당

		// === TypeDataModule 저장 (타입 정보 포함) ===
		if (TypeDataModule)
		{
			JSON TypeDataJson = JSON::Make(JSON::Class::Object);
			TypeDataJson["Type"] = TypeDataModule->GetClass()->Name;
			TypeDataModule->Serialize(false, TypeDataJson);
			InOutHandle["TypeDataModule"] = TypeDataJson;
		}

		// === Modules 배열 저장 (타입 정보 포함) ===
		JSON ModulesJson = JSON::Make(JSON::Class::Array);
		for (UParticleModule* Module : Modules)
		{
			if (Module)
			{
				JSON ModuleData = JSON::Make(JSON::Class::Object);
				ModuleData["Type"] = Module->GetClass()->Name;
				Module->Serialize(false, ModuleData);
				ModulesJson.append(ModuleData);
			}
		}
		InOutHandle["Modules"] = ModulesJson;
	}
}

void UParticleLODLevel::DuplicateSubObjects()
{
	UObject::DuplicateSubObjects();

	// RequiredModule 복제
	if (RequiredModule)
	{
		UParticleModuleRequired* NewRequired = ObjectFactory::DuplicateObject<UParticleModuleRequired>(RequiredModule);
		RequiredModule = NewRequired;
	}

	// 모든 모듈 복제
	TArray<UParticleModule*> NewModules;
	for (UParticleModule* Module : Modules)
	{
		if (Module)
		{
			UParticleModule* NewModule = ObjectFactory::DuplicateObject<UParticleModule>(Module);
			NewModules.Add(NewModule);
		}
	}
	Modules = NewModules;

	// TypeDataModule 복제
	if (TypeDataModule)
	{
		UParticleModuleTypeDataBase* NewTypeData = ObjectFactory::DuplicateObject<UParticleModuleTypeDataBase>(TypeDataModule);
		TypeDataModule = NewTypeData;
	}

	// SpawnModule은 Modules 배열에서 찾아서 할당 (별도 복제 X, 이미 Modules에서 복제됨)
	SpawnModule = nullptr;
	for (UParticleModule* Module : Modules)
	{
		if (UParticleModuleSpawn* Spawn = Cast<UParticleModuleSpawn>(Module))
		{
			SpawnModule = Spawn;
			break;
		}
	}

	// 모듈 정보 재캐싱
	CacheModuleInfo();
}
