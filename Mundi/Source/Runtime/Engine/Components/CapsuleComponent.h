#pragma once
#include "ShapeComponent.h"

class UCapsuleComponent : public UShapeComponent
{
public:
	DECLARE_CLASS(UCapsuleComponent, UShapeComponent)

	UCapsuleComponent();

protected:
	float CapsuleHalfHeight = WorldAABB.GetHalfExtent().Z;
	float CapsuleRadius = WorldAABB.GetHalfExtent().X < WorldAABB.GetHalfExtent().Y ? WorldAABB.GetHalfExtent().Y : WorldAABB.GetHalfExtent().X;

	void GetShape(FShape& Out) const override; 
};