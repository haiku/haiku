/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Globals.h"

#include <math.h>
#include <stddef.h>

#include <Font.h>


BClipboard* gMouseClipboard = NULL;


bool
IsFontUsable(const BFont& font)
{
	// TODO: If BFont::IsFullAndHalfFixed() was implemented, we could
	// use that. But I don't think it's easily implementable using
	// Freetype.

	if (font.IsFixed())
		return true;

	// manually check if all applicable chars are the same width
	char buffer[2] = { ' ', 0 };
	int firstWidth = (int)ceilf(font.StringWidth(buffer));

	// TODO: Workaround for broken fonts/font_subsystem
	if (firstWidth <= 0)
		return false;

	for (int c = ' ' + 1; c <= 0x7e; c++) {
		buffer[0] = c;
		int width = (int)ceilf(font.StringWidth(buffer));

		if (width != firstWidth)
			return false;
	}

	return true;
}
