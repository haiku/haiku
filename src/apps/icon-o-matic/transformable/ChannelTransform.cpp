/*
 * Copyright 2006-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ChannelTransform.h"

#include <math.h>
#include <stdio.h>


// constructor
ChannelTransform::ChannelTransform()
	:
	Transformable(),
	fPivot(0.0, 0.0),
	fTranslation(0.0, 0.0),
	fRotation(0.0),
	fXScale(1.0),
	fYScale(1.0)
{
}

// copy constructor
ChannelTransform::ChannelTransform(const ChannelTransform& other)
	:
	Transformable(other),
	fPivot(other.fPivot),
	fTranslation(other.fTranslation),
	fRotation(other.fRotation),
	fXScale(other.fXScale),
	fYScale(other.fYScale)
{
}


// destructor
ChannelTransform::~ChannelTransform()
{
}


// SetTransformation
void
ChannelTransform::SetTransformation(const Transformable& other)
{
	// calc affine parameters

	// translation
	double tx;
	double ty;
	other.translation(&tx, &ty);

	// rotation
	double rotation = agg::rad2deg(other.rotation());

	// scale
	double scaleX;
	double scaleY;
	other.scaling(&scaleX, &scaleY);

	if (isnanf(tx) || isnanf(ty) || isnanf(scaleX) || isnanf(scaleY))
		return;

	SetTransformation(B_ORIGIN, BPoint(tx, ty), rotation, scaleX, scaleY);
}


// SetTransformation
void
ChannelTransform::SetTransformation(BPoint pivot, BPoint translation,
	double rotation, double xScale, double yScale)
{
//printf("SetTransformation(BPoint(%.1f, %.1f), BPoint(%.1f, %.1f), "
//"%.2f, %.2f, %.2f)\n", pivot.x, pivot.y, translation.x, translation.y,
//rotation, xScale, yScale);

	if (fTranslation != translation ||
		fPivot != pivot ||
		fRotation != rotation ||
		fXScale != xScale ||
		fYScale != yScale) {

		fPivot = pivot;
		fTranslation = translation;
		fRotation = rotation;
		fXScale = xScale;
		fYScale = yScale;

		_UpdateMatrix();
	}
}


// SetPivot
void
ChannelTransform::SetPivot(BPoint pivot)
{
	if (pivot == fPivot)
		return;

	fPivot = pivot;

	_UpdateMatrix();
}


// TranslateBy
void
ChannelTransform::TranslateBy(BPoint offset)
{
	if (offset.x == 0.0 && offset.y == 0.0)
		return;

	fTranslation += offset;

	_UpdateMatrix();
}


// RotateBy
/*!	Converts a rotation in world coordinates into
	a combined local rotation and a translation.
*/
void
ChannelTransform::RotateBy(BPoint origin, double degrees)
{
	if (degrees == 0.0)
		return;

	origin -= fPivot;

	fRotation += degrees;

	// rotate fTranslation
	double xOffset = fTranslation.x - origin.x;
	double yOffset = fTranslation.y - origin.y;

	agg::trans_affine_rotation m(degrees * M_PI / 180.0);
	m.transform(&xOffset, &yOffset);

	fTranslation.x = origin.x + xOffset;
	fTranslation.y = origin.y + yOffset;

	_UpdateMatrix();
}


// RotateBy
void
ChannelTransform::RotateBy(double degrees)
{
	if (degrees == 0.0)
		return;

	fRotation += degrees;

	_UpdateMatrix();
}


// ScaleBy
//
// converts a scalation in world coordinates into
// a combined local scalation and a translation
void
ChannelTransform::ScaleBy(BPoint origin, double xScale, double yScale)
{
	// TODO: Untested?
	if (xScale == 1.0 && yScale == 1.0)
		return;

	fXScale *= xScale;
	fYScale *= yScale;

	// scale fTranslation
	double xOffset = fTranslation.x - origin.x;
	double yOffset = fTranslation.y - origin.y;

	fTranslation.x = origin.x + (xOffset * xScale);
	fTranslation.y = origin.y + (yOffset * yScale);

	_UpdateMatrix();
}

// ScaleBy
void
ChannelTransform::ScaleBy(double xScale, double yScale)
{
	if (xScale == 1.0 && yScale == 1.0)
		return;

	fXScale *= xScale;
	fYScale *= yScale;

	_UpdateMatrix();
}


// SetTranslationAndScale
void
ChannelTransform::SetTranslationAndScale(BPoint offset, double xScale,
	double yScale)
{
	if (fTranslation == offset && fXScale == xScale && fYScale == yScale)
		return;

	fTranslation = offset;

	fXScale = xScale;
	fYScale = yScale;

	_UpdateMatrix();
}


// Reset
void
ChannelTransform::Reset()
{
	SetTransformation(B_ORIGIN, B_ORIGIN, 0.0, 1.0, 1.0);
}


// =
ChannelTransform&
ChannelTransform::operator=(const ChannelTransform& other)
{
	fTranslation = other.fTranslation;
	fRotation = other.fRotation;
	fXScale = other.fXScale;
	fYScale = other.fYScale;

	Transformable::operator=(other);

	return *this;
}


// _UpdateMatrix
void
ChannelTransform::_UpdateMatrix()
{
	// fix up scales in case any is zero
	double xScale = fXScale;
	if (xScale == 0.0)
		xScale = 0.000001;
	double yScale = fYScale;
	if (yScale == 0.0)
		yScale = 0.000001;

	// start clean
	reset();
	// the "pivot" is like the offset from world to local
	// coordinate system and is the center for rotation and scale
	multiply(agg::trans_affine_translation(-fPivot.x, -fPivot.y));
	multiply(agg::trans_affine_scaling(xScale, yScale));
	multiply(agg::trans_affine_rotation(fRotation * M_PI / 180.0));

	multiply(agg::trans_affine_translation(fPivot.x + fTranslation.x,
		fPivot.y + fTranslation.y));

	// call hook function
	Update();
}

