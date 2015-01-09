/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Josef Gajdusek
 */


#include "PopUpColumn.h"

#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Window.h>

#include "EditWindow.h"
#include "ShortcutsWindow.h"

PopUpColumn::PopUpColumn(BPopUpMenu* menu, const char* name, float width,
	float minWidth, float maxWidth, uint32 truncate, bool editable,
	bool cycle, int cycleInit, alignment align)
	:
	BStringColumn(name, width, minWidth, maxWidth, truncate, align),
	fEditable(editable),
	fCycle(cycle),
	fCycleInit(cycleInit),
	fMenu(menu)
{
	SetWantsEvents(true);
}


PopUpColumn::~PopUpColumn()
{
	delete fMenu;
}

void
PopUpColumn::MouseDown(BColumnListView* parent, BRow* row, BField* field,
	BRect fieldRect, BPoint point, uint32 buttons)
{
	if ((buttons & B_SECONDARY_MOUSE_BUTTON)
		|| (buttons & B_PRIMARY_MOUSE_BUTTON && (fEditable || fCycle))) {
		BMessage* msg = new BMessage(ShortcutsWindow::HOTKEY_ITEM_MODIFIED);
		msg->SetInt32("row", parent->IndexOf(row));
		msg->SetInt32("column", LogicalFieldNum());
		if (buttons & B_SECONDARY_MOUSE_BUTTON) {
			BMenuItem* selected = fMenu->Go(parent->ConvertToScreen(point));
			if (selected) {
				msg->SetString("text", selected->Label());
				parent->Window()->PostMessage(msg);
			}
		}
		if (buttons & B_PRIMARY_MOUSE_BUTTON && row->IsSelected()) {
			BStringField* stringField = static_cast<BStringField*>(field);
			if (fEditable) {
				EditWindow* edit = new EditWindow(stringField->String(), 0);
				msg->SetString("text", edit->Go());
			} else if (fCycle) {
				BMenuItem* item;
				for (int i = 0; (item = fMenu->ItemAt(i)) != NULL; i++)
					if (strcmp(stringField->String(), item->Label()) == 0) {
						item = fMenu->ItemAt((i + 1) % fMenu->CountItems());
						break;
					}
				if (item == NULL)
					item = fMenu->ItemAt(fCycleInit);
				msg->SetString("text", item->Label());
			}
			parent->Window()->PostMessage(msg);
		}
	}
	BStringColumn::MouseDown(parent, row, field, fieldRect, point, buttons);
}

