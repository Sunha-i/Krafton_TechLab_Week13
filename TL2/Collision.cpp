#include "pch.h"
#include "Collision.h"
#include "AABB.h"
#include "OBB.h"

namespace Collision
{
    bool Intersects(const FAABB& Aabb, const FOBB& Obb)
    {
        const FOBB ConvertedOBB(Aabb, FMatrix::Identity());
        return ConvertedOBB.Intersects(Obb);
    }
    
}
