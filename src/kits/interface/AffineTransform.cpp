/*
 * Copyright 2008-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephen Deken, stephen.deken@gmail.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */
//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------

#include <AffineTransform.h>

#include <TypeConstants.h>


const BAffineTransform B_AFFINE_IDENTITY_TRANSFORM;


BAffineTransform::BAffineTransform()
	:
	sx(1.0),
	shy(0.0),
	shx(0.0),
	sy(1.0),
	tx(0.0),
	ty(0.0)
{
}


BAffineTransform::BAffineTransform(double sx, double shy, double shx,
		double sy, double tx, double ty)
	:
	sx(sx),
	shy(shy),
	shx(shx),
	sy(sy),
	tx(tx),
	ty(ty)
{
}


BAffineTransform::BAffineTransform(const BAffineTransform& other)
	:
	sx(other.sx),
	shy(other.shy),
	shx(other.shx),
	sy(other.sy),
	tx(other.tx),
	ty(other.ty)
{
}


BAffineTransform::~BAffineTransform()
{
}


// #pragma mark -


bool
BAffineTransform::IsFixedSize() const
{
	return true;
}


type_code
BAffineTransform::TypeCode() const
{
	return B_AFFINE_TRANSFORM_TYPE;
}


ssize_t
BAffineTransform::FlattenedSize() const
{
	return 6 * sizeof(double);
}


status_t
BAffineTransform::Flatten(void* _buffer, ssize_t size) const
{
	if (_buffer == NULL || size < FlattenedSize())
		return B_BAD_VALUE;

	double* buffer = reinterpret_cast<double*>(_buffer);

	buffer[0] = sx;
	buffer[1] = shy;
	buffer[2] = shx;
	buffer[3] = sy;
	buffer[4] = tx;
	buffer[5] = ty;

	return B_OK;
}


status_t
BAffineTransform::Unflatten(type_code code, const void* _buffer, ssize_t size)
{
	if (_buffer == NULL || size < FlattenedSize() || code != TypeCode())
		return B_BAD_VALUE;

	const double* buffer = reinterpret_cast<const double*>(_buffer);

	sx = buffer[0];
	shy = buffer[1];
	shx = buffer[2];
	sy = buffer[3];
	tx = buffer[4];
	ty = buffer[5];

	return B_OK;
}


// #pragma mark -


/*static*/ BAffineTransform
BAffineTransform::AffineTranslation(double x, double y)
{
	return BAffineTransform(1.0, 0.0, 0.0, 1.0, x, y);
}


/*static*/ BAffineTransform
BAffineTransform::AffineRotation(double angle)
{
	return BAffineTransform(cos(angle), sin(angle), -sin(angle), cos(angle),
		0.0, 0.0);
}


/*static*/ BAffineTransform
BAffineTransform::AffineScaling(double x, double y)
{
	return BAffineTransform(x, 0.0, 0.0, y, 0.0, 0.0);
}


/*static*/ BAffineTransform
BAffineTransform::AffineScaling(double scale)
{
	return BAffineTransform(scale, 0.0, 0.0, scale, 0.0, 0.0);
}


/*static*/ BAffineTransform
BAffineTransform::AffineShearing(double x, double y)
{
	return BAffineTransform(1.0, tan(y), tan(x), 1.0, 0.0, 0.0);
}


// #pragma mark -


BPoint
BAffineTransform::Apply(const BPoint& point) const
{
	double x = point.x;
	double y = point.y;
	Apply(&x, &y);
	return BPoint(x, y);
}


BPoint
BAffineTransform::ApplyInverse(const BPoint& point) const
{
	double x = point.x;
	double y = point.y;
	ApplyInverse(&x, &y);
	return BPoint(x, y);
}


void
BAffineTransform::Apply(BPoint* point) const
{
	if (point == NULL)
		return;
	double x = point->x;
	double y = point->y;
	Apply(&x, &y);
	point->x = x;
	point->y = y;
}


void
BAffineTransform::ApplyInverse(BPoint* point) const
{
	if (point == NULL)
		return;
	double x = point->x;
	double y = point->y;
	ApplyInverse(&x, &y);
	point->x = x;
	point->y = y;
}


void
BAffineTransform::Apply(BPoint* points, uint32 count) const
{
	if (points != NULL) {
		for (uint32 i = 0; i < count; ++i)
			Apply(&points[i]);
	}
}


void
BAffineTransform::ApplyInverse(BPoint* points, uint32 count) const
{
	if (points != NULL) {
		for (uint32 i = 0; i < count; ++i)
			ApplyInverse(&points[i]);
	}
}


// #pragma mark -


const BAffineTransform&
BAffineTransform::TranslateBy(const BPoint& delta)
{
	return TranslateBy(delta.x, delta.y);
}


BAffineTransform
BAffineTransform::TranslateByCopy(double x, double y) const
{
	BAffineTransform copy(*this);
	copy.TranslateBy(x, y);
	return copy;
}


BAffineTransform
BAffineTransform::TranslateByCopy(const BPoint& delta) const
{
	return TranslateByCopy(delta.x, delta.y);
}


// #pragma mark -


const BAffineTransform&
BAffineTransform::RotateBy(const BPoint& center, double angle)
{
	TranslateBy(-center.x, -center.y);
	RotateBy(angle);
	return TranslateBy(center.x, center.y);
}


BAffineTransform
BAffineTransform::RotateByCopy(double angle) const
{
	BAffineTransform copy(*this);
	copy.RotateBy(angle);
	return copy;
}


BAffineTransform
BAffineTransform::RotateByCopy(const BPoint& center, double angle) const
{
	BAffineTransform copy(*this);
	copy.RotateBy(center, angle);
	return copy;
}


// #pragma mark -


const BAffineTransform&
BAffineTransform::ScaleBy(const BPoint& center, double scale)
{
	return ScaleBy(center, scale, scale);
}


const BAffineTransform&
BAffineTransform::ScaleBy(const BPoint& center, double x, double y)
{
	TranslateBy(-center.x, -center.y);
	ScaleBy(x, y);
	return TranslateBy(center.x, center.y);
}


const BAffineTransform&
BAffineTransform::ScaleBy(const BPoint& scale)
{
	return ScaleBy(scale.x, scale.y);
}


const BAffineTransform&
BAffineTransform::ScaleBy(const BPoint& center, const BPoint& scale)
{
	return ScaleBy(center, scale.x, scale.y);
}


BAffineTransform
BAffineTransform::ScaleByCopy(double scale) const
{
	return ScaleByCopy(scale, scale);
}


BAffineTransform
BAffineTransform::ScaleByCopy(const BPoint& center, double scale) const
{
	return ScaleByCopy(center, scale, scale);
}


BAffineTransform
BAffineTransform::ScaleByCopy(double x, double y) const
{
	BAffineTransform copy(*this);
	copy.ScaleBy(x, y);
	return copy;
}


BAffineTransform
BAffineTransform::ScaleByCopy(const BPoint& center, double x, double y) const
{
	BAffineTransform copy(*this);
	copy.ScaleBy(center, x, y);
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


const BAffineTransform&
BAffineTransform::SetScale(double scale)
{
	return SetScale(scale, scale);
}


const BAffineTransform&
BAffineTransform::SetScale(double x, double y)
{
	double tx;
	double ty;
	double rotation;
	double shearX;
	double shearY;
	if (!GetAffineParameters(&tx, &ty, &rotation, NULL, NULL,
			&shearX, &shearY)) {
		return *this;
	}

	BAffineTransform result;
	result.ShearBy(shearX, shearY);
	result.ScaleBy(x, y);
	result.RotateBy(rotation);
	result.TranslateBy(tx, ty);

	return *this = result;
}


// #pragma mark -


const BAffineTransform&
BAffineTransform::ShearBy(const BPoint& center, double x, double y)
{
	TranslateBy(-center.x, -center.y);
	ShearBy(x, y);
	return TranslateBy(center.x, center.y);
}


const BAffineTransform&
BAffineTransform::ShearBy(const BPoint& shear)
{
	return ShearBy(shear.x, shear.y);
}


const BAffineTransform&
BAffineTransform::ShearBy(const BPoint& center, const BPoint& shear)
{
	return ShearBy(center, shear.x, shear.y);
}


BAffineTransform
BAffineTransform::ShearByCopy(double x, double y) const
{
	BAffineTransform copy(*this);
	copy.ShearBy(x, y);
	return copy;
}


BAffineTransform
BAffineTransform::ShearByCopy(const BPoint& center, double x, double y) const
{
	BAffineTransform copy(*this);
	copy.ShearBy(center, x, y);
	return copy;
}


BAffineTransform
BAffineTransform::ShearByCopy(const BPoint& shear) const
{
	BAffineTransform copy(*this);
	copy.ShearBy(shear);
	return copy;
}


BAffineTransform
BAffineTransform::ShearByCopy(const BPoint& center, const BPoint& shear) const
{
	BAffineTransform copy(*this);
	copy.ShearBy(center, shear);
	return copy;
}


// #pragma mark -


const BAffineTransform&
BAffineTransform::PreMultiply(const BAffineTransform& other)
{
	double t0 = sx * other.sx + shy * other.shx;
	double t2 = shx * other.sx + sy * other.shx;
	double t4 = tx * other.sx + ty * other.shx + other.tx;
	shy = sx * other.shy + shy * other.sy;
	sy = shx * other.shy + sy * other.sy;
	ty = tx * other.shy + ty * other.sy + other.ty;
	sx = t0;
	shx = t2;
	tx = t4;
	return *this;
}


bool
BAffineTransform::IsValid(double epsilon) const
{
	return fabs(sx) > epsilon && fabs(sy) > epsilon;
}


static inline bool
IsEqualEpsilon(double v1, double v2, double epsilon)
{
    return fabs(v1 - v2) <= double(epsilon);
}


bool
BAffineTransform::IsIdentity(double epsilon) const
{
	return IsEqualEpsilon(sx, 1.0, epsilon)
		&& IsEqualEpsilon(shy, 0.0, epsilon)
		&& IsEqualEpsilon(shx, 0.0, epsilon)
		&& IsEqualEpsilon(sy, 1.0, epsilon)
		&& IsEqualEpsilon(tx, 0.0, epsilon)
		&& IsEqualEpsilon(ty, 0.0, epsilon);
}


bool
BAffineTransform::IsEqual(const BAffineTransform& other, double epsilon) const
{
	return IsEqualEpsilon(sx, other.sx, epsilon)
		&& IsEqualEpsilon(shy, other.shy, epsilon)
		&& IsEqualEpsilon(shx, other.shx, epsilon)
		&& IsEqualEpsilon(sy, other.sy, epsilon)
		&& IsEqualEpsilon(tx, other.tx, epsilon)
		&& IsEqualEpsilon(ty, other.ty, epsilon);
}


const BAffineTransform&
BAffineTransform::Invert()
{
	double d  = InverseDeterminant();

	double t0 = sy * d;
	sy =  sx * d;
	shy = -shy * d;
	shx = -shx * d;

	double t4 = -tx * t0 - ty * shx;
	ty = -tx * shy - ty * sy;

	sx = t0;
	tx = t4;

    return *this;
}


const BAffineTransform&
BAffineTransform::FlipX()
{
	sx = -sx;
	shy = -shy;
	tx = -tx;
	return *this;
}


const BAffineTransform&
BAffineTransform::FlipY()
{
	shx = -shx;
	sy = -sy;
	ty = -ty;
	return *this;
}


const BAffineTransform&
BAffineTransform::Reset()
{
	sx = sy = 1.0;
	shy = shx = tx = ty = 0.0;
	return *this;
}


void
BAffineTransform::GetTranslation(double* _tx, double* _ty) const
{
	if (_tx)
		*_tx = tx;
	if (_ty)
		*_ty = ty;
}


double
BAffineTransform::Rotation() const
{
	double x1 = 0.0;
	double y1 = 0.0;
	double x2 = 1.0;
	double y2 = 0.0;
	Apply(&x1, &y1);
	Apply(&x2, &y2);
	return atan2(y2 - y1, x2 - x1);
}


double
BAffineTransform::Scale() const
{
	double x = 0.707106781 * sx + 0.707106781 * shx;
	double y = 0.707106781 * shy + 0.707106781 * sy;
	return sqrt(x * x + y * y);
}


void
BAffineTransform::GetScale(double* _sx, double* _sy) const
{
	double x1 = 0.0;
	double y1 = 0.0;
	double x2 = 1.0;
	double y2 = 1.0;
	BAffineTransform t(*this);
	t.PreMultiply(AffineRotation(-Rotation()));
	t.Apply(&x1, &y1);
	t.Apply(&x2, &y2);
	if (_sx)
		*_sx = x2 - x1;
	if (_sy)
		*_sy = y2 - y1;
}


void
BAffineTransform::GetScaleAbs(double* _sx, double* _sy) const
{
	// When there is considerable shear this method gives us much
	// better estimation than just sx, sy.
	if (_sx)
		*_sx = sqrt(sx * sx + shx * shx);
	if (_sy)
		*_sy = sqrt(shy * shy + sy * sy);
}


bool
BAffineTransform::GetAffineParameters(double* _translationX,
	double* _translationY, double* _rotation, double* _scaleX, double* _scaleY,
	double* _shearX, double* _shearY) const
{
	GetTranslation(_translationX, _translationY);

	double rotation = Rotation();
	if (_rotation != NULL)
		*_rotation = rotation;

	// Calculate shear
	double x1 = 0.0;
	double y1 = 0.0;
	double x2 = 1.0;
	double y2 = 0.0;
	double x3 = 0.0;
	double y3 = 1.0;

	// Reverse the effects of any rotation
	BAffineTransform t(*this);
	t.PreMultiply(AffineRotation(-rotation));

	t.Apply(&x1, &y1);
	t.Apply(&x2, &y2);
	t.Apply(&x3, &y3);

	double shearX = y2 - y1;
	double shearY = x3 - x1;

	// Calculate scale
	x1 = 0.0;
	y1 = 0.0;
	x2 = 1.0;
	y2 = 0.0;
	x3 = 0.0;
	y3 = 1.0;

	// Reverse the effects of any shear
	t.PreMultiplyInverse(AffineShearing(shearX, shearY));

	t.Apply(&x1, &y1);
	t.Apply(&x2, &y2);
	t.Apply(&x3, &y3);

	double scaleX = x2 - x1;
	double scaleY = y3 - y1;

	if (_scaleX != NULL)
		*_scaleX = scaleX;
	if (_scaleY != NULL)
		*_scaleY = scaleY;

	// Since scale was calculated last, the shear values are still scaled.
	// We cannot get the affine parameters from a matrix with 0 scale.
	if (scaleX == 0.0 && scaleY == 0.0)
		return false;

	if (_shearX != NULL)
		*_shearX = shearX / scaleX;
	if (_shearY != NULL)
		*_shearY = shearY / scaleY;

	return true;
}

