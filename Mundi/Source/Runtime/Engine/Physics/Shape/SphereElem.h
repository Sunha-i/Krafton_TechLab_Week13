#pragma once
#include "ShapeElem.h"

class FSphereElem : public FShapeElem
{

    inline static EAggCollisionShape StaticShapeType = EAggCollisionShape::Sphere;
};