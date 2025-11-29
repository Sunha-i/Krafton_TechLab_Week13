#include "pch.h"
#include "PhysicsEventCallback.h"
#include "PhysScene.h"
#include "HitResult.h"
#include "PhysicsTypes.h"
#include "GlobalConsole.h"

using namespace physx;

// ═══════════════════════════════════════════════════════════════════════════
// 생성자
// ═══════════════════════════════════════════════════════════════════════════

FPhysicsEventCallback::FPhysicsEventCallback(FPhysScene* InOwnerScene)
    : OwnerScene(InOwnerScene)
{
    UE_LOG("FPhysicsEventCallback: Event callback created");
}

// ═══════════════════════════════════════════════════════════════════════════
// onContact - 물리적 충돌 이벤트
// ═══════════════════════════════════════════════════════════════════════════

void FPhysicsEventCallback::onContact(
    const PxContactPairHeader& PairHeader,
    const PxContactPair* Pairs,
    PxU32 NbPairs)
{
    // 삭제된 Actor는 무시
    if (PairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_0 ||
        PairHeader.flags & PxContactPairHeaderFlag::eREMOVED_ACTOR_1)
    {
        return;
    }

    // 각 ContactPair 처리
    for (PxU32 i = 0; i < NbPairs; ++i)
    {
        const PxContactPair& ContactPair = Pairs[i];

        // 충돌 시작/유지/종료 확인
        // PxFlags와 int 비교 시 C++20 호환성 문제로 명시적 캐스트 사용
        bool bNewContact = (ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND);
        bool bContactPersist = (ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS);
        bool bContactLost = (ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_LOST);

        if (bNewContact)
        {
            ++TotalContactEvents;

            // 충돌 정보 추출
            FHitResult HitResult;
            ExtractHitResult(ContactPair, PairHeader, true, HitResult);

            // Log collision info (Mundi coordinate system)
            UE_LOG("===========================================================");
            UE_LOG("[Contact] #%u - New collision detected!", TotalContactEvents);
            UE_LOG("  Impact Point (Mundi): (%.2f, %.2f, %.2f)",
                HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y, HitResult.ImpactPoint.Z);
            UE_LOG("  Impact Normal (Mundi): (%.2f, %.2f, %.2f)",
                HitResult.ImpactNormal.X, HitResult.ImpactNormal.Y, HitResult.ImpactNormal.Z);
            UE_LOG("  Penetration Depth: %.4f", HitResult.PenetrationDepth);
            UE_LOG("  Contact Count: %u", ContactPair.contactCount);
            UE_LOG("===========================================================");

            // Phase 2에서 추가할 코드:
            // FBodyInstance* BodyInst0 = static_cast<FBodyInstance*>(PairHeader.actors[0]->userData);
            // FBodyInstance* BodyInst1 = static_cast<FBodyInstance*>(PairHeader.actors[1]->userData);
            // Comp0->OnComponentHit.Broadcast(...);
        }
        else if (bContactLost)
        {
            UE_LOG("[Contact] Collision ended");
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// onTrigger - 트리거 이벤트
// ═══════════════════════════════════════════════════════════════════════════

void FPhysicsEventCallback::onTrigger(PxTriggerPair* Pairs, PxU32 Count)
{
    for (PxU32 i = 0; i < Count; ++i)
    {
        const PxTriggerPair& TriggerPair = Pairs[i];

        // 삭제된 Shape는 무시
        if (TriggerPair.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER |
                                  PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
        {
            continue;
        }

        ++TotalTriggerEvents;

        // Check trigger enter/exit
        if (TriggerPair.status == PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            UE_LOG("===========================================================");
            UE_LOG("[Trigger] #%u - Trigger entered (BeginOverlap)", TotalTriggerEvents);
            UE_LOG("===========================================================");

            // Phase 2: TriggerComp->OnComponentBeginOverlap.Broadcast(...);
        }
        else if (TriggerPair.status == PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            UE_LOG("[Trigger] Trigger exited (EndOverlap)");

            // Phase 2: TriggerComp->OnComponentEndOverlap.Broadcast(...);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// 기타 이벤트 핸들러
// ═══════════════════════════════════════════════════════════════════════════

void FPhysicsEventCallback::onConstraintBreak(PxConstraintInfo* Constraints, PxU32 Count)
{
    for (PxU32 i = 0; i < Count; ++i)
    {
        UE_LOG("[Constraint] Joint broken!");
    }
}

void FPhysicsEventCallback::onWake(PxActor** Actors, PxU32 Count)
{
    // 수면에서 깨어난 Actor 처리 (로그 생략 - 너무 빈번함)
}

void FPhysicsEventCallback::onSleep(PxActor** Actors, PxU32 Count)
{
    // 수면 상태로 전환된 Actor 처리 (로그 생략 - 너무 빈번함)
}

void FPhysicsEventCallback::onAdvance(
    const PxRigidBody* const* BodyBuffer,
    const PxTransform* PoseBuffer,
    const PxU32 Count)
{
    // CCD(Continuous Collision Detection) 고급 이벤트 (필요시 구현)
}

// ═══════════════════════════════════════════════════════════════════════════
// 헬퍼 함수
// ═══════════════════════════════════════════════════════════════════════════

void FPhysicsEventCallback::ExtractHitResult(
    const PxContactPair& ContactPair,
    const PxContactPairHeader& PairHeader,
    bool bIsFirstBody,
    FHitResult& OutHitResult)
{
    OutHitResult.Reset();
    OutHitResult.bBlockingHit = true;

    if (ContactPair.contactCount > 0)
    {
        PxContactPairPoint ContactPoints[64];
        PxU32 ContactCount = ContactPair.extractContacts(ContactPoints, 64);

        if (ContactCount > 0)
        {
            const PxContactPairPoint& FirstContact = ContactPoints[0];

            // PhysX 좌표를 Mundi 좌표로 변환
            OutHitResult.ImpactPoint = PhysicsConversion::ToFVector(FirstContact.position);
            OutHitResult.ImpactNormal = PhysicsConversion::ToFVector(
                bIsFirstBody ? FirstContact.normal : -FirstContact.normal);
            OutHitResult.Location = OutHitResult.ImpactPoint;
            OutHitResult.Normal = OutHitResult.ImpactNormal;
            OutHitResult.PenetrationDepth = FirstContact.separation < 0 ? -FirstContact.separation : 0;
            OutHitResult.bStartPenetrating = FirstContact.separation < 0;
        }
    }
}
