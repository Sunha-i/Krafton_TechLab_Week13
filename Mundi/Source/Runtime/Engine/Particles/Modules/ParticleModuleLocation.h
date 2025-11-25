#pragma once

#include "ParticleModule.h"
#include "Distribution.h"
#include "UParticleModuleLocation.generated.h"

// 언리얼 엔진 호환: 스폰 분포 형태
enum class ELocationDistributionShape : uint8
{
	Box = 0,      // 박스 형태
	Sphere = 1,   // 구 형태
	Cylinder = 2  // 실린더 형태
};

// 언리얼 엔진 호환: 파티클별 위치 페이로드 (16바이트, 16바이트 정렬)
struct FParticleLocationPayload
{
	FVector SpawnLocation;  // 실제 스폰 위치 (12바이트)
	float Padding0;         // 정렬 패딩 (4바이트)
	// 총 16바이트
};

class FParticleRandomStream;

UCLASS(DisplayName="위치 모듈", Description="파티클의 초기 위치를 결정하는 모듈입니다")
class UParticleModuleLocation : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	// 시작 위치 중심점 - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="Location")
	FDistributionVector StartLocation = FDistributionVector(FVector(0.0f, 0.0f, 0.0f));

	// 분포 형태 (0=Box, 1=Sphere, 2=Cylinder)
	UPROPERTY(EditAnywhere, Category="Location")
	int32 DistributionShape = 0;

	// Box 분포 시 각 축별 범위 - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="Location")
	FDistributionVector BoxExtent = FDistributionVector(FVector(0.0f, 0.0f, 0.0f));

	// 구/실린더 분포 시 반지름 - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="Location")
	FDistributionFloat SphereRadius = FDistributionFloat(100.0f);

	// 실린더 분포 시 높이 - Distribution 시스템
	UPROPERTY(EditAnywhere, Category="Location")
	FDistributionFloat CylinderHeight = FDistributionFloat(100.0f);

	// 표면에만 스폰할지 여부 (Sphere/Cylinder 전용)
	UPROPERTY(EditAnywhere, Category="Location")
	bool bSurfaceOnly = false;

	UParticleModuleLocation()
	{
		bSpawnModule = true;  // 스폰 시 위치 설정
	}
	virtual ~UParticleModuleLocation() = default;

	// 언리얼 엔진 호환: 파티클별 페이로드 크기 반환
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = nullptr) override
	{
		return sizeof(FParticleLocationPayload);
	}

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

private:
	// 분포 형태별 랜덤 위치 생성 (RandomStream 사용)
	FVector GenerateRandomLocationBox(FParticleRandomStream& RandomStream, const FVector& Extent);
	FVector GenerateRandomLocationSphere(FParticleRandomStream& RandomStream, float Radius);
	FVector GenerateRandomLocationCylinder(FParticleRandomStream& RandomStream, float Radius, float Height);
};
