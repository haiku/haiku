/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include <Point.h>
#include <Gradient.h>
#include <GradientDiamond.h>


// constructor
BGradientDiamond::BGradientDiamond()
{
	fData.diamond.cx = 0.0f;
	fData.diamond.cy = 0.0f;
	fType = TYPE_DIAMOND;
}


// constructor
BGradientDiamond::BGradientDiamond(const BPoint& center)
{
	fData.diamond.cx = center.x;
	fData.diamond.cy = center.y;
	fType = TYPE_DIAMOND;
}


// constructor
BGradientDiamond::BGradientDiamond(float cx, float cy)
{
	fData.diamond.cx = cx;
	fData.diamond.cy = cy;
	fType = TYPE_DIAMOND;
}


// Center
BPoint
BGradientDiamond::Center() const
{
	return BPoint(fData.diamond.cx, fData.diamond.cy);
}


// SetCenter
void
BGradientDiamond::SetCenter(const BPoint& center)
{
	fData.diamond.cx = center.x;
	fData.diamond.cy = center.y;
}


// SetCenter
void
BGradientDiamond::SetCenter(float cx, float cy)
{
	fData.diamond.cx = cx;
	fData.diamond.cy = cy;
}
