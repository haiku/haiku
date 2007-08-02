/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * A handy front-end to agg::trans_affine transformation matrix.
 *
 */

#include <stdio.h>
#include <string.h>

#include <Message.h>

#include "Transformable.h"

// min4
inline float
min4(float a, float b, float c, float d)
{
	return min_c(a, min_c(b, min_c(c, d)));
} 

// max4
inline float
max4(float a, float b, float c, float d)
{
	return max_c(a, max_c(b, max_c(c, d)));
} 

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

// constructor
Transformable::Transformable(const BMessage* archive)
	: agg::trans_affine()
{
	if (archive) {
		double storage[6];
		status_t ret = B_OK;
		for (int32 i = 0; i < 6; i++) {
			ret = archive->FindDouble("affine matrix", i, &storage[i]);
			if (ret < B_OK)
				break;
		}
		if (ret >= B_OK)
			load_from(storage);
	}
}

// destructor
Transformable::~Transformable()
{
}

// Archive
status_t
Transformable::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);
	if (ret >= B_OK) {
		double storage[6];
		store_to(storage);
		for (int32 i = 0; i < 6; i++) {
			ret = into->AddDouble("affine matrix", storage[i]);
			if (ret < B_OK)
				break;
		}
		// finish off
		if (ret >= B_OK)
			ret = into->AddString("class", "Transformable");
	}
	return ret;
}

// StoreTo
void
Transformable::StoreTo(double matrix[6]) const
{
	store_to(matrix);
}

// LoadFrom
void
Transformable::LoadFrom(double matrix[6])
{
	// before calling the potentially heavy TransformationChanged()
	// hook function, we make sure that it is actually true
	Transformable t;
	t.load_from(matrix);
	if (*this != t) {
		load_from(matrix);
		TransformationChanged();
	}
}

// SetTransformable
void
Transformable::SetTransformable(const Transformable& other)
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
	reset();
}

// IsIdentity
bool
Transformable::IsIdentity() const
{
	double m[6];
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

// operator==
bool
Transformable::operator==(const Transformable& other) const
{
	double m1[6];
	other.store_to(m1);
	double m2[6];
	store_to(m2);
	if (m1[0] == m2[0] &&
		m1[1] == m2[1] &&
		m1[2] == m2[2] &&
		m1[3] == m2[3] &&
		m1[4] == m2[4] &&
		m1[5] == m2[5])
		return true;
	return false;
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

// TransformBounds
BRect
Transformable::TransformBounds(const BRect& bounds) const
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
Transformable::RotateBy(BPoint origin, double radians)
{
	if (radians != 0.0) {
		multiply(agg::trans_affine_translation(-origin.x, -origin.y));
		multiply(agg::trans_affine_rotation(radians));
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
