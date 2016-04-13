/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *
 * Based on ColorWhichItem by DarkWyrm (bpmagic@columbus.rr.com)
 */
#ifndef _COLOR_ITEM_H
#define _COLOR_ITEM_H


#include <InterfaceDefs.h>
#include <StringItem.h>


class ColorItem : public BStringItem {
public:
							ColorItem(const char* string, rgb_color color);

	virtual	void			DrawItem(BView* owner, BRect frame, bool complete);
	virtual	void			SetColor(rgb_color color);

private:
			rgb_color		fColor;
};


#endif	// _COLOR_ITEM_H
