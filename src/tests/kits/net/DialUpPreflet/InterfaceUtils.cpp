//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#include "InterfaceUtils.h"

#include "DialUpAddon.h"
#include <ListView.h>
#include <Menu.h>
#include <MenuItem.h>
#include <String.h>
#include <StringItem.h>


int32
FindNextMenuInsertionIndex(BMenu *menu, const BString& name, int32 index = 0)
{
	BMenuItem *item;
	for(; index < menu->CountItems(); index++) {
		item = menu->ItemAt(index);
		if(item && name.ICompare(item->Label()) <= 0)
			return index;
	}
	
	return index;
}


int32
FindNextListInsertionIndex(BListView *list, const BString& name)
{
	int32 index = 0;
	BStringItem *item;
	for(; index < list->CountItems(); index++) {
		item = static_cast<BStringItem*>(list->ItemAt(index));
		if(item && name.ICompare(item->Text()) <= 0)
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
		
		int32 insertAt = FindNextMenuInsertionIndex(menu, name);
		menu->AddItem(new BMenuItem(name.String(), message), insertAt);
	}
}
