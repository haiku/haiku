/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Rene Gollent (rene@gollent.com)
 */
#include "ColorWhichItem.h"
#include <stdio.h>

ColorWhichItem::ColorWhichItem(const char* text, color_which which)
	: BStringItem(text, 0, false)
	, colorWhich(which)
{
}

color_which
ColorWhichItem::ColorWhich(void)
{
	return colorWhich;
}

