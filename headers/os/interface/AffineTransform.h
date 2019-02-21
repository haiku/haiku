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
#ifndef _AFFINE_TRANSFORM_H
#define _AFFINE_TRANSFORM_H


#include <Flattenable.h>
#include <Point.h>

#include <math.h>


class BAffineTransform : public BFlattenable {
public:

#if __cplusplus < 201103L
	static	const double			kDefaultEpsilon = 1e-14;
#else
	static	constexpr double		kDefaultEpsilon = 1e-14;
#endif

public:
									BAffineTransform();
									BAffineTransform(double sx, double shy,
										double shx, double sy, double tx,
										double ty);
									BAffineTransform(
										const BAffineTransform& copyFrom);
	virtual							~BAffineTransform();

	// BFlattenable interface
	virtual	bool					IsFixedSize() const;
	virtual	type_code				TypeCode() const;
	virtual	ssize_t					FlattenedSize() const;
	virtual	status_t				Flatten(void* buffer,
										ssize_t size) const;
	virtual	status_t				Unflatten(type_code code,
										const void* buffer, ssize_t size);

	// Construction
	static	BAffineTransform		AffineTranslation(double x, double y);
	static	BAffineTransform		AffineRotation(double angle);
	static	BAffineTransform		AffineScaling(double x, double y);
	static	BAffineTransform		AffineScaling(double scale);
	static	BAffineTransform		AffineShearing(double x, double y);

	// Application
	inline	void					Apply(double* x, double* y) const;
	inline	void					ApplyInverse(double* x, double* y) const;

			BPoint					Apply(const BPoint& point) const;
			BPoint					ApplyInverse(const BPoint& point) const;

			void					Apply(BPoint* point) const;
			void					ApplyInverse(BPoint* point) const;

			void					Apply(BPoint* points, uint32 count) const;
			void					ApplyInverse(BPoint* points,
										uint32 count) const;

	// Translation
	inline	const BAffineTransform&	TranslateBy(double x, double y);
			const BAffineTransform&	TranslateBy(const BPoint& delta);

	inline	const BAffineTransform&	PreTranslateBy(double x, double y);

			BAffineTransform		TranslateByCopy(double x, double y) const;
			BAffineTransform		TranslateByCopy(const BPoint& delta) const;

//			const BAffineTransform&	SetTranslation(double x, double y);

	// Rotation
	inline	const BAffineTransform&	RotateBy(double angle);
			const BAffineTransform&	RotateBy(const BPoint& center,
										double angle);

	inline	const BAffineTransform&	PreRotateBy(double angleRadians);

			BAffineTransform		RotateByCopy(double angle) const;
			BAffineTransform		RotateByCopy(const BPoint& center,
										double angle) const;

//			const BAffineTransform&	SetRotation(double angle);

	// Scaling
	inline	const BAffineTransform&	ScaleBy(double scale);
			const BAffineTransform&	ScaleBy(const BPoint& center,
										double scale);
	inline	const BAffineTransform&	ScaleBy(double x, double y);
			const BAffineTransform&	ScaleBy(const BPoint& center, double x,
										double y);
			const BAffineTransform&	ScaleBy(const BPoint& scale);
			const BAffineTransform&	ScaleBy(const BPoint& center,
										const BPoint& scale);

	inline	const BAffineTransform&	PreScaleBy(double x, double y);

			BAffineTransform		ScaleByCopy(double scale) const;
			BAffineTransform		ScaleByCopy(const BPoint& center,
										double scale) const;
			BAffineTransform		ScaleByCopy(double x, double y) const;
			BAffineTransform		ScaleByCopy(const BPoint& center,
										double x, double y) const;
			BAffineTransform		ScaleByCopy(const BPoint& scale) const;
			BAffineTransform		ScaleByCopy(const BPoint& center,
										const BPoint& scale) const;

			const BAffineTransform&	SetScale(double scale);
			const BAffineTransform&	SetScale(double x, double y);

	// Shearing
	inline	const BAffineTransform&	ShearBy(double x, double y);
			const BAffineTransform&	ShearBy(const BPoint& center, double x,
										double y);
			const BAffineTransform&	ShearBy(const BPoint& shear);
			const BAffineTransform&	ShearBy(const BPoint& center,
										const BPoint& shear);

			BAffineTransform		ShearByCopy(double x, double y) const;
			BAffineTransform		ShearByCopy(const BPoint& center,
										double x, double y) const;
			BAffineTransform		ShearByCopy(const BPoint& shear) const;
			BAffineTransform		ShearByCopy(const BPoint& center,
										const BPoint& shear) const;

//			const BAffineTransform&	SetShear(double x, double y);

	// Multiplication
	inline	const BAffineTransform&	Multiply(const BAffineTransform& other);
			const BAffineTransform&	PreMultiply(const BAffineTransform& other);
	inline	const BAffineTransform&	MultiplyInverse(
										const BAffineTransform& other);
	inline	const BAffineTransform&	PreMultiplyInverse(
										const BAffineTransform& other);

	// Operators
	inline	BAffineTransform&		operator=(
										const BAffineTransform& copyFrom);

	inline	bool					operator==(
										const BAffineTransform& other) const;
	inline	bool					operator!=(
										const BAffineTransform& other) const;

	inline	const BAffineTransform&	operator*=(const BAffineTransform& other);
	inline	const BAffineTransform&	operator/=(const BAffineTransform& other);

	inline	BAffineTransform		operator*(
										const BAffineTransform& other) const;
	inline	BAffineTransform		operator/(
										const BAffineTransform& other) const;

	inline	BAffineTransform		operator~() const;

	// Utility
			bool					IsValid(double epsilon
										= kDefaultEpsilon) const;
			bool					IsIdentity(double epsilon
										= kDefaultEpsilon) const;
			bool					IsDilation(double epsilon
										= kDefaultEpsilon) const;
			bool					IsEqual(const BAffineTransform& other,
										double epsilon
											= kDefaultEpsilon) const;

			const BAffineTransform&	Invert();
			const BAffineTransform&	FlipX();
			const BAffineTransform&	FlipY();
			const BAffineTransform&	Reset();

	inline	double					Determinant() const;
	inline	double					InverseDeterminant() const;
			void					GetTranslation(double* tx,
										double* ty) const;
			double					Rotation() const;
			double					Scale() const;
			void					GetScale(double* sx, double* sy) const;
			void					GetScaleAbs(double* sx,
										double* sy) const;
			bool					GetAffineParameters(double* translationX,
										double* translationY, double* rotation,
										double* scaleX, double* scaleY,
										double* shearX, double* shearY) const;

public:
			double					sx;
			double					shy;
			double					shx;
			double					sy;
			double					tx;
			double					ty;
};


extern const BAffineTransform B_AFFINE_IDENTITY_TRANSFORM;


// #pragma mark - inline methods


inline void
BAffineTransform::Apply(double* x, double* y) const
{
	double tmp = *x;
	*x = tmp * sx + *y * shx + tx;
	*y = tmp * shy + *y * sy + ty;
}


inline void
BAffineTransform::ApplyInverse(double* x, double* y) const
{
	double d = InverseDeterminant();
	double a = (*x - tx) * d;
	double b = (*y - ty) * d;
	*x = a * sy - b * shx;
	*y = b * sx - a * shy;
}


// #pragma mark -


inline const BAffineTransform&
BAffineTransform::TranslateBy(double x, double y)
{
	tx += x;
	ty += y;
	return *this;
}


inline const BAffineTransform&
BAffineTransform::PreTranslateBy(double x, double y)
{
	tx += x * sx + y * shx;
	ty += x * shy + y * sy;
	return *this;
}


inline const BAffineTransform&
BAffineTransform::RotateBy(double angle)
{
	double ca = cos(angle);
	double sa = sin(angle);
	double t0 = sx * ca - shy * sa;
	double t2 = shx * ca - sy * sa;
	double t4 = tx * ca - ty * sa;
	shy = sx * sa + shy * ca;
	sy = shx * sa + sy * ca;
	ty = tx * sa + ty * ca;
	sx = t0;
	shx = t2;
	tx = t4;
	return *this;
}


inline const BAffineTransform&
BAffineTransform::PreRotateBy(double angle)
{
	double ca = cos(angle);
	double sa = sin(angle);
	double newSx = sx * ca + shx * sa;
	double newSy = -shy * sa + sy * ca;
	shy = shy * ca + sy * sa;
	shx = -sx * sa + shx * ca;
	sx = newSx;
	sy = newSy;
	return *this;
}


inline const BAffineTransform&
BAffineTransform::ScaleBy(double x, double y)
{
	double mm0 = x;
		// Possible hint for the optimizer
	double mm3 = y;
	sx *= mm0;
	shx *= mm0;
	tx *= mm0;
	shy *= mm3;
	sy *= mm3;
	ty *= mm3;
	return *this;
}


inline const BAffineTransform&
BAffineTransform::ScaleBy(double s)
{
	double m = s;
		// Possible hint for the optimizer
	sx *= m;
	shx *= m;
	tx *= m;
	shy *= m;
	sy *= m;
	ty *= m;
	return *this;
}


inline const BAffineTransform&
BAffineTransform::PreScaleBy(double x, double y)
{
	sx *= x;
	shx *= y;
	shy *= x;
	sy *= y;
	return *this;
}


inline const BAffineTransform&
BAffineTransform::ShearBy(double x, double y)
{
	BAffineTransform shearTransform = AffineShearing(x, y);
	return PreMultiply(shearTransform);
}


// #pragma mark -


inline const BAffineTransform&
BAffineTransform::Multiply(const BAffineTransform& other)
{
	BAffineTransform t(other);
	return *this = t.PreMultiply(*this);
}


inline const BAffineTransform&
BAffineTransform::MultiplyInverse(const BAffineTransform& other)
{
	BAffineTransform t(other);
	t.Invert();
	return Multiply(t);
}


inline const BAffineTransform&
BAffineTransform::PreMultiplyInverse(const BAffineTransform& other)
{
	BAffineTransform t(other);
	t.Invert();
	return *this = t.Multiply(*this);
}


// #pragma mark -


inline BAffineTransform&
BAffineTransform::operator=(const BAffineTransform& other)
{
	sx = other.sx;
	shy = other.shy;
	shx = other.shx;
	sy = other.sy;
	tx = other.tx;
	ty = other.ty;
	return *this;
}

inline bool
BAffineTransform::operator==(const BAffineTransform& other) const
{
	return IsEqual(other);
}

inline bool
BAffineTransform::operator!=(const BAffineTransform& other) const
{
	return !IsEqual(other);
}


inline const BAffineTransform&
BAffineTransform::operator*=(const BAffineTransform& other)
{
	return Multiply(other);
}


inline const BAffineTransform&
BAffineTransform::operator/=(const BAffineTransform& other)
{
	return MultiplyInverse(other);
}


inline BAffineTransform
BAffineTransform::operator*(const BAffineTransform& other) const
{
	return BAffineTransform(*this).Multiply(other);
}


inline BAffineTransform
BAffineTransform::operator/(const BAffineTransform& other) const
{
	return BAffineTransform(*this).MultiplyInverse(other);
}


inline BAffineTransform
BAffineTransform::operator~() const
{
	BAffineTransform result(*this);
	return result.Invert();
}


// #pragma mark -


inline double
BAffineTransform::Determinant() const
{
	return sx * sy - shy * shx;
}


inline double
BAffineTransform::InverseDeterminant() const
{
	return 1.0 / (sx * sy - shy * shx);
}


#endif // _AFFINE_TRANSFORM_H
