#pragma once
struct FAABB;
struct FOBB;

namespace Collision
{
    bool Intersects(const FAABB& Aabb, const FOBB& Obb);
}