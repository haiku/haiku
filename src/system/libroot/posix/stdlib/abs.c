/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 * 		Daniel Reinhold, danielre@users.sf.net
 */


#include <stdlib.h>


int
abs(int i)
{
	return (i < 0) ? -i : i;
}


long
labs(long i)
{
	return (i < 0) ? -i : i;
}


long long
llabs(long long i)
{
	return (i < 0) ? -i : i;
}
