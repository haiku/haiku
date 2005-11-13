/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <math.h>


float
roundf(float value)
{
	if (value >= 0.0f)
		return floorf(value + 0.5f);

	return ceilf(value + 0.5f);
}


double
round(double value)
{
	if (value >= 0.0)
		return floor(value + 0.5);

	return ceil(value + 0.5);
}


long double
roundl(long double value)
{
	// TODO: fix me!
	return (long double)round((double)value);
#if 0
	if (value >= 0.0)
		return floorl(value + 0.5);

	return ceill(value + 0.5);
#endif
}

