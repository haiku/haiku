/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Rene Gollent (rene@gollent.com)
 *		Ryan Leavengood <leavengood@gmail.com>
 */


#ifndef COLORWHICH_ITEM_H
#define COLORWHICH_ITEM_H

#include <InterfaceDefs.h>
#include <ListItem.h>
#include <View.h>

class ColorWhichItem : public BStringItem
{
public:
						ColorWhichItem(const char* text, color_which which, rgb_color color);

	virtual void		DrawItem(BView *owner, BRect frame, bool complete);
			color_which	ColorWhich(void);
			void		SetColor(rgb_color color);

private:
			color_which fColorWhich;
			rgb_color fColor;
};

#endif
