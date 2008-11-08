/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include <Point.h>
#include <Gradient.h>
#include <GradientRadial.h>


// constructor
BGradientRadial::BGradientRadial()
{
	fData.radial.cx = 0.0f;
	fData.radial.cy = 0.0f;
	fData.radial.radius = 0.0f;
	fType = TYPE_RADIAL;
}


// constructor
BGradientRadial::BGradientRadial(const BPoint& center, float radius)
{
	fData.radial.cx = center.x;
	fData.radial.cy = center.y;
	fData.radial.radius = radius;
	fType = TYPE_RADIAL;
}


// constructor
BGradientRadial::BGradientRadial(float cx, float cy, float radius)
{
	fData.radial.cx = cx;
	fData.radial.cy = cy;
	fData.radial.radius = radius;
	fType = TYPE_RADIAL;
}


// Center
BPoint
BGradientRadial::Center() const
{
	return BPoint(fData.radial.cx, fData.radial.cy);
}


// SetCenter
void
BGradientRadial::SetCenter(const BPoint& center)
{
	fData.radial.cx = center.x;
	fData.radial.cy = center.y;
}


// SetCenter
void
BGradientRadial::SetCenter(float cx, float cy)
{
	fData.radial.cx = cx;
	fData.radial.cy = cy;
}


// Radius
float
BGradientRadial::Radius() const
{
	return fData.radial.radius;
}


// SetRadius
void
BGradientRadial::SetRadius(float radius)
{
	fData.radial.radius = radius;
}
