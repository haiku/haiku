/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall <mccall@@digitalparadise.co.uk>
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 *
 */

#include "Bitmaps.h"


#include <Debug.h>
#include <Screen.h>


void
ReplaceTransparentColor(BBitmap* bitmap, rgb_color with)
{
	// other color spaces not implemented yet
	ASSERT(bitmap->ColorSpace() == B_COLOR_8_BIT);

	BScreen screen(B_MAIN_SCREEN_ID);
	uint32 withIndex = screen.IndexForColor(with);

	uchar* bits = (uchar*)bitmap->Bits();
	int32 bitsLength = bitmap->BitsLength();
	for (int32 index = 0; index < bitsLength; index++)
		if (bits[index] == B_TRANSPARENT_8_BIT)
			bits[index] = withIndex;
}
