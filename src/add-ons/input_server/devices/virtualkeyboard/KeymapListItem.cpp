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
 
#include "KeymapListItem.h"


KeymapListItem::KeymapListItem(entry_ref& keymap, const char* name)
	:
	BStringItem(name ? name : keymap.name),
	fKeymap(keymap)
{
}
