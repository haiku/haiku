/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

// Some math utils, useful for animation


#include "MathUtils.h"

#include <math.h>


float
MathUtils::EaseInOutCubic(float time, float start, float distance,
	float duration)
{
	time /= duration  / 2.0;
	if (time < 1.0)
		return distance / 2.0 * time * time * time + start;
	time -= 2.0;
	return distance / 2.0 * (time * time * time + 2.0) + start;
}


float
MathUtils::EaseInOutQuart(float time, float start, float distance,
	float duration)
{
	time /= duration  / 2;

	if (time < 1)
		return distance / 2 * time * time * time * time + start;

	time -= 2;

	return -distance / 2 * (time * time * time * time - 2) + start;
}


float
MathUtils::EaseInOutQuint(float time, float start, float distance,
	float duration)
{
	time /= duration  / 2;
	if (time < 1)
		return distance / 2 * time * time * time * time * time + start;
	time -= 2;
	return distance / 2 *(time * time * time * time * time + 2) + start;
}


float
MathUtils::EaseInOutSine(float time, float start, float distance,
	float duration)
{
	return -distance / 2 * (cos(3.14159 * time / distance) - 1) + start;
}
