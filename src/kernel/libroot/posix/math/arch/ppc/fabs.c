/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <math.h>


double
fabs(double value)
{
	// ToDo: that's a slow, generic implementation
	if (value > 0)
		return value;

	return -value;
}
