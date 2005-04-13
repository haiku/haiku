/*
 *  Copyright (c) 2002, OpenBeOS Project.
 *  All rights reserved.
 *  Distributed under the terms of the OpenBeOS license.
 *
 *
 *  atof.c:
 *  implements the standard C library function atof()
 *  (merely a wrapper for strtod(), actually)
 *
 *
 *  Author(s):
 *  Daniel Reinhold (danielre@users.sf.net)
 *
 */
 
#include <stdlib.h>


double
atof(const char *num)
{
	return strtod(num, NULL);
}


