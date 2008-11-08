/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include <Point.h>
#include <Gradient.h>
#include <GradientRadialFocus.h>


// constructor
BGradientRadialFocus::BGradientRadialFocus()
{
	fData.radial_focus.cx = 0.0f;
	fData.radial_focus.cy = 0.0f;
	fData.radial_focus.fx = 0.0f;
	fData.radial_focus.fy = 0.0f;
	fData.radial_focus.radius = 0.0f;
	fType = TYPE_RADIAL_FOCUS;
}


// constructor
BGradientRadialFocus::BGradientRadialFocus(const BPoint& center, float radius,
	const BPoint& focal)
{
	fData.radial_focus.cx = center.x;
	fData.radial_focus.cy = center.y;
	fData.radial_focus.fx = focal.x;
	fData.radial_focus.fy = focal.y;
	fData.radial_focus.radius = radius;
	fType = TYPE_RADIAL_FOCUS;
}


// constructor
BGradientRadialFocus::BGradientRadialFocus(float cx, float cy, float radius,
	float fx, float fy)
{
	fData.radial_focus.cx = cx;
	fData.radial_focus.cy = cy;
	fData.radial_focus.fx = fx;
	fData.radial_focus.fy = fy;
	fData.radial_focus.radius = radius;
	fType = TYPE_RADIAL_FOCUS;
}


// Center
BPoint
BGradientRadialFocus::Center() const
{
	return BPoint(fData.radial_focus.cx, fData.radial_focus.cy);
}


// SetCenter
void
BGradientRadialFocus::SetCenter(const BPoint& center)
{
	fData.radial_focus.cx = center.x;
	fData.radial_focus.cy = center.y;
}


// SetCenter
void
BGradientRadialFocus::SetCenter(float cx, float cy)
{
	fData.radial_focus.cx = cx;
	fData.radial_focus.cy = cy;
}


// Focal
BPoint
BGradientRadialFocus::Focal() const
{
	return BPoint(fData.radial_focus.fx, fData.radial_focus.fy);
}


// SetFocal
void
BGradientRadialFocus::SetFocal(const BPoint& focal)
{
	fData.radial_focus.fx = focal.x;
	fData.radial_focus.fy = focal.y;
}


// SetFocal
void
BGradientRadialFocus::SetFocal(float fx, float fy)
{
	fData.radial_focus.fx = fx;
	fData.radial_focus.fy = fy;
}


// Radius
float
BGradientRadialFocus::Radius() const
{
	return fData.radial_focus.radius;
}


// SetRadius
void
BGradientRadialFocus::SetRadius(float radius)
{
	fData.radial_focus.radius = radius;
}
