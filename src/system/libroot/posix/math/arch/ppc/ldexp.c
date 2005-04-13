/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <math.h>


double
ldexp(double value, int exp)
{
	// ToDo: that's a slow, generic implementation
	double power = 1.;
	while (exp-- > 0)
		power *= 2.;

	return value * power;
}
