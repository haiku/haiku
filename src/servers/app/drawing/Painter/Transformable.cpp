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


Transformable::Transformable()
	: agg::trans_affine()
{
}


Transformable::Transformable(const Transformable& other)
	: agg::trans_affine(other)
{
}


Transformable::Transformable(const BMessage* archive)
	: agg::trans_affine()
{
	if (archive != NULL) {
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


Transformable::~Transformable()
{
}


status_t
Transformable::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);
	if (ret == B_OK) {
		double storage[6];
		store_to(storage);
		for (int32 i = 0; i < 6; i++) {
			ret = into->AddDouble("affine matrix", storage[i]);
			if (ret < B_OK)
				break;
		}
		// finish off
		if (ret == B_OK)
			ret = into->AddString("class", "Transformable");
	}
	return ret;
}


void
Transformable::StoreTo(double matrix[6]) const
{
	store_to(matrix);
}


void
Transformable::LoadFrom(double matrix[6])
{
	// Before calling the potentially heavy TransformationChanged()
	// hook function, we make sure that it is actually true
	Transformable t;
	t.load_from(matrix);
	if (*this != t) {
		load_from(matrix);
		TransformationChanged();
	}
}


void
Transformable::SetTransformable(const Transformable& other)
{
	if (*this != other) {
		*this = other;
		TransformationChanged();
	}
}


Transformable&
Transformable::operator=(const Transformable& other)
{
	if (other != *this) {
		agg::trans_affine::operator=(other);
		TransformationChanged();
	}
	return *this;
}


Transformable&
Transformable::operator=(const agg::trans_affine& other)
{
	if (other != *this) {
		agg::trans_affine::operator=(other);
		TransformationChanged();
	}
	return *this;
}


Transformable&
Transformable::Multiply(const Transformable& other)
{
	if (!other.IsIdentity()) {
		multiply(other);
		TransformationChanged();
	}
	return *this;
}


void
Transformable::Reset()
{
	reset();
}


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


bool
Transformable::IsDilation() const
{
	double m[6];
	store_to(m);
	return m[1] == 0.0 && m[2] == 0.0;
}


void
Transformable::Transform(double* x, double* y) const
{
	transform(x, y);
}


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


BPoint
Transformable::Transform(const BPoint& point) const
{
	BPoint p(point);
	Transform(&p);
	return p;
}


void
Transformable::InverseTransform(double* x, double* y) const
{
	inverse_transform(x, y);
}


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


BPoint
Transformable::InverseTransform(const BPoint& point) const
{
	BPoint p(point);
	InverseTransform(&p);
	return p;
}


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


bool
Transformable::IsTranslationOnly() const
{
	double matrix[6];
	store_to(matrix);
	return matrix[0] == 1.0 && matrix[1] == 0.0
		&& matrix[2] == 0.0 && matrix[3] == 1.0;
}



void
Transformable::TranslateBy(BPoint offset)
{
	if (offset.x != 0.0 || offset.y != 0.0) {
		multiply(agg::trans_affine_translation(offset.x, offset.y));
		TransformationChanged();
	}
}


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
