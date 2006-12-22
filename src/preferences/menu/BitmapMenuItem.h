/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 */
#ifndef BITMAP_MENU_ITEM_H
#define BITMAP_MENU_ITEM_H


#include <MenuItem.h>
#include <String.h>

class BBitmap;


class BitmapMenuItem : public BMenuItem {
	public:
		BitmapMenuItem(const char* name, BMessage* message, BBitmap* bmp,
			char shortcut = '\0', uint32 modifiers = 0);
		~BitmapMenuItem();
		virtual void DrawContent();

	protected:
		virtual void GetContentSize(float *width, float *height);	

	private:
		BBitmap* fBitmap;
		BString fName;
};

#endif	// BITMAP_MENU_ITEM_H
