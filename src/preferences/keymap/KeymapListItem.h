// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        KeymapListItem.h
//  Author:      Sandor Vroemisse, Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 12, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

/*
 * A BStringItem modified such that it holds
 * the BEntry object it corresponds with
 */

#ifndef KEYMAP_LIST_ITEM_H
#define KEYMAP_LIST_ITEM_H

#include <ListItem.h>
#include <Entry.h>

class KeymapListItem : public BStringItem {
public:
	KeymapListItem( entry_ref &keymap, const char* name = NULL);
	entry_ref & KeymapEntry() { return fKeymap; };

protected:
	entry_ref	fKeymap;
};

#endif //KEYMAP_LIST_ITEM_H
