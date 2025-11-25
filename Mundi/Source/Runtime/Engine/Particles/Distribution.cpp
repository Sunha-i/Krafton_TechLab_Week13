#include "pch.h"
#include "Distribution.h"
#include "Source/Runtime/Engine/Components/ParticleSystemComponent.h"

// ============================================================
// FDistributionFloat::GetValue() 구현
// ============================================================
float FDistributionFloat::GetValue(
	float Time,
	FParticleRandomStream& RandomStream,
	UParticleSystemComponent* Owner
) const
{
	switch (Type)
	{
	case EDistributionType::Constant:
		// 고정값 반환
		return ConstantValue;

	case EDistributionType::Uniform:
		// Min~Max 범위에서 랜덤 값 (결정론적 랜덤)
		return RandomStream.GetRangeFloat(MinValue, MaxValue);

	case EDistributionType::ConstantCurve:
		// 시간에 따른 커브 값 (모든 파티클 동일)
		return ConstantCurve.Eval(Time);

	case EDistributionType::UniformCurve:
	{
		// 시간에 따른 Min/Max 커브 계산 후 그 범위 내 랜덤
		float MinAtTime = MinCurve.Eval(Time);
		float MaxAtTime = MaxCurve.Eval(Time);
		return RandomStream.GetRangeFloat(MinAtTime, MaxAtTime);
	}

	case EDistributionType::ParticleParameter:
		// 런타임 파라미터에서 값 가져오기
		if (Owner && !ParameterName.empty())
		{
			return Owner->GetFloatParameter(ParameterName, ParameterDefaultValue);
		}
		return ParameterDefaultValue;

	default:
		return 0.0f;
	}
}

// ============================================================
// FDistributionVector::GetValue() 구현
// ============================================================
FVector FDistributionVector::GetValue(
	float Time,
	FParticleRandomStream& RandomStream,
	UParticleSystemComponent* Owner
) const
{
	switch (Type)
	{
	case EDistributionType::Constant:
		// 고정값 반환
		return ConstantValue;

	case EDistributionType::Uniform:
		// 각 축별로 Min~Max 범위에서 랜덤 값
		return RandomStream.GetRangeVector(MinValue, MaxValue);

	case EDistributionType::ConstantCurve:
		// 시간에 따른 커브 값
		return ConstantCurve.Eval(Time);

	case EDistributionType::UniformCurve:
	{
		// 시간에 따른 Min/Max 커브 계산 후 그 범위 내 랜덤
		FVector MinAtTime = MinCurve.Eval(Time);
		FVector MaxAtTime = MaxCurve.Eval(Time);
		return RandomStream.GetRangeVector(MinAtTime, MaxAtTime);
	}

	case EDistributionType::ParticleParameter:
		// 런타임 파라미터에서 값 가져오기
		if (Owner && !ParameterName.empty())
		{
			return Owner->GetVectorParameter(ParameterName, ParameterDefaultValue);
		}
		return ParameterDefaultValue;

	default:
		return FVector(0.0f, 0.0f, 0.0f);
	}
}

// ============================================================
// FInterpCurvePointFloat::Serialize() 구현
// ============================================================
void FInterpCurvePointFloat::Serialize(bool bIsLoading, JSON& InOutHandle)
{
	if (bIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "InVal", InVal);
		FJsonSerializer::ReadFloat(InOutHandle, "OutVal", OutVal);
		FJsonSerializer::ReadFloat(InOutHandle, "ArriveTangent", ArriveTangent);
		FJsonSerializer::ReadFloat(InOutHandle, "LeaveTangent", LeaveTangent);
		int32 Mode = 0;
		FJsonSerializer::ReadInt32(InOutHandle, "InterpMode", Mode);
		InterpMode = static_cast<EInterpCurveMode>(Mode);
	}
	else
	{
		InOutHandle["InVal"] = InVal;
		InOutHandle["OutVal"] = OutVal;
		InOutHandle["ArriveTangent"] = ArriveTangent;
		InOutHandle["LeaveTangent"] = LeaveTangent;
		InOutHandle["InterpMode"] = static_cast<int32>(InterpMode);
	}
}

// ============================================================
// FInterpCurvePointVector::Serialize() 구현
// ============================================================
void FInterpCurvePointVector::Serialize(bool bIsLoading, JSON& InOutHandle)
{
	if (bIsLoading)
	{
		FJsonSerializer::ReadFloat(InOutHandle, "InVal", InVal);
		FJsonSerializer::ReadVector(InOutHandle, "OutVal", OutVal);
		FJsonSerializer::ReadVector(InOutHandle, "ArriveTangent", ArriveTangent);
		FJsonSerializer::ReadVector(InOutHandle, "LeaveTangent", LeaveTangent);
		int32 Mode = 0;
		FJsonSerializer::ReadInt32(InOutHandle, "InterpMode", Mode);
		InterpMode = static_cast<EInterpCurveMode>(Mode);
	}
	else
	{
		InOutHandle["InVal"] = InVal;
		InOutHandle["OutVal"] = FJsonSerializer::VectorToJson(OutVal);
		InOutHandle["ArriveTangent"] = FJsonSerializer::VectorToJson(ArriveTangent);
		InOutHandle["LeaveTangent"] = FJsonSerializer::VectorToJson(LeaveTangent);
		InOutHandle["InterpMode"] = static_cast<int32>(InterpMode);
	}
}

// ============================================================
// FInterpCurveFloat::Serialize() 구현
// ============================================================
void FInterpCurveFloat::Serialize(bool bIsLoading, JSON& InOutHandle)
{
	if (bIsLoading)
	{
		FJsonSerializer::ReadBool(InOutHandle, "bIsLooped", bIsLooped);
		FJsonSerializer::ReadFloat(InOutHandle, "LoopKeyOffset", LoopKeyOffset);

		JSON PointsArray;
		if (FJsonSerializer::ReadArray(InOutHandle, "Points", PointsArray))
		{
			Points.Empty();
			for (int32 i = 0; i < static_cast<int32>(PointsArray.size()); ++i)
			{
				FInterpCurvePointFloat Point;
				JSON PointJson = PointsArray.at(i);
				Point.Serialize(true, PointJson);
				Points.Add(Point);
			}
		}
	}
	else
	{
		InOutHandle["bIsLooped"] = bIsLooped;
		InOutHandle["LoopKeyOffset"] = LoopKeyOffset;

		JSON PointsArray = JSON::Make(JSON::Class::Array);
		for (int32 i = 0; i < Points.Num(); ++i)
		{
			JSON PointJson = JSON::Make(JSON::Class::Object);
			Points[i].Serialize(false, PointJson);
			PointsArray.append(PointJson);
		}
		InOutHandle["Points"] = PointsArray;
	}
}

// ============================================================
// FInterpCurveVector::Serialize() 구현
// ============================================================
void FInterpCurveVector::Serialize(bool bIsLoading, JSON& InOutHandle)
{
	if (bIsLoading)
	{
		FJsonSerializer::ReadBool(InOutHandle, "bIsLooped", bIsLooped);
		FJsonSerializer::ReadFloat(InOutHandle, "LoopKeyOffset", LoopKeyOffset);

		JSON PointsArray;
		if (FJsonSerializer::ReadArray(InOutHandle, "Points", PointsArray))
		{
			Points.Empty();
			for (int32 i = 0; i < static_cast<int32>(PointsArray.size()); ++i)
			{
				FInterpCurvePointVector Point;
				JSON PointJson = PointsArray.at(i);
				Point.Serialize(true, PointJson);
				Points.Add(Point);
			}
		}
	}
	else
	{
		InOutHandle["bIsLooped"] = bIsLooped;
		InOutHandle["LoopKeyOffset"] = LoopKeyOffset;

		JSON PointsArray = JSON::Make(JSON::Class::Array);
		for (int32 i = 0; i < Points.Num(); ++i)
		{
			JSON PointJson = JSON::Make(JSON::Class::Object);
			Points[i].Serialize(false, PointJson);
			PointsArray.append(PointJson);
		}
		InOutHandle["Points"] = PointsArray;
	}
}

// ============================================================
// FDistributionFloat::Serialize() 구현
// ============================================================
void FDistributionFloat::Serialize(bool bIsLoading, JSON& InOutHandle)
{
	if (bIsLoading)
	{
		int32 TypeInt = 0;
		FJsonSerializer::ReadInt32(InOutHandle, "Type", TypeInt);
		Type = static_cast<EDistributionType>(TypeInt);

		FJsonSerializer::ReadFloat(InOutHandle, "ConstantValue", ConstantValue);
		FJsonSerializer::ReadFloat(InOutHandle, "MinValue", MinValue);
		FJsonSerializer::ReadFloat(InOutHandle, "MaxValue", MaxValue);
		FJsonSerializer::ReadString(InOutHandle, "ParameterName", ParameterName);
		FJsonSerializer::ReadFloat(InOutHandle, "ParameterDefaultValue", ParameterDefaultValue);

		JSON CurveJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "ConstantCurve", CurveJson))
			ConstantCurve.Serialize(true, CurveJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "MinCurve", CurveJson))
			MinCurve.Serialize(true, CurveJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "MaxCurve", CurveJson))
			MaxCurve.Serialize(true, CurveJson);
	}
	else
	{
		InOutHandle["Type"] = static_cast<int32>(Type);
		InOutHandle["ConstantValue"] = ConstantValue;
		InOutHandle["MinValue"] = MinValue;
		InOutHandle["MaxValue"] = MaxValue;
		InOutHandle["ParameterName"] = ParameterName;
		InOutHandle["ParameterDefaultValue"] = ParameterDefaultValue;

		JSON CurveJson = JSON::Make(JSON::Class::Object);
		ConstantCurve.Serialize(false, CurveJson);
		InOutHandle["ConstantCurve"] = CurveJson;

		CurveJson = JSON::Make(JSON::Class::Object);
		MinCurve.Serialize(false, CurveJson);
		InOutHandle["MinCurve"] = CurveJson;

		CurveJson = JSON::Make(JSON::Class::Object);
		MaxCurve.Serialize(false, CurveJson);
		InOutHandle["MaxCurve"] = CurveJson;
	}
}

// ============================================================
// FDistributionVector::Serialize() 구현
// ============================================================
void FDistributionVector::Serialize(bool bIsLoading, JSON& InOutHandle)
{
	if (bIsLoading)
	{
		int32 TypeInt = 0;
		FJsonSerializer::ReadInt32(InOutHandle, "Type", TypeInt);
		Type = static_cast<EDistributionType>(TypeInt);

		FJsonSerializer::ReadVector(InOutHandle, "ConstantValue", ConstantValue);
		FJsonSerializer::ReadVector(InOutHandle, "MinValue", MinValue);
		FJsonSerializer::ReadVector(InOutHandle, "MaxValue", MaxValue);
		FJsonSerializer::ReadString(InOutHandle, "ParameterName", ParameterName);
		FJsonSerializer::ReadVector(InOutHandle, "ParameterDefaultValue", ParameterDefaultValue);

		JSON CurveJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "ConstantCurve", CurveJson))
			ConstantCurve.Serialize(true, CurveJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "MinCurve", CurveJson))
			MinCurve.Serialize(true, CurveJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "MaxCurve", CurveJson))
			MaxCurve.Serialize(true, CurveJson);
	}
	else
	{
		InOutHandle["Type"] = static_cast<int32>(Type);
		InOutHandle["ConstantValue"] = FJsonSerializer::VectorToJson(ConstantValue);
		InOutHandle["MinValue"] = FJsonSerializer::VectorToJson(MinValue);
		InOutHandle["MaxValue"] = FJsonSerializer::VectorToJson(MaxValue);
		InOutHandle["ParameterName"] = ParameterName;
		InOutHandle["ParameterDefaultValue"] = FJsonSerializer::VectorToJson(ParameterDefaultValue);

		JSON CurveJson = JSON::Make(JSON::Class::Object);
		ConstantCurve.Serialize(false, CurveJson);
		InOutHandle["ConstantCurve"] = CurveJson;

		CurveJson = JSON::Make(JSON::Class::Object);
		MinCurve.Serialize(false, CurveJson);
		InOutHandle["MinCurve"] = CurveJson;

		CurveJson = JSON::Make(JSON::Class::Object);
		MaxCurve.Serialize(false, CurveJson);
		InOutHandle["MaxCurve"] = CurveJson;
	}
}

// ============================================================
// FDistributionColor::Serialize() 구현
// ============================================================
void FDistributionColor::Serialize(bool bIsLoading, JSON& InOutHandle)
{
	if (bIsLoading)
	{
		JSON RGBJson, AlphaJson;
		if (FJsonSerializer::ReadObject(InOutHandle, "RGB", RGBJson))
			RGB.Serialize(true, RGBJson);
		if (FJsonSerializer::ReadObject(InOutHandle, "Alpha", AlphaJson))
			Alpha.Serialize(true, AlphaJson);
	}
	else
	{
		JSON RGBJson = JSON::Make(JSON::Class::Object);
		RGB.Serialize(false, RGBJson);
		InOutHandle["RGB"] = RGBJson;

		JSON AlphaJson = JSON::Make(JSON::Class::Object);
		Alpha.Serialize(false, AlphaJson);
		InOutHandle["Alpha"] = AlphaJson;
	}
}
