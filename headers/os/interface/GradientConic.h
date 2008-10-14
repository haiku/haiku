/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef GRADIENT_CONIC_H
#define GRADIENT_CONIC_H

#include <Gradient.h>

class BPoint;

class BGradientConic : public BGradient {
public:
	BGradientConic();
	BGradientConic(const BPoint& center, float angle);
	BGradientConic(float cx, float cy, float angle);
	
	BPoint Center() const;
	void SetCenter(const BPoint& center);
	void SetCenter(float cx, float cy);

	float Angle() const;
	void SetAngle(float angle);
};

#endif // GRADIENT_CONIC_H
