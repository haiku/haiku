/*
 * Copyright 2016-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Based on ColorWhichItem by DarkWyrm (bpmagic@columbus.rr.com)
 */
#ifndef _COLOR_ITEM_H
#define _COLOR_ITEM_H


#include <InterfaceDefs.h>
#include <StringItem.h>


namespace BPrivate {


class BColorItem : public BStringItem {
public:
							BColorItem(const char* text, rgb_color color);
							BColorItem(const char* text, color_which which, rgb_color color);

	virtual	void			DrawItem(BView* owner, BRect frame, bool complete);

			void			SetColor(rgb_color color) { fColor = color; };
			rgb_color		Color() { return fColor; };

			color_which		ColorWhich() { return fColorWhich; };

private:
			rgb_color		fColor;
			color_which		fColorWhich;
};


} // namespace BPrivate


// remove once apps have been updated to use BColorItem
typedef BPrivate::BColorItem ColorItem;


#endif // _COLOR_ITEM_H
