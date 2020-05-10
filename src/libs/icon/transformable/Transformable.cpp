/*
 * Copyright 2006-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Transformable.h"

#include <stdio.h>
#include <string.h>

// constructor
Transformable::Transformable()
	: agg::trans_affine()
{
}

// copy constructor
Transformable::Transformable(const Transformable& other)
	: agg::trans_affine(other)
{
}

// destructor
Transformable::~Transformable()
{
}

// StoreTo
void
Transformable::StoreTo(double matrix[matrix_size]) const
{
	store_to(matrix);
}

// LoadFrom
void
Transformable::LoadFrom(const double matrix[matrix_size])
{
	// before calling the potentially heavy TransformationChanged()
	// hook function, make sure that the transformation
	// really changed
	Transformable t;
	t.load_from(matrix);
	if (*this != t) {
		load_from(matrix);
		TransformationChanged();
	}
}

// SetTransform
void
Transformable::SetTransform(const Transformable& other)
{
	if (*this != other) {
		*this = other;
		TransformationChanged();
	}
}

// operator=
Transformable&
Transformable::operator=(const Transformable& other)
{
	if (other != *this) {
		reset();
		multiply(other);
		TransformationChanged();
	}
	return *this;
}

// Multiply
Transformable&
Transformable::Multiply(const Transformable& other)
{
	if (!other.IsIdentity()) {
		multiply(other);
		TransformationChanged();
	}
	return *this;
}

// Reset
void
Transformable::Reset()
{
	if (!IsIdentity()) {
		reset();
		TransformationChanged();
	}
}

// Invert
void
Transformable::Invert()
{
	if (!IsIdentity()) {
		invert();
		TransformationChanged();
	}
}

// IsIdentity
bool
Transformable::IsIdentity() const
{
	double m[matrix_size];
	store_to(m);
	if (m[0] == 1.0 &&
		m[1] == 0.0 &&
		m[2] == 0.0 &&
		m[3] == 1.0 &&
		m[4] == 0.0 &&
		m[5] == 0.0)
		return true;
	return false;
}

// IsTranslationOnly
bool
Transformable::IsTranslationOnly() const
{
	double m[matrix_size];
	store_to(m);
	if (m[0] == 1.0 &&
		m[1] == 0.0 &&
		m[2] == 0.0 &&
		m[3] == 1.0)
		return true;
	return false;
}

// IsNotDistorted
bool
Transformable::IsNotDistorted() const
{
	double m[matrix_size];
	store_to(m);
	return (m[0] == m[3]);
}

// IsValid
bool
Transformable::IsValid() const
{
	double m[matrix_size];
	store_to(m);
	return ((m[0] * m[3] - m[1] * m[2]) != 0.0);
}

// operator==
bool
Transformable::operator==(const Transformable& other) const
{
	double m1[matrix_size];
	other.store_to(m1);
	double m2[matrix_size];
	store_to(m2);
	return memcmp(m1, m2, sizeof(m1)) == 0;
}

// operator!=
bool
Transformable::operator!=(const Transformable& other) const
{
	return !(*this == other);
}

// Transform
void
Transformable::Transform(double* x, double* y) const
{
	transform(x, y);
}

// Transform
void
Transformable::Transform(BPoint* point) const
{
	if (point) {
		double x = point->x;
		double y = point->y;

		transform(&x, &y);

		point->x = x;
		point->y = y;
	}
}

// Transform
BPoint
Transformable::Transform(const BPoint& point) const
{
	BPoint p(point);
	Transform(&p);
	return p;
}

// InverseTransform
void
Transformable::InverseTransform(double* x, double* y) const
{
	inverse_transform(x, y);
}

// InverseTransform
void
Transformable::InverseTransform(BPoint* point) const
{
	if (point) {
		double x = point->x;
		double y = point->y;

		inverse_transform(&x, &y);

		point->x = x;
		point->y = y;
	}
}

// InverseTransform
BPoint
Transformable::InverseTransform(const BPoint& point) const
{
	BPoint p(point);
	InverseTransform(&p);
	return p;
}

inline float
min4(float a, float b, float c, float d)
{
	return min_c(a, min_c(b, min_c(c, d)));
}

inline float
max4(float a, float b, float c, float d)
{
	return max_c(a, max_c(b, max_c(c, d)));
}

// TransformBounds
BRect
Transformable::TransformBounds(BRect bounds) const
{
	if (bounds.IsValid()) {
		BPoint lt(bounds.left, bounds.top);
		BPoint rt(bounds.right, bounds.top);
		BPoint lb(bounds.left, bounds.bottom);
		BPoint rb(bounds.right, bounds.bottom);

		Transform(&lt);
		Transform(&rt);
		Transform(&lb);
		Transform(&rb);

		return BRect(floorf(min4(lt.x, rt.x, lb.x, rb.x)),
					 floorf(min4(lt.y, rt.y, lb.y, rb.y)),
					 ceilf(max4(lt.x, rt.x, lb.x, rb.x)),
					 ceilf(max4(lt.y, rt.y, lb.y, rb.y)));
	}
	return bounds;
}

// TranslateBy
void
Transformable::TranslateBy(BPoint offset)
{
	if (offset.x != 0.0 || offset.y != 0.0) {
		multiply(agg::trans_affine_translation(offset.x, offset.y));
		TransformationChanged();
	}
}

// RotateBy
void
Transformable::RotateBy(BPoint origin, double degrees)
{
	if (degrees != 0.0) {
		multiply(agg::trans_affine_translation(-origin.x, -origin.y));
		multiply(agg::trans_affine_rotation(degrees * (M_PI / 180.0)));
		multiply(agg::trans_affine_translation(origin.x, origin.y));
		TransformationChanged();
	}
}

// ScaleBy
void
Transformable::ScaleBy(BPoint origin, double xScale, double yScale)
{
	if (xScale != 1.0 || yScale != 1.0) {
		multiply(agg::trans_affine_translation(-origin.x, -origin.y));
		multiply(agg::trans_affine_scaling(xScale, yScale));
		multiply(agg::trans_affine_translation(origin.x, origin.y));
		TransformationChanged();
	}
}

// ShearBy
void
Transformable::ShearBy(BPoint origin, double xShear, double yShear)
{
	if (xShear != 0.0 || yShear != 0.0) {
		multiply(agg::trans_affine_translation(-origin.x, -origin.y));
		multiply(agg::trans_affine_skewing(xShear, yShear));
		multiply(agg::trans_affine_translation(origin.x, origin.y));
		TransformationChanged();
	}
}

// TransformationChanged
void
Transformable::TransformationChanged()
{
	// default implementation doesn't care
}


void
Transformable::PrintToStream() const
{
	double matrix[6];
	store_to(matrix);
	printf("Transform:\n%f\t%f\t%f\n%f\t%f\t%f\n",
		matrix[0], matrix[2], matrix[4], matrix[1], matrix[3], matrix[5]);
}
