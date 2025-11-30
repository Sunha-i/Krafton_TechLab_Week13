#include "pch.h"
#include "BodyInstanceImpl.h"

using namespace physx;

// ═══════════════════════════════════════════════════════════════════════════
// 헬퍼 함수
// ═══════════════════════════════════════════════════════════════════════════

PxRigidDynamic* FBodyInstanceImpl::GetPxRigidDynamic() const
{
    if (!RigidActorSync)
    {
        return nullptr;
    }

    return RigidActorSync->is<PxRigidDynamic>();
}
