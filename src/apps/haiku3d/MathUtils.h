/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _MATH_UTILS_H
#define _MATH_UTILS_H


class MathUtils
{
public:
	static	float	EaseInOutCubic(float time, float begin, float distance,
						float duration);
	static	float	EaseInOutQuart(float time, float begin, float distance,
						float duration);
	static	float	EaseInOutQuint(float time, float begin, float distance,
						float duration);
	static	float	EaseInOutSine(float time, float begin, float distance,
						float duration);
};

#endif /* _MATH_UTILS_H */
