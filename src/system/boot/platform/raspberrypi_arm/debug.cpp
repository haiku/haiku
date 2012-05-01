/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stdio.h>
#include <stdarg.h>


void
panic(const char* format, ...)
{
#warning IMPLEMENT panic
}


void
dprintf(const char* format, ...)
{
#warning IMPLEMENT dprintf
}


char*
platform_debug_get_log_buffer(size_t* _size)
{
	return NULL;
}
