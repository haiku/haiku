/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform/generic/text_console.h>


void
console_clear_screen(void)
{
}


int32
console_width(void)
{
	return 80;
}


int32
console_height(void)
{
	return 25;
}


void
console_set_cursor(int32 x, int32 y)
{
}


void
console_show_cursor(void)
{
}


void
console_hide_cursor(void)
{
}


void
console_set_color(int32 foreground, int32 background)
{
}


int
console_wait_for_key(void)
{
	return 0;
}


status_t
console_init(void)
{
	return B_OK;
}
