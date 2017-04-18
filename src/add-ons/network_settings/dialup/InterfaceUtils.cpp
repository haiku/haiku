/* -----------------------------------------------------------------------
 * Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
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
 * ----------------------------------------------------------------------- */

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
