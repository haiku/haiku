/*
 * Copyright (C) 2009 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DrawingModeToString.h"

#include <string.h>


struct DrawingModeString {
	const char*		string;
	drawing_mode	mode;
};


static const DrawingModeString kDrawingModes[] = {
	{ "B_OP_COPY",		B_OP_COPY },
	{ "B_OP_OVER",		B_OP_OVER },
	{ "B_OP_ERASE",		B_OP_ERASE },
	{ "B_OP_INVERT",	B_OP_INVERT },
	{ "B_OP_ADD",		B_OP_ADD },
	{ "B_OP_SUBTRACT",	B_OP_SUBTRACT },
	{ "B_OP_BLEND",		B_OP_BLEND },
	{ "B_OP_MIN",		B_OP_MIN },
	{ "B_OP_MAX",		B_OP_MAX },
	{ "B_OP_SELECT",	B_OP_SELECT },
	{ "B_OP_ALPHA",		B_OP_ALPHA }
};


bool ToDrawingMode(const char* string, drawing_mode& mode)
{
	int entries = sizeof(kDrawingModes) / sizeof(DrawingModeString);
	for (int32 i = 0; i < entries; i++) {
		if (strcmp(kDrawingModes[i].string, string) == 0) {
			mode = kDrawingModes[i].mode;
			return true;
		}
	}
	return false;
}


bool ToString(drawing_mode mode, const char*& string)
{
	int entries = sizeof(kDrawingModes) / sizeof(DrawingModeString);
	for (int32 i = 0; i < entries; i++) {
		if (kDrawingModes[i].mode == mode) {
			string = kDrawingModes[i].string;
			return true;
		}
	}
	return false;
}
