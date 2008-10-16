/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephen Deken, stephen.deken@gmail.com
 */

#include <AffineTransform.h>
#include <Point.h>

BAffineTransform::BAffineTransform()
	: fTransformMatrix()
{
}


BAffineTransform::BAffineTransform(const BAffineTransform& copyFrom)
	: fTransformMatrix(copyFrom.fTransformMatrix)
{
}


BAffineTransform::~BAffineTransform()
{
}


BAffineTransform&
BAffineTransform::operator=(const BAffineTransform& copyFrom)
{
	if (copyFrom != *this) {
		fTransformMatrix.reset();
		fTransformMatrix.multiply(copyFrom.fTransformMatrix);
		TransformationChanged();
	}
	return *this;
}


bool
BAffineTransform::operator==(const BAffineTransform& other) const
{
	return fTransformMatrix == other.fTransformMatrix;
}


bool
BAffineTransform::operator!=(const BAffineTransform& other) const
{
	return fTransformMatrix != other.fTransformMatrix;
}


void
BAffineTransform::TransformationChanged() const
{
	// default implementation does nothing
}


BPoint
BAffineTransform::Apply(const BPoint& point) const
{
	BPoint rval(point);
	_TransformPoint(rval);
	return rval;
}


void
BAffineTransform::Apply(BPoint* points, uint32 count) const
{
	for (uint32 i = 0; i < count; ++i)
		_TransformPoint(points[i]);
}


void
BAffineTransform::_Rotate(float angle)
{
	if (angle != 0.0)
		fTransformMatrix.multiply(agg::trans_affine_rotation(angle));
}


void
BAffineTransform::_Scale(float scaleX, float scaleY)
{
	if (scaleX != 0.0 || scaleY != 0.0)
		fTransformMatrix.multiply(agg::trans_affine_scaling(scaleX, scaleY));
}


void
BAffineTransform::_Translate(float deltaX, float deltaY)
{
	if (deltaX != 0.0 || deltaY != 0.0)
		fTransformMatrix.multiply(agg::trans_affine_translation(deltaX, deltaY));
}


void
BAffineTransform::_Shear(float shearX, float shearY)
{
	if (shearX != 0.0 || shearY != 0.0)
		fTransformMatrix.multiply(agg::trans_affine_skewing(shearX, shearY));
}


void
BAffineTransform::Rotate(float angle)
{
	_Rotate(angle);
	TransformationChanged();
}


void
BAffineTransform::Rotate(const BPoint& center, float angle)
{
	_Translate(-center.x, -center.y);
	_Rotate(angle);
	_Translate(center.x, center.y);
	TransformationChanged();
}


BAffineTransform&
BAffineTransform::RotateBySelf(float angle)
{
	Rotate(angle);
	return *this;
}


BAffineTransform&
BAffineTransform::RotateBySelf(const BPoint& center, float angle)
{
	Rotate(center, angle);
	return *this;
}


BAffineTransform
BAffineTransform::RotateByCopy(float angle) const
{
	BAffineTransform copy(*this);
	copy.Rotate(angle);
	return copy;
}


BAffineTransform
BAffineTransform::RotateByCopy(const BPoint& center, float angle) const
{
	BAffineTransform copy(*this);
	copy.Rotate(center, angle);
	return copy;
}


void
BAffineTransform::Translate(float deltaX, float deltaY)
{
	_Translate(deltaX, deltaY);
	TransformationChanged();
}


void
BAffineTransform::Translate(const BPoint& delta)
{
	Translate(delta.x, delta.y);
}


BAffineTransform&
BAffineTransform::TranslateBySelf(float deltaX, float deltaY)
{
	Translate(deltaX, deltaY);
	return *this;
}


BAffineTransform&
BAffineTransform::TranslateBySelf(const BPoint& delta)
{
	return TranslateBySelf(delta.x, delta.y);
}


BAffineTransform
BAffineTransform::TranslateByCopy(float deltaX, float deltaY) const
{
	BAffineTransform copy(*this);
	copy.Translate(deltaX, deltaY);
	return copy;
}


BAffineTransform
BAffineTransform::TranslateByCopy(const BPoint& delta) const
{
	return TranslateByCopy(delta.x, delta.y);
}


void
BAffineTransform::Scale(float scale)
{
	Scale(scale, scale);
}


void
BAffineTransform::Scale(const BPoint& center, float scale)
{
	Scale(center, scale, scale);
}


void
BAffineTransform::Scale(float scaleX, float scaleY)
{
	_Scale(scaleX, scaleY);
	TransformationChanged();
}


void
BAffineTransform::Scale(const BPoint& center, float scaleX, float scaleY)
{
	_Translate(-center.x, -center.y);
	_Scale(scaleX, scaleY);
	_Translate(center.x, center.y);
	TransformationChanged();
}


void
BAffineTransform::Scale(const BPoint& scale)
{
	Scale(scale.x, scale.y);
}


void
BAffineTransform::Scale(const BPoint& center, const BPoint& scale)
{
	Scale(center, scale.x, scale.y);
}


BAffineTransform&
BAffineTransform::ScaleBySelf(float scale)
{
	return ScaleBySelf(scale, scale);
}


BAffineTransform&
BAffineTransform::ScaleBySelf(const BPoint& center, float scale)
{
	return ScaleBySelf(center, scale, scale);
}


BAffineTransform&
BAffineTransform::ScaleBySelf(float scaleX, float scaleY)
{
	Scale(scaleX, scaleY);
	return *this;
}


BAffineTransform&
BAffineTransform::ScaleBySelf(const BPoint& center, float scaleX, float scaleY)
{
	Scale(center, scaleX, scaleY);
	return *this;
}


BAffineTransform&
BAffineTransform::ScaleBySelf(const BPoint& scale)
{
	return ScaleBySelf(scale.x, scale.y);
}


BAffineTransform&
BAffineTransform::ScaleBySelf(const BPoint& center, const BPoint& scale)
{
	return ScaleBySelf(center, scale.x, scale.y);
}


BAffineTransform
BAffineTransform::ScaleByCopy(float scale) const
{
	return ScaleByCopy(scale, scale);
}


BAffineTransform
BAffineTransform::ScaleByCopy(const BPoint& center, float scale) const
{
	return ScaleByCopy(center, scale, scale);
}


BAffineTransform
BAffineTransform::ScaleByCopy(float scaleX, float scaleY) const
{
	BAffineTransform copy(*this);
	copy.Scale(scaleX, scaleY);
	return copy;
}


BAffineTransform
BAffineTransform::ScaleByCopy(const BPoint& center, float scaleX, float scaleY) const
{
	BAffineTransform copy(*this);
	copy.Scale(center, scaleX, scaleY);
	return copy;
}


BAffineTransform
BAffineTransform::ScaleByCopy(const BPoint& scale) const
{
	return ScaleByCopy(scale.x, scale.y);
}


BAffineTransform
BAffineTransform::ScaleByCopy(const BPoint& center, const BPoint& scale) const
{
	return ScaleByCopy(center, scale.x, scale.y);
}


void
BAffineTransform::Shear(float shearX, float shearY)
{
	_Shear(shearX, shearY);
	TransformationChanged();
}


void
BAffineTransform::Shear(const BPoint& center, float shearX, float shearY)
{
	_Translate(-center.x, -center.y);
	_Shear(shearX, shearY);
	_Translate(center.x, center.y);
	TransformationChanged();
}


void
BAffineTransform::Shear(const BPoint& shear)
{
	Shear(shear.x, shear.y);
}


void
BAffineTransform::Shear(const BPoint& center, const BPoint& shear)
{
	Shear(center, shear.x, shear.y);
}


BAffineTransform&
BAffineTransform::ShearBySelf(float shearX, float shearY)
{
	Shear(shearX, shearY);
	return *this;
}


BAffineTransform&
BAffineTransform::ShearBySelf(const BPoint& center, float shearX, float shearY)
{
	Shear(center, shearX, shearY);
	return *this;
}


BAffineTransform&
BAffineTransform::ShearBySelf(const BPoint& shear)
{
	Shear(shear);
	return *this;
}


BAffineTransform&
BAffineTransform::ShearBySelf(const BPoint& center, const BPoint& shear)
{
	Shear(center, shear);
	return *this;
}


BAffineTransform
BAffineTransform::ShearByCopy(float shearX, float shearY) const
{
	BAffineTransform copy(*this);
	copy.Shear(shearX, shearY);
	return copy;
}


BAffineTransform
BAffineTransform::ShearByCopy(const BPoint& center, float shearX, float shearY) const
{
	BAffineTransform copy(*this);
	copy.Shear(center, shearX, shearY);
	return copy;
}


BAffineTransform
BAffineTransform::ShearByCopy(const BPoint& shear) const
{
	BAffineTransform copy(*this);
	copy.Shear(shear);
	return copy;
}


BAffineTransform
BAffineTransform::ShearByCopy(const BPoint& center, const BPoint& shear) const
{
	BAffineTransform copy(*this);
	copy.Shear(center, shear);
	return copy;
}


void
BAffineTransform::_TransformPoint(BPoint& point) const
{
	double x = point.x;
	double y = point.y;
	fTransformMatrix.transform(&x, &y);
	point.x = x;
	point.y = y;
}
