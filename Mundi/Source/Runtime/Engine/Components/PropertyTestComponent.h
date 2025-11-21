#pragma once

#include "ActorComponent.h"
#include "Texture.h"
#include "StaticMesh.h"
#include "SkeletalMesh.h"
#include "Material.h"
#include "Sound.h"
#include "TestTypes.h"
#include "UPropertyTestComponent.generated.h"

/**
 * Test component for verifying property system functionality
 * Includes all supported property types for widget rendering and serialization
 */
UCLASS(DisplayName="Property Test", Description="Property system test component")
class UPropertyTestComponent : public UActorComponent
{
public:
	GENERATED_REFLECTION_BODY()

	UPropertyTestComponent();
	virtual ~UPropertyTestComponent() = default;

	// ===== Basic Types =====
	UPROPERTY(EditAnywhere, Category = "Basic Types")
	bool bTestBool;

	UPROPERTY(EditAnywhere, Category = "Basic Types", Range = "0, 100")
	int32 TestInt;

	UPROPERTY(EditAnywhere, Category = "Basic Types", Range = "0.0, 1.0")
	float TestFloat;

	UPROPERTY(EditAnywhere, Category = "Basic Types")
	FString TestString;

	UPROPERTY(EditAnywhere, Category = "Basic Types")
	FName TestName;

	// ===== Vector Types =====
	UPROPERTY(EditAnywhere, Category = "Vector Types")
	FVector TestVector;

	UPROPERTY(EditAnywhere, Category = "Vector Types")
	FLinearColor TestColor;

	// ===== Resource Types =====
	UPROPERTY(EditAnywhere, Category = "Resources")
	UTexture* TestTexture;

	UPROPERTY(EditAnywhere, Category = "Resources")
	UStaticMesh* TestStaticMesh;

	UPROPERTY(EditAnywhere, Category = "Resources")
	USkeletalMesh* TestSkeletalMesh;

	UPROPERTY(EditAnywhere, Category = "Resources")
	UMaterial* TestMaterial;

	UPROPERTY(EditAnywhere, Category = "Resources")
	USound* TestSound;

	// ===== Array Types =====
	UPROPERTY(EditAnywhere, Category = "Arrays")
	TArray<int32> TestIntArray;

	UPROPERTY(EditAnywhere, Category = "Arrays")
	TArray<float> TestFloatArray;

	UPROPERTY(EditAnywhere, Category = "Arrays")
	TArray<FString> TestStringArray;

	UPROPERTY(EditAnywhere, Category = "Arrays")
	TArray<USound*> TestSoundArray;

	// ===== Map Types =====
	UPROPERTY(EditAnywhere, Category = "Maps")
	TMap<FString, int32> TestStringIntMap;

	UPROPERTY(EditAnywhere, Category = "Maps")
	TMap<FString, float> TestStringFloatMap;

	UPROPERTY(EditAnywhere, Category = "Maps")
	TMap<FString, FString> TestStringStringMap;

	// ===== Script =====
	UPROPERTY(EditAnywhere, Category = "Script", FileExtension = "lua")
	FString TestScriptFile;

	// ===== Enum Types =====
	UPROPERTY(EditAnywhere, Category = "Enum")
	ETestMode TestEnum;

	// ===== Struct Types =====
	UPROPERTY(EditAnywhere, Category = "Struct")
	FTestStruct TestStruct;
};
