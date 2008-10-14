/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef GRADIENT_LINEAR_H
#define GRADIENT_LINEAR_H

#include <Gradient.h>

class BPoint;

class BGradientLinear : public BGradient {
public:
	BGradientLinear();
	BGradientLinear(const BPoint& start, const BPoint& end);
	BGradientLinear(float x1, float y1, float x2, float y2);
	
	BPoint Start() const;
	void SetStart(const BPoint& start);
	void SetStart(float x1, float y1);
	
	BPoint End() const;
	void SetEnd(const BPoint& end);
	void SetEnd(float x2, float y2);
};

#endif // GRADIENT_LINEAR_H
