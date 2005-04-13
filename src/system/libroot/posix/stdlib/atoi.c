/*
 *  Copyright (c) 2002, OpenBeOS Project.
 *  All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *
 *  atoi.c:
 *  implements the standard C library functions:
 *    atoi, atoui, atol, atoul, atoll
 *  (these are all just wrappers around calls to the strto[u]l[l] functions)
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */
 
#include <stdlib.h>


int
atoi(const char *num)
{
	return (int) strtol(num, NULL, 10);
}


unsigned int
atoui(const char *num)
{
	return (unsigned int) strtoul(num, NULL, 10);
}


long
atol(const char *num)
{
	return strtol(num, NULL, 10);
}


unsigned long
atoul(const char *num)
{
	return strtoul(num, NULL, 10);
}


long long int
atoll(const char *num)
{
	return strtoll(num, NULL, 10);
}

