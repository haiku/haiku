/*
 * Copyright 2003-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer, laplace@haiku-os.org
 */
#ifndef ENTRY_MENU_ITEM_H
#define ENTRY_MENU_ITEM_H


#include <Bitmap.h>
#include <Entry.h>
#include <MenuItem.h>
#include <Mime.h>


class EntryMenuItem : public BMenuItem {
public:
				EntryMenuItem(entry_ref* entry, const char* label,
					BMessage* message, char shortcut = 0, uint32 modifiers = 0);
				~EntryMenuItem();
	
	void 		GetContentSize(float* width, float* height);
	void 		DrawContent();
	
private:
	BBitmap* 	GetIcon(BMimeType* mimeType);
	BBitmap* 	LoadIcon();

	entry_ref 	fEntry;
	BBitmap* 	fSmallIcon;

	enum {
		kIconSize = 16,
		kTextIndent = kIconSize + 4,
	};
};


#endif	// ENTRY_MENU_ITEM_H

