#pragma once

#include "PrimitiveComponent.h"
#include "UShapeComponent.generated.h"

enum class EShapeKind : uint8
{
	Box = 0,
	Sphere,
	Capsule,
};

struct FBoxShape { FVector BoxExtent; };
struct FSphereShape { float SphereRadius; };
struct FCapsuleSphere { float CapsuleRadius, CapsuleHalfHeight; };

struct FShape
{
	FShape() {}

	EShapeKind Kind; 
	union {
		FBoxShape Box;
		FSphereShape Sphere;
		FCapsuleSphere Capsule;
	};
}; 

UCLASS(DisplayName="셰이프 컴포넌트", Description="충돌 모양 기본 컴포넌트입니다")
class UShapeComponent : public UPrimitiveComponent
{ 
public:  

	GENERATED_REFLECTION_BODY();

public:

    // ===== Lua-Bindable Properties (Auto-moved from protected/private) =====

	UPROPERTY(EditAnywhere, Category="Shape")
	bool bShapeIsVisible;

	UPROPERTY(EditAnywhere, Category="Shape")
	bool bShapeHiddenInGame;

	UShapeComponent();

protected:
	~UShapeComponent() override;

public:
	virtual void TickComponent(float DeltaSeconds) override;

	virtual void GetShape(FShape& OutShape) const {};
	virtual void BeginPlay() override;
	virtual void EndPlay() override;
    virtual void OnRegister(UWorld* InWorld) override;
	virtual void OnUnregister() override;
    virtual void OnTransformUpdated() override;

    void UpdateOverlaps();

    // Bounds 업데이트 (자식 클래스에서 구현)
    virtual void UpdateBounds() {}

    FAABB GetWorldAABB() const override;
	virtual const TArray<FOverlapInfo>& GetOverlapInfos() const override { return OverlapInfos; }

	// Serialization
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	// Duplication
	virtual void DuplicateSubObjects() override;

	// ═══════════════════════════════════════════════════════════════════════
	// BodySetup (Archetype 패턴 - 기본값 공유 + Copy-on-Write)
	// ═══════════════════════════════════════════════════════════════════════

	/** 이 Shape의 BodySetup 반환 (Archetype 또는 자체 소유) */
	virtual UBodySetup* GetBodySetup() const override;

protected:
	/** Shape용 BodySetup (nullptr이면 Archetype 사용) */
	UBodySetup* ShapeBodySetup = nullptr;

	/** Archetype(기본값) BodySetup 사용 여부 */
	bool bUseArchetypeBodySetup = true;

	/** 기본 BodySetup 반환 (파생 클래스에서 구현) */
	virtual UBodySetup* GetDefaultBodySetup() const { return nullptr; }

	/** 현재 파라미터가 기본값과 같은지 확인 (파생 클래스에서 구현) */
	virtual bool IsUsingDefaultParameters() const { return true; }

	/** 자체 BodySetup 생성이 필요한지 확인하고 필요시 생성 */
	void EnsureBodySetupIsValid();

	/** BodySetup 생성/업데이트 (Shape 크기 변경 시 호출) */
	virtual void UpdateBodySetup();

	// ㅡㅡㅡㅡㅡㅡㅡㅡㅡ디버깅용ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ

	mutable FAABB WorldAABB; //브로드 페이즈 용
	TSet<UShapeComponent*> OverlapNow; // 이번 프레임에서 overlap 된 Shap Comps
	TSet<UShapeComponent*> OverlapPrev; // 지난 프레임에서 overlap 됐으면 Cache

	bool bIsOverlapping = false;  // 충돌 상태 플래그 (Week09 호환)
	 

	FVector4 ShapeColor ;
	bool bDrawOnlyIfSelected;


	TArray<FOverlapInfo> OverlapInfos; 
	//TODO: float LineThickness;

};
