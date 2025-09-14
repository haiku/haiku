/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "GrepListView.h"

#include <Catalog.h>
#include <MenuItem.h>
#include <Path.h>
#include <Window.h>

#include "Model.h"

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
	_CreatePopUpMenu();
}


void
GrepListView::AttachedToWindow()
{
	fContextMenu->SetTargetForItems(Window());
}


void
GrepListView::MouseDown(BPoint where)
{
	int32 index = IndexOf(where);
	if (index >= 0) {
		if (Window() != NULL && Window()->CurrentMessage() != NULL) {
			uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");
			if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
				BListItem* item = ItemAt(index);
				bool clickOnSelection = item != NULL && item->IsSelected();
				if (CurrentSelection() < 0 || !clickOnSelection)
					Select(index);
				if (CurrentSelection() >= 0)
					fContextMenu->Go(ConvertToScreen(where), true, true, true);
				return;
			}
		}
	}
	BOutlineListView::MouseDown(where);
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


void
GrepListView::_CreatePopUpMenu()
{
	fContextMenu = new BPopUpMenu("ContextMenu", false, false);

	#undef B_TRANSLATION_CONTEXT
	#define B_TRANSLATION_CONTEXT "GrepWindow"
	fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(MSG_SELECT_ALL), 'A', B_SHIFT_KEY));

	fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Trim to selection"),
		new BMessage(MSG_TRIM_SELECTION), 'T'));

	fContextMenu->AddSeparatorItem();

	fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Open selection"),
		new BMessage(MSG_OPEN_SELECTION), 'O'));

	fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Show files in Tracker"),
		new BMessage(MSG_SELECT_IN_TRACKER), 'K'));

	fContextMenu->AddItem(new BMenuItem(B_TRANSLATE("Copy text to clipboard"),
		new BMessage(MSG_COPY_TEXT), 'B'));
}