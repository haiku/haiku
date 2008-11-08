/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef _GRADIENT_RADIAL_FOCUS_H
#define _GRADIENT_RADIAL_FOCUS_H

#include <Gradient.h>

class BPoint;

class BGradientRadialFocus : public BGradient {
public:
	BGradientRadialFocus();
	BGradientRadialFocus(const BPoint& center, float radius,
		const BPoint& focal);
	BGradientRadialFocus(float cx, float cy, float radius, float fx, float fy);
	
	BPoint Center() const;
	void SetCenter(const BPoint& center);
	void SetCenter(float cx, float cy);
	
	BPoint Focal() const;
	void SetFocal(const BPoint& focal);
	void SetFocal(float fx, float fy);
	
	float Radius() const;
	void SetRadius(float radius);
};

#endif // _GRADIENT_RADIAL_FOCUS_H
