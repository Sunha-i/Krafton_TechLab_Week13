#include "pch.h"
#include "PropertyTestComponent.h"

UPropertyTestComponent::UPropertyTestComponent()
	: bTestBool(true)
	, TestInt(42)
	, TestFloat(0.5f)
	, TestString("Hello World")
	, TestName(FName("TestName"))
	, TestVector(FVector(1.0f, 2.0f, 3.0f))
	, TestColor(FLinearColor(1.0f, 0.5f, 0.0f, 1.0f))
	, TestTexture(nullptr)
	, TestStaticMesh(nullptr)
	, TestSkeletalMesh(nullptr)
	, TestMaterial(nullptr)
	, TestSound(nullptr)
	, TestEnum(ETestMode::ModeA)
{
	// Initialize arrays with test data
	TestIntArray = { 1, 2, 3, 4, 5 };
	TestFloatArray = { 0.1f, 0.2f, 0.3f };
	TestStringArray = { "First", "Second", "Third" };

	// Initialize maps with test data
	TestStringIntMap["Health"] = 100;
	TestStringIntMap["Mana"] = 50;
	TestStringIntMap["Stamina"] = 75;

	TestStringFloatMap["Speed"] = 1.5f;
	TestStringFloatMap["Damage"] = 10.0f;

	TestStringStringMap["Name"] = "TestObject";
	TestStringStringMap["Type"] = "Component";

	// Initialize struct with test data
	TestStruct.Value = 42.0f;
	TestStruct.Name = "TestStruct";
	TestStruct.Position = FVector(10.0f, 20.0f, 30.0f);
}
