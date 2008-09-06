/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Rene Gollent (rene@gollent.com)
 */
#ifndef COLORWHICH_ITEM_H
#define COLORWHICH_ITEM_H

#include <InterfaceDefs.h>
#include <ListItem.h>

class ColorWhichItem : public BStringItem
{
public:
	ColorWhichItem(const char* text, color_which which);

	color_which ColorWhich(void);

private:
	color_which colorWhich;
};

#endif
