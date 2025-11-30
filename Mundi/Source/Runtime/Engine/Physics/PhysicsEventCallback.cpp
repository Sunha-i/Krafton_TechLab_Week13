#include "pch.h"
#include "PhysicsEventCallback.h"
#include "PhysScene.h"
#include "HitResult.h"
#include "PhysicsTypes.h"
#include "BodyInstance.h"
#include "PrimitiveComponent.h"
#include "Actor.h"
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

    // userData에서 FBodyInstance 추출
    FBodyInstance* BodyInst0 = static_cast<FBodyInstance*>(PairHeader.actors[0]->userData);
    FBodyInstance* BodyInst1 = static_cast<FBodyInstance*>(PairHeader.actors[1]->userData);

    // 테스트용 Actor는 userData가 없음
    if (!BodyInst0 || !BodyInst1)
    {
        // Phase 1 로그 출력 (기존 코드 유지)
        for (PxU32 i = 0; i < NbPairs; ++i)
        {
            const PxContactPair& ContactPair = Pairs[i];
            bool bNewContact = (ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND);
            if (bNewContact)
            {
                ++TotalContactEvents;

                // 충돌 정보 추출
                FHitResult HitResult;
                ExtractHitResult(ContactPair, PairHeader, true, HitResult);

                UE_LOG("===========================================================");
                UE_LOG("[Contact] #%u - Collision (no BodyInstance)", TotalContactEvents);
                UE_LOG("  Impact Point (Mundi): (%.2f, %.2f, %.2f)",
                    HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y, HitResult.ImpactPoint.Z);
                UE_LOG("===========================================================");
            }
        }
        return;
    }

    UPrimitiveComponent* Comp0 = BodyInst0->OwnerComponent;
    UPrimitiveComponent* Comp1 = BodyInst1->OwnerComponent;

    if (!Comp0 || !Comp1)
    {
        return;
    }

    // 각 ContactPair 처리
    for (PxU32 i = 0; i < NbPairs; ++i)
    {
        const PxContactPair& ContactPair = Pairs[i];

        // 충돌 시작/유지/종료 확인
        bool bNewContact = (ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND);
        bool bContactLost = (ContactPair.events & PxPairFlag::eNOTIFY_TOUCH_LOST);

        if (bNewContact)
        {
            ++TotalContactEvents;

            // HitResult 추출
            FHitResult HitResult0, HitResult1;
            ExtractHitResult(ContactPair, PairHeader, true, HitResult0);
            ExtractHitResult(ContactPair, PairHeader, false, HitResult1);

            // 임펄스 계산
            PxVec3 TotalImpulse(0);
            if (ContactPair.contactCount > 0)
            {
                PxContactPairPoint ContactPoints[64];
                PxU32 ContactCount = ContactPair.extractContacts(ContactPoints, 64);
                for (PxU32 j = 0; j < ContactCount; ++j)
                {
                    TotalImpulse += ContactPoints[j].impulse;
                }
            }
            FVector NormalImpulse = PhysicsConversion::ToFVector(TotalImpulse);

            // 델리게이트 브로드캐스트
            AActor* Owner0 = Comp0->GetOwner();
            AActor* Owner1 = Comp1->GetOwner();

            // Comp0에 Hit 이벤트 전달
            HitResult0.HitActor = Owner1;
            HitResult0.HitComponent = Comp1;
            Comp0->OnComponentHit.Broadcast(Comp0, Owner1, Comp1, NormalImpulse, HitResult0);

            // Comp1에 Hit 이벤트 전달
            HitResult1.HitActor = Owner0;
            HitResult1.HitComponent = Comp0;
            Comp1->OnComponentHit.Broadcast(Comp1, Owner0, Comp0, -NormalImpulse, HitResult1);

            UE_LOG("[Contact] #%u - Hit event broadcast (Impulse: %.2f, %.2f, %.2f)",
                TotalContactEvents, NormalImpulse.X, NormalImpulse.Y, NormalImpulse.Z);
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

        // userData에서 FBodyInstance 추출
        FBodyInstance* TriggerBody = static_cast<FBodyInstance*>(TriggerPair.triggerActor->userData);
        FBodyInstance* OtherBody = static_cast<FBodyInstance*>(TriggerPair.otherActor->userData);

        if (!TriggerBody || !OtherBody)
        {
            ++TotalTriggerEvents;
            UE_LOG("[Trigger] #%u - Trigger event (no BodyInstance)", TotalTriggerEvents);
            continue;
        }

        UPrimitiveComponent* TriggerComp = TriggerBody->OwnerComponent;
        UPrimitiveComponent* OtherComp = OtherBody->OwnerComponent;

        if (!TriggerComp || !OtherComp)
        {
            continue;
        }

        AActor* TriggerOwner = TriggerComp->GetOwner();
        AActor* OtherOwner = OtherComp->GetOwner();

        ++TotalTriggerEvents;

        if (TriggerPair.status == PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            // BeginOverlap 이벤트 - UPrimitiveComponent 델리게이트로 브로드캐스트
            FHitResult EmptyHit;

            // 트리거 컴포넌트에 이벤트 전달
            TriggerComp->OnComponentBeginOverlap.Broadcast(
                TriggerComp, OtherOwner, OtherComp, 0, false, EmptyHit);

            // 상대 컴포넌트에 이벤트 전달
            OtherComp->OnComponentBeginOverlap.Broadcast(
                OtherComp, TriggerOwner, TriggerComp, 0, false, EmptyHit);

            UE_LOG("[Trigger] #%u - BeginOverlap broadcast", TotalTriggerEvents);
        }
        else if (TriggerPair.status == PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            // EndOverlap 이벤트 - UPrimitiveComponent 델리게이트로 브로드캐스트

            // 트리거 컴포넌트에 이벤트 전달
            TriggerComp->OnComponentEndOverlap.Broadcast(
                TriggerComp, OtherOwner, OtherComp, 0);

            // 상대 컴포넌트에 이벤트 전달
            OtherComp->OnComponentEndOverlap.Broadcast(
                OtherComp, TriggerOwner, TriggerComp, 0);

            UE_LOG("[Trigger] #%u - EndOverlap broadcast", TotalTriggerEvents);
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
