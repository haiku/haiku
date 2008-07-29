/*
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <math.h>
#include <stdlib.h>

// random_number_between
inline float
random_number_between(float v1, float v2)
{
	if (v1 < v2)
		return v1 + fmod(rand() / 1000.0, (v2 - v1));
	else if (v2 < v1)
		return v2 + fmod(rand() / 1000.0, (v1 - v2));
	return v1;
}

#endif // TEST_SUPPORT_H
