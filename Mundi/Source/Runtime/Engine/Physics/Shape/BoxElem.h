#pragma once
#include "ShapeElem.h"

class FBoxElem : public FShapeElem
{

    inline static EAggCollisionShape StaticShapeType = EAggCollisionShape::Box;
};