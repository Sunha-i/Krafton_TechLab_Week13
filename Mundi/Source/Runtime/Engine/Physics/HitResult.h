#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// HitResult.h
// 충돌 결과 정보 구조체
// ─────────────────────────────────────────────────────────────────────────────

#include "Vector.h"
#include "UEContainer.h"

class AActor;
class UPrimitiveComponent;

/**
 * @brief 충돌 결과 정보
 *
 * PhysX의 충돌 정보를 게임 레이어에서 사용할 수 있는 형태로 변환한 구조체.
 * Mundi 좌표계로 변환된 값을 저장.
 */
struct FHitResult
{
    // ═══════════════════════════════════════════════════════════════════════
    // 충돌 위치 정보 (Mundi 좌표계)
    // ═══════════════════════════════════════════════════════════════════════

    /** 충돌 발생 여부 (blocking hit이 있으면 true) */
    bool bBlockingHit = false;

    /** 트레이스 시작 시점에 이미 다른 물체와 겹쳐 있었는지 여부 */
    bool bStartPenetrating = false;

    /**
     * 실제 충돌이 발생한 표면의 정확한 위치 (월드 좌표계)
     *
     * - Line Trace: 레이가 표면에 맞은 정확한 점
     * - Sweep Trace: 트레이스 형상(구체/박스 등)이 표면에 닿은 접촉점
     *
     * 예: 구체가 벽에 닿으면, 벽 표면에서 구체가 실제로 접촉한 점
     */
    FVector ImpactPoint = FVector::Zero();

    /**
     * 충돌한 표면의 수직 벡터 (월드 좌표계, 정규화됨)
     *
     * 표면이 "바라보는" 방향을 나타냄
     * - 벽을 맞추면: 벽이 향하는 방향 (예: 동쪽 벽이면 서쪽 방향)
     * - 바닥을 맞추면: 위쪽 방향 (0, 0, 1)
     *
     * 파티클 효과 방향, 반사 계산, 밀어내기 방향 등에 사용
     */
    FVector ImpactNormal = FVector::Zero();

    /**
     * 충돌 시점에 트레이스 형상의 중심 위치 (월드 좌표계)
     *
     * - Line Trace: ImpactPoint와 동일 (선에는 두께가 없음)
     * - Sweep Trace: 충돌 시 트레이스 형상(구체/박스)의 중심 좌표
     *                표면에서 형상의 반지름만큼 떨어진 위치
     *
     * 예: 반지름 50의 구체가 벽에 닿으면,
     *     Location은 벽에서 50만큼 떨어진 구체의 중심점
     */
    FVector Location = FVector::Zero();

    /**
     * 트레이스 방향에 대한 표면의 노멀 벡터 (월드 좌표계, 정규화됨)
     *
     * 대부분의 경우 ImpactNormal과 동일
     * 복잡한 지오메트리에서 트레이스 방향을 고려한 노멀이 필요할 때 사용
     */
    FVector Normal = FVector::Zero();

    /**
     * 침투 깊이 (물체가 겹쳐 있는 정도)
     *
     * bStartPenetrating이 true일 때 의미 있는 값
     * 물체를 밀어내야 할 거리를 나타냄
     */
    float PenetrationDepth = 0.0f;

    /**
     * 트레이스 시작점부터 충돌점까지의 거리 비율 (0.0 ~ 1.0)
     *
     * - 0.0: 시작점에서 즉시 충돌
     * - 0.5: 트레이스 거리의 절반 지점에서 충돌
     * - 1.0: 충돌 없음 (끝점까지 도달)
     */
    float Time = 1.0f;

    /**
     * 트레이스 시작점부터 충돌점까지의 실제 거리 (단위: cm)
     *
     * Time * TraceLength로 계산됨
     */
    float Distance = 0.0f;

    // ═══════════════════════════════════════════════════════════════════════
    // 충돌 대상 정보
    // ═══════════════════════════════════════════════════════════════════════

    /** 충돌한 Actor */
    AActor* HitActor = nullptr;

    /** 충돌한 Component */
    UPrimitiveComponent* HitComponent = nullptr;

    /** 충돌한 본(Bone) 이름 (스켈레탈 메시용) */
    FString BoneName;

    /** 충돌한 Face 인덱스 */
    int32 FaceIndex = -1;

    // ═══════════════════════════════════════════════════════════════════════
    // 유틸리티
    // ═══════════════════════════════════════════════════════════════════════

    /** 결과 초기화 */
    void Reset()
    {
        bBlockingHit = false;
        bStartPenetrating = false;
        ImpactPoint = FVector::Zero();
        ImpactNormal = FVector::Zero();
        Location = FVector::Zero();
        Normal = FVector::Zero();
        PenetrationDepth = 0.0f;
        Time = 1.0f;
        Distance = 0.0f;
        HitActor = nullptr;
        HitComponent = nullptr;
        BoneName.clear();
        FaceIndex = -1;
    }

    /** 유효한 충돌인지 확인 */
    bool IsValidBlockingHit() const
    {
        return bBlockingHit;
    }
};

/**
 * @brief 오버랩 결과 정보
 */
struct FOverlapResult
{
    /** 오버랩한 Actor */
    AActor* Actor = nullptr;

    /** 오버랩한 Component */
    UPrimitiveComponent* Component = nullptr;

    /** 본 인덱스 (스켈레탈 메시용) */
    int32 ItemIndex = -1;

    /** 유효한 오버랩인지 확인 */
    bool IsValid() const
    {
        return Actor != nullptr || Component != nullptr;
    }
};
