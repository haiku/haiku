/*
 * ValidRect.h
 * Copyright 1999-2000 Y.Takagi All Rights Reserved.
 */

#ifndef __VALIDRECT_H
#define __VALIDRECT_H

#include <GraphicsDefs.h>

class BBitmap;

struct RECT {
	int left;
	int top;
	int right;
	int bottom;
};

bool get_valid_rect(BBitmap *bitmap, RECT *rc);

int color_space2pixel_depth(color_space cs);

#endif	// __VALIDRECT_H
