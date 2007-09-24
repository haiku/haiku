/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <locale.h>
#include <limits.h>


char *
setlocale(int category, const char *locale)
{
	if (locale == NULL || !locale[0])
		return "C";

	// ToDo: this should check if liblocale.so is available and use its functions
	return NULL;
}
