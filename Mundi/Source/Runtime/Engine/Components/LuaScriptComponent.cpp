#include "pch.h"
#include "LuaScriptComponent.h"
#include "PrimitiveComponent.h"
#include "HitResult.h"
#include <sol/state.hpp>
#include <sol/coroutine.hpp>

#include "CameraActor.h"
#include "LuaManager.h"
#include "GameObject.h"

// for test
#include "PlayerCameraManager.h"
// IMPLEMENT_CLASS is now auto-generated in .generated.cpp
//BEGIN_PROPERTIES(ULuaScriptComponent)
//	MARK_AS_COMPONENT("Lua 스크립트 컴포넌트", "Lua 스크립트 컴포넌트입니다.")
//	ADD_PROPERTY_SCRIPT(FString, ScriptFilePath, "Script", ".lua", true, "Lua Script 파일 경로")
//END_PROPERTIES()

ULuaScriptComponent::ULuaScriptComponent()
{
	bCanEverTick = true;	// tick 지원 여부
}

ULuaScriptComponent::~ULuaScriptComponent()
{
	// 소멸자는 EndPlay가 호출되지 않았을 경우를 대비한
	// 최후의 안전 장치 역할을 해야 합니다.
	CleanupLuaResources();
}

void ULuaScriptComponent::BeginPlay()
{
	// 델리게이트 등록 - Owner의 첫 번째 PrimitiveComponent에 바인딩
	if (AActor* Owner = GetOwner())
	{
		// Owner의 PrimitiveComponent를 찾아서 델리게이트 등록
		if (UActorComponent* Comp = Owner->GetComponent(UPrimitiveComponent::StaticClass()))
		{
			BoundPrimitiveComponent = static_cast<UPrimitiveComponent*>(Comp);
			BeginHandleLua = BoundPrimitiveComponent->OnComponentBeginOverlap.AddDynamic(this, &ULuaScriptComponent::OnBeginOverlap);
			EndHandleLua = BoundPrimitiveComponent->OnComponentEndOverlap.AddDynamic(this, &ULuaScriptComponent::OnEndOverlap);
			HitHandleLua = BoundPrimitiveComponent->OnComponentHit.AddDynamic(this, &ULuaScriptComponent::OnHit);
		}
	}

	auto LuaVM = GetWorld()->GetLuaManager();
	Lua  = &(LuaVM->GetState());

	// 독립된 환경 생성, Engine Object&Util 주입
	Env = LuaVM->CreateEnvironment();

	FGameObject* Obj = Owner->GetGameObject();
	Env["Obj"] = Obj;

	Env["StartCoroutine"] = [LuaVM, this](sol::function f) {
		sol::state_view L = LuaVM->GetState();
		
		sol::thread Thread = sol::thread::create(L); // Coroutine 관리할 Thread 생성
		sol::state_view ThreadState = Thread.state();
		
		sol::coroutine Coroutine(ThreadState.lua_state(), f);                // 스레드에 함수 올리기
		return LuaVM->GetScheduler().Register(std::move(Thread), std::move(Coroutine), this);
	};
	
	if(ScriptFilePath.empty())
	{
		return;
	}

	if (!LuaVM->LoadScriptInto(Env, ScriptFilePath)) {
		UE_LOG("[Lua][error] failed to run: %s\n", ScriptFilePath.c_str());
#ifdef _EDITOR
		GEngine.EndPIE();
#endif
		return;
	}

	// InputManger 주입
	(*Lua)["InputManager"] = &UInputManager::GetInstance(); 
	// 함수 캐시
	FuncBeginPlay = FLuaManager::GetFunc(Env, "BeginPlay");
	FuncTick      = FLuaManager::GetFunc(Env, "Tick");
	FuncOnBeginOverlap = FLuaManager::GetFunc(Env, "OnBeginOverlap");
	FuncOnEndOverlap = FLuaManager::GetFunc(Env, "OnEndOverlap");
	FuncOnHit = FLuaManager::GetFunc(Env, "OnHit");
	FuncEndPlay		  =	FLuaManager::GetFunc(Env, "EndPlay");
	
	if (FuncBeginPlay.valid()) {
		auto Result = FuncBeginPlay();
		if (!Result.valid())
		{
			sol::error Err = Result; UE_LOG("[Lua][error] %s\n", Err.what());
#ifdef _EDITOR
			GEngine.EndPIE();
#endif
		}
	}

	bIsLuaCleanedUp = false;
}

void ULuaScriptComponent::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (FuncOnBeginOverlap.valid())
	{
		FGameObject* OtherGameObject = nullptr;
		if (OtherActor)
		{
			OtherGameObject = OtherActor->GetGameObject();
		}

		if (OtherGameObject)
		{
			auto Result = FuncOnBeginOverlap(OtherGameObject);
			if (!Result.valid())
			{
				sol::error Err = Result; UE_LOG("[Lua][error] %s\n", Err.what());
#ifdef _EDITOR
				GEngine.EndPIE();
#endif
			}
		}
	}
}

void ULuaScriptComponent::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (FuncOnEndOverlap.valid())
	{
		FGameObject* OtherGameObject = nullptr;
		if (OtherActor)
		{
			OtherGameObject = OtherActor->GetGameObject();
		}

		if (OtherGameObject)
		{
			auto Result = FuncOnEndOverlap(OtherGameObject);
			if (!Result.valid())
			{
				sol::error Err = Result; UE_LOG("[Lua][error] %s\n", Err.what());
#ifdef _EDITOR
				GEngine.EndPIE();
#endif
			}
		}
	}
}

void ULuaScriptComponent::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (FuncOnHit.valid())
	{
		FGameObject* OtherGameObject = nullptr;
		if (OtherActor)
		{
			OtherGameObject = OtherActor->GetGameObject();
		}

		if (OtherGameObject)
		{
			auto Result = FuncOnHit(OtherGameObject);
			if (!Result.valid())
			{
				sol::error Err = Result; UE_LOG("[Lua][error] %s\n", Err.what());
#ifdef _EDITOR
				GEngine.EndPIE();
#endif
			}
		}
	}
}

void ULuaScriptComponent::TickComponent(float DeltaTime)
{
	if (FuncTick.valid()) {
		auto Result = FuncTick(DeltaTime);
		if (!Result.valid()) { sol::error Err = Result; UE_LOG("[Lua][error] %s\n", Err.what()); }
	}
}

void ULuaScriptComponent::EndPlay()
{
	if (FuncEndPlay.valid())
	{
		auto Result = FuncEndPlay();
		if (!Result.valid())
		{
			sol::error Err = Result; UE_LOG("[Lua][error] %s\n", Err.what());
#ifdef _EDITOR
			GEngine.EndPIE();
#endif
		}
	}

	// BeginPlay에서 등록한 델리게이트를 대칭적으로 해제
	if (BoundPrimitiveComponent)
	{
		BoundPrimitiveComponent->OnComponentBeginOverlap.Remove(BeginHandleLua);
		BoundPrimitiveComponent->OnComponentEndOverlap.Remove(EndHandleLua);
		BoundPrimitiveComponent->OnComponentHit.Remove(HitHandleLua);
		BoundPrimitiveComponent = nullptr;
	}

	// 모든 Lua 관련 리소스 정리
	CleanupLuaResources();
}

void ULuaScriptComponent::CleanupLuaResources()
{
	// 이미 정리되었다면 중복 실행 방지
	if (bIsLuaCleanedUp)
	{
		return;
	}

	// GetWorld()나 LuaManager가 유효한지 확인 (소멸 시점에는 이미 없을 수 있음)
	if (UWorld* World = GetWorld())
	{
		if (FLuaManager* LuaVM = World->GetLuaManager())
		{
			// 1. 코루틴 정리 (가장 중요. Use-After-Free 방지)
			LuaVM->GetScheduler().CancelByOwner(this);
		}
	}

	// 2. Lua 참조 해제
	FuncBeginPlay = sol::nil;
	FuncTick = sol::nil;
	FuncOnBeginOverlap = sol::nil;
	FuncOnEndOverlap = sol::nil;
	FuncOnHit = sol::nil;
	FuncEndPlay = sol::nil;
	Env = sol::nil;
	Lua = nullptr;

	bIsLuaCleanedUp = true;
}
