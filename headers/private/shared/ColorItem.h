/*
 * Copyright 2016-2022 Haiku, Inc. All rights reserved.
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
			void			SetColor(rgb_color color) { fColor = color; }
			rgb_color		Color() const { return fColor; }

private:
			rgb_color		fColor;
};


#endif	// _COLOR_ITEM_H
