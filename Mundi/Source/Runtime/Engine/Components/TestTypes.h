#pragma once

#include "Vector.h"
#include "FTestStruct.generated.h"

// ===== Test Enum =====
UENUM()
enum class ETestMode : uint8
{
	None = 0,
	ModeA = 1,
	ModeB = 2,
	ModeC = 3
};

// ===== Test Struct =====
USTRUCT(DisplayName="Test Struct", Description="Simple test struct")
struct FTestStruct
{
	GENERATED_REFLECTION_BODY()

	UPROPERTY(EditAnywhere, Category = "Struct")
	float Value;

	UPROPERTY(EditAnywhere, Category = "Struct")
	FString Name;

	UPROPERTY(EditAnywhere, Category = "Struct")
	FVector Position;

	FTestStruct() : Value(0.0f), Name("Default"), Position(FVector(0.0f, 0.0f, 0.0f)) {}
};
