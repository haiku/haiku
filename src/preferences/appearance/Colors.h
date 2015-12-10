/*
 * Copyright 2001-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Rene Gollent <rene@gollent.com>
 *		Joseph Groover <looncraz@looncraz.net>
 */
#ifndef COLORS_H
#define COLORS_H


#include <InterfaceDefs.h>


typedef struct {
	color_which	which;
	const char*	text;
} ColorDescription;


const ColorDescription* get_color_description(int32 index);
int32 color_description_count(void);
void get_default_colors(BMessage* storage);
void get_current_colors(BMessage* storage);


#endif	// COLORS_H
