/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "fssh_stdio.h"

#include <stdio.h>
#include <stdlib.h>


int
fssh_sprintf(char *string, char const *format, ...)
{
	va_list args;
	va_start(args, format);

	int result = vsprintf(string, format, args);

	va_end(args);

	return result;
}


int
fssh_snprintf(char *string, fssh_size_t size, char const *format, ...)
{
	va_list args;
	va_start(args, format);

	int result = vsnprintf(string, size, format, args);

	va_end(args);

	return result;
}


int
fssh_vsprintf(char *string, char const *format, va_list ap)
{
	return vsprintf(string, format, ap);
}


int
fssh_vsnprintf(char *string, fssh_size_t size, char const *format, va_list ap)
{
	return vsnprintf(string, size, format, ap);
}


int
fssh_sscanf(char const *str, char const *format, ...)
{
	va_list args;
	va_start(args, format);

	int result = vsscanf(str, format, args);

	va_end(args);

	return result;
}


int
fssh_vsscanf(char const *str, char const *format, va_list ap)
{
	return vsscanf(str, format, ap);
}
