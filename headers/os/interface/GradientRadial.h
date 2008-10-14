/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef GRADIENT_RADIAL_H
#define GRADIENT_RADIAL_H

#include <Gradient.h>

class BPoint;

class BGradientRadial : public BGradient {
public:
	BGradientRadial();
	BGradientRadial(const BPoint& center, float radius);
	BGradientRadial(float cx, float cy, float radius);
	
	BPoint Center() const;
	void SetCenter(const BPoint& center);
	void SetCenter(float cx, float cy);
	
	float Radius() const;
	void SetRadius(float radius);
};

#endif // GRADIENT_RADIAL_H
