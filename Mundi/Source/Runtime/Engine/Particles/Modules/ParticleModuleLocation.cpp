#include "pch.h"
#include "ParticleModuleLocation.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"

void UParticleModuleLocation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner || !Owner->Component)
	{
		return;
	}
	const FTransform& ComponentTransform = Owner->Component->GetWorldTransform();

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleLocationPayload, Payload);

	// Distribution 시스템을 사용하여 시작 위치 계산
	FVector BaseLocation = StartLocation.GetValue(0.0f, Owner->RandomStream, Owner->Component);

	// 분포 형태에 따라 랜덤 위치 생성
	FVector LocalRandomOffset;
	switch (static_cast<ELocationDistributionShape>(DistributionShape))
	{
	case ELocationDistributionShape::Sphere:
		{
			float Radius = SphereRadius.GetValue(0.0f, Owner->RandomStream, Owner->Component);
			LocalRandomOffset = GenerateRandomLocationSphere(Owner->RandomStream, Radius);
		}
		break;
	case ELocationDistributionShape::Cylinder:
		{
			float Radius = SphereRadius.GetValue(0.0f, Owner->RandomStream, Owner->Component);
			float Height = CylinderHeight.GetValue(0.0f, Owner->RandomStream, Owner->Component);
			LocalRandomOffset = GenerateRandomLocationCylinder(Owner->RandomStream, Radius, Height);
		}
		break;
	case ELocationDistributionShape::Box:
	default:
		{
			FVector Extent = BoxExtent.GetValue(0.0f, Owner->RandomStream, Owner->Component);
			LocalRandomOffset = GenerateRandomLocationBox(Owner->RandomStream, Extent);
		}
		break;
	}

	// 총 로컬 오프셋을 컴포넌트의 월드 변환을 적용해 최종 월드 위치로 변환
	FVector TotalLocalOffset = BaseLocation + LocalRandomOffset;
	FVector FinalWorldPosition = ComponentTransform.TransformPosition(TotalLocalOffset);

	// 최종 스폰 위치 계산
	Payload.SpawnLocation = FinalWorldPosition;
	ParticleBase->Location = Payload.SpawnLocation;
	ParticleBase->OldLocation = Payload.SpawnLocation;
}

FVector UParticleModuleLocation::GenerateRandomLocationBox(FParticleRandomStream& RandomStream, const FVector& Extent)
{
	// 박스 형태: 각 축별로 -Extent ~ +Extent 범위에서 균등 분포
	return FVector(
		RandomStream.GetSignedFraction() * Extent.X,
		RandomStream.GetSignedFraction() * Extent.Y,
		RandomStream.GetSignedFraction() * Extent.Z
	);
}

FVector UParticleModuleLocation::GenerateRandomLocationSphere(FParticleRandomStream& RandomStream, float InRadius)
{
	// 언리얼 엔진 호환: 구 형태 분포
	// 균등한 구 분포를 위해 구좌표계 사용
	float Theta = RandomStream.GetFraction() * 2.0f * 3.14159265f;  // 0 ~ 2π
	float Phi = acosf(2.0f * RandomStream.GetFraction() - 1.0f);    // 0 ~ π (균등 분포)

	float FinalRadius;
	if (bSurfaceOnly)
	{
		// 표면에만 스폰
		FinalRadius = InRadius;
	}
	else
	{
		// 부피 전체에 균등 분포 (r^3 보정)
		float RandomValue = RandomStream.GetFraction();
		FinalRadius = InRadius * powf(RandomValue, 1.0f / 3.0f);
	}

	// 구좌표 -> 직교좌표 변환
	float SinPhi = sinf(Phi);
	return FVector(
		FinalRadius * SinPhi * cosf(Theta),
		FinalRadius * SinPhi * sinf(Theta),
		FinalRadius * cosf(Phi)
	);
}

FVector UParticleModuleLocation::GenerateRandomLocationCylinder(FParticleRandomStream& RandomStream, float InRadius, float InHeight)
{
	// 언리얼 엔진 호환: 실린더 형태 분포 (Z축 기준)
	float Theta = RandomStream.GetFraction() * 2.0f * 3.14159265f;  // 0 ~ 2π

	float FinalRadius;
	if (bSurfaceOnly)
	{
		// 원통 표면에만 스폰
		FinalRadius = InRadius;
	}
	else
	{
		// 원통 부피 전체에 균등 분포 (r^2 보정)
		float RandomValue = RandomStream.GetFraction();
		FinalRadius = InRadius * sqrtf(RandomValue);
	}

	// Z 높이는 -InHeight/2 ~ +InHeight/2 범위
	float Height = (RandomStream.GetFraction() - 0.5f) * InHeight;

	return FVector(
		FinalRadius * cosf(Theta),
		FinalRadius * sinf(Theta),
		Height
	);
}

void UParticleModuleLocation::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		if (FJsonSerializer::ReadObject(InOutHandle, "StartLocation", TempJson))
			StartLocation.Serialize(true, TempJson);
		FJsonSerializer::ReadInt32(InOutHandle, "DistributionShape", DistributionShape);
		if (FJsonSerializer::ReadObject(InOutHandle, "BoxExtent", TempJson))
			BoxExtent.Serialize(true, TempJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "SphereRadius", TempJson))
			SphereRadius.Serialize(true, TempJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "CylinderHeight", TempJson))
			CylinderHeight.Serialize(true, TempJson);
		FJsonSerializer::ReadBool(InOutHandle, "bSurfaceOnly", bSurfaceOnly);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		StartLocation.Serialize(false, TempJson);
		InOutHandle["StartLocation"] = TempJson;

		InOutHandle["DistributionShape"] = DistributionShape;

		TempJson = JSON::Make(JSON::Class::Object);
		BoxExtent.Serialize(false, TempJson);
		InOutHandle["BoxExtent"] = TempJson;

		TempJson = JSON::Make(JSON::Class::Object);
		SphereRadius.Serialize(false, TempJson);
		InOutHandle["SphereRadius"] = TempJson;

		TempJson = JSON::Make(JSON::Class::Object);
		CylinderHeight.Serialize(false, TempJson);
		InOutHandle["CylinderHeight"] = TempJson;

		InOutHandle["bSurfaceOnly"] = bSurfaceOnly;
	}
}
