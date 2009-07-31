/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

// Some math utils, useful for animation


#include "MathUtils.h"


float
MathUtils::EaseInOutCubic(float t /*time*/, float b /*begin*/, float c /*distance*/, float d /*duration*/)
{
	t /= d / 2.0;
	if (t < 1.0)
		return c / 2.0 * t * t * t + b;
	t -= 2.0;
	return c / 2.0 * (t * t * t + 2.0) + b;
}


float
MathUtils::EaseInOutQuart(float t /*time*/, float b /*begin*/, float c /*distance*/, float d /*duration*/)
{
	t /= d / 2;

	if (t < 1)
		return c / 2 * t * t * t * t + b;

	t -= 2;

	return -c / 2 * (t * t * t * t - 2) + b;
}


float
MathUtils::EaseInOutQuint(float t /*time*/, float b /*begin*/, float c /*distance*/, float d /*duration*/)
{
	t /= d / 2;
	if (t < 1)
		return c / 2 * t * t * t * t * t + b;
	t -= 2;
	return c / 2 *(t * t * t * t * t + 2) + b;
}


float
MathUtils::EaseInOutSine(float t /*time*/, float b /*begin*/, float c /*distance*/, float d /*duration*/)
{
	return -c / 2 * (cos(3.14159 * t / d) - 1) + b;
}
