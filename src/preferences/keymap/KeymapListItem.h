/*
 * Copyright 2004-2006 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sandor Vroemisse
 *		Jérôme Duval
 */

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
