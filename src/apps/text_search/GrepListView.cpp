/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
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
	: BOutlineListView(BRect(0, 0, 40, 80), "SearchResults", 
		B_MULTIPLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES,
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

