#pragma once

#include "ResourceBase.h"
#include "ParticleEmitter.h"
#include "UParticleSystem.generated.h"

UCLASS(DisplayName="파티클 시스템", Description="파티클 시스템은 여러 이미터를 포함하는 에셋입니다")
class UParticleSystem : public UResourceBase
{
public:
	GENERATED_REFLECTION_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Particle System")
	TArray<UParticleEmitter*> Emitters;

	UParticleSystem() = default;
	virtual ~UParticleSystem();

	// 인덱스로 이미터 가져오기
	UParticleEmitter* GetEmitter(int32 EmitterIndex) const;

	// 직렬화
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// 복제
	virtual void DuplicateSubObjects() override;
};
