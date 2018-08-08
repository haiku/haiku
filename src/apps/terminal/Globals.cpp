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
	if (font.IsFixed() || font.IsFullAndHalfFixed())
		return true;

	return false;
}
