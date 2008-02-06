/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>


char *
gcvt(double value, int digits, char *buffer)
{
	sprintf(buffer, "%.*g", digits, value);
	return buffer;
}


// TODO: eventually add ecvt(), and fcvt() as well, if needed.
