/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <locale.h>
#include <limits.h>


char *
setlocale(int category, const char *locale)
{
	// ToDo: this should check if liblocale.so is available and use its functions
	return NULL;
}
