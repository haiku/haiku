/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include "stage2_priv.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

void puts(const char *str)
{
	while (*str)
		putc(*str++);
}

int dprintf(const char *fmt, ...)
{
	int ret;
	va_list args;
	char temp[256];

	va_start(args, fmt);
	ret = vsprintf(temp,fmt,args);
	va_end(args);

	puts(temp);
	return ret;
}

