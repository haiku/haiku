/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <SupportDefs.h>

#include <time.h>
#include <string.h>


extern "C" size_t
strftime(char *buffer, size_t bufferSize, const char *format, const struct tm *tm)
{
	// ToDo: implement (or copy) strftime()
	return strlcpy(buffer, "99-Sep-99", bufferSize);
}

