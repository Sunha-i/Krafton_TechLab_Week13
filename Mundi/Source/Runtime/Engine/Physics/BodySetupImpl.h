#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// BodySetupImpl.h
// UBodySetup의 PhysX Shape 생성 구현 (Physics 모듈 내부 전용)
// ─────────────────────────────────────────────────────────────────────────────
//
// 주의: 이 헤더는 Physics 모듈 내부에서만 사용해야 합니다.
//       외부 모듈에서 include하지 마세요.
// ─────────────────────────────────────────────────────────────────────────────

#include <PxPhysicsAPI.h>
#include "Vector.h"

class UBodySetup;

/**
 * @brief UBodySetup의 PhysX Shape 생성 헬퍼
 *
 * UBodySetup 헤더에서 PhysX 의존성을 제거하기 위한 헬퍼 네임스페이스.
 * BodyInstance.cpp 등 Physics 모듈 내부에서만 사용합니다.
 */
namespace BodySetupHelper
{
    /**
     * @brief PhysX Shape 생성 및 Actor에 부착
     * @param Setup 형상 데이터 (UBodySetup)
     * @param RigidActor 대상 PxRigidActor
     * @param DefaultMaterial 기본 PxMaterial
     * @param Scale 월드 스케일 (형상 크기에 적용)
     * @return 생성된 PxShape (nullptr이면 실패)
     */
    physx::PxShape* CreatePhysicsShape(
        const UBodySetup* Setup,
        physx::PxRigidActor* RigidActor,
        physx::PxMaterial* DefaultMaterial,
        const FVector& Scale);
}
