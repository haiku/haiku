/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "InterfaceUtils.h"

#include "DialUpAddon.h"
#include <ListView.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Screen.h>
#include <String.h>
#include <ListItem.h>	// Contains StringItem class declaration
#include <Window.h>


BPoint
center_on_screen(BRect rect, BWindow *window)
{
	BRect screenFrame = (BScreen(window).Frame());
	BPoint point((screenFrame.Width() - rect.Width()) / 2.0,
		(screenFrame.Height() - rect.Height()) / 2.0);
	if(!screenFrame.Contains(point))
		point.Set(0, 0);

	return point;
}


int32
FindNextMenuInsertionIndex(BMenu *menu, const char *name, int32 index)
{
	BMenuItem *item;
	for(; index < menu->CountItems(); index++) {
		item = menu->ItemAt(index);
		if(item && strcasecmp(name, item->Label()) <= 0)
			return index;
	}

	return index;
}


int32
FindNextListInsertionIndex(BListView *list, const char *name)
{
	int32 index = 0;
	BStringItem *item;
	for(; index < list->CountItems(); index++) {
		item = static_cast<BStringItem*>(list->ItemAt(index));
		if(item && strcasecmp(name, item->Text()) <= 0)
			return index;
	}

	return index;
}


void
AddAddonsToMenu(const BMessage *source, BMenu *menu, const char *type, uint32 what)
{
	DialUpAddon *addon;
	for(int32 index = 0; source->FindPointer(type, index,
			reinterpret_cast<void**>(&addon)) == B_OK; index++) {
		if(!addon || (!addon->FriendlyName() && !addon->TechnicalName()))
			continue;

		BMessage *message = new BMessage(what);
		message->AddPointer("Addon", addon);

		BString name;
		if(addon->TechnicalName()) {
			name << addon->TechnicalName();
			if(addon->FriendlyName())
				name << " (";
		}
		if(addon->FriendlyName()) {
			name << addon->FriendlyName();
			if(addon->TechnicalName())
				name << ")";
		}

		int32 insertAt = FindNextMenuInsertionIndex(menu, name.String());
		menu->AddItem(new BMenuItem(name.String(), message), insertAt);
	}
}
