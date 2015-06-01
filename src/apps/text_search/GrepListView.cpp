/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "GrepListView.h"

#include <Path.h>

ResultItem::ResultItem(const entry_ref& ref)
	: BStringItem("", 0, false),
	  ref(ref)
{
	BEntry entry(&ref);
	BPath path(&entry);
	SetText(path.Path());
}


GrepListView::GrepListView()
	: BOutlineListView("SearchResults",
		B_MULTIPLE_SELECTION_LIST,
		B_WILL_DRAW | B_NAVIGABLE)
{
}


ResultItem*
GrepListView::FindItem(const entry_ref& ref, int32* _index) const
{
	int32 count = FullListCountItems();
	for (int32 i = 0; i < count; i++) {
		ResultItem* item = dynamic_cast<ResultItem*>(FullListItemAt(i));
		if (item == NULL)
			continue;
		if (item->ref == ref) {
			*_index = i;
			return item;
		}
	}
	*_index = -1;
	return NULL;
}


ResultItem*
GrepListView::RemoveResults(const entry_ref& ref, bool completeItem)
{
	int32 index;
	ResultItem* item = FindItem(ref, &index);
	if (item == NULL)
		return NULL;

	// remove all the sub items
	while (true) {
		BListItem* subItem = FullListItemAt(index + 1);
		if (subItem && subItem->OutlineLevel() > 0)
			delete RemoveItem(index + 1);
		else
			break;
	}

	if (completeItem) {
		// remove file item itself
		delete RemoveItem(index);
		item = NULL;
	}

	return item;
}

