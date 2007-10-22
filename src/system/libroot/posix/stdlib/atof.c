/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 * 		Daniel Reinhold, danielre@users.sf.net
 */


#include <stdlib.h>


double
atof(const char* num)
{
	return strtod(num, NULL);
}
