#pragma once
#include "ShapeElem.h"

class FConvexElem : public FShapeElem
{

    inline static EAggCollisionShape StaticShapeType = EAggCollisionShape::Convex;
};