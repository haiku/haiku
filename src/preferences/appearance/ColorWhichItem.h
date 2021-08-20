/*
 * Copyright 2001-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Rene Gollent, rene@gollent.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef COLORWHICH_ITEM_H
#define COLORWHICH_ITEM_H


#include <InterfaceDefs.h>
#include <StringItem.h>


class ColorWhichItem : public BStringItem
{
public:
							ColorWhichItem(const char* text, color_which which,
								rgb_color color);

	virtual void			DrawItem(BView* owner, BRect frame, bool complete);

			color_which		ColorWhich() { return fColorWhich; };

			rgb_color		Color() { return fColor; };
			void			SetColor(rgb_color color) { fColor = color; };

private:
			color_which		fColorWhich;
			rgb_color		fColor;
};


#endif
