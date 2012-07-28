/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */
#ifndef COLOR_MENU_ITEM_H
#define COLOR_MENU_ITEM_H


#include <MenuItem.h>


class BMessage;


class ColorMenuItem: public BMenuItem {
	public:
						ColorMenuItem(const char* label, rgb_color color, 
							BMessage* message);

	protected:
		virtual void	DrawContent();

	private:
		rgb_color		fItemColor;
};

#endif	// COLOR_MENU_ITEM_H

