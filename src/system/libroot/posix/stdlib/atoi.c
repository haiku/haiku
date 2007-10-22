/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 * 		Daniel Reinhold, danielre@users.sf.net
 */

 
#include <stdlib.h>


int
atoi(const char* num)
{
	return (int) strtol(num, NULL, 10);
}


unsigned int
atoui(const char* num)
{
	return (unsigned int) strtoul(num, NULL, 10);
}


long
atol(const char* num)
{
	return strtol(num, NULL, 10);
}


unsigned long
atoul(const char* num)
{
	return strtoul(num, NULL, 10);
}


long long int
atoll(const char* num)
{
	return strtoll(num, NULL, 10);
}
