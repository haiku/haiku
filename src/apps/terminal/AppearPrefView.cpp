/*
 * Copyright 2001-2007, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai. 
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AppearPrefView.h"
#include "MenuUtil.h"
#include "PrefHandler.h"
#include "TermConst.h"

#include <View.h>
#include <Button.h>
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <stdlib.h>


AppearancePrefView::AppearancePrefView(BRect frame, const char *name,
	BMessenger messenger)
	: PrefView(frame, name),
	fAppearancePrefViewMessenger(messenger)
{
  	const char *color_tbl[] = {
    	PREF_TEXT_FORE_COLOR,
    	PREF_TEXT_BACK_COLOR,
    	PREF_CURSOR_FORE_COLOR,
    	PREF_CURSOR_BACK_COLOR,
    	PREF_SELECT_FORE_COLOR,
    	PREF_SELECT_BACK_COLOR,
    	NULL
  	};

	float fontDividerSize = StringWidth("Font:") + 8.0;
	float sizeDividerSize = StringWidth("Size:") + 8.0;

	BRect r(5, 5, 225, 25);

	BMenu *menu = _MakeFontMenu(MSG_HALF_FONT_CHANGED,
		gTermPref->getString(PREF_HALF_FONT_FAMILY));
	fFont = new BMenuField(r, "font", "Font:", menu, B_WILL_DRAW);
	fFont->SetDivider(fontDividerSize);
	AddChild(fFont);

	r.OffsetBy(r.Width() + 10, 0);
	menu = _MakeSizeMenu(MSG_HALF_SIZE_CHANGED, 12);
	fFontSize = new BMenuField(r, "size", "Size:", menu, B_WILL_DRAW);
	fFontSize->SetDivider(sizeDividerSize);
	AddChild(fFontSize);

	r.OffsetBy(-r.Width() - 10,r.Height() + 25);
	fColorField = new BMenuField(r, "color", "Change:",
		MakeMenu(MSG_COLOR_FIELD_CHANGED, color_tbl, color_tbl[0]), B_WILL_DRAW);
	fColorField->SetDivider(StringWidth(fColorField->Label()) + 8.0);
	AddChild(fColorField);

  	fColorControl = SetupBColorControl(BPoint(r.left, r.bottom + 10),
  		B_CELLS_32x8, 6, MSG_COLOR_CHANGED);
  	fColorControl->SetValue(gTermPref->getRGB(PREF_TEXT_FORE_COLOR));
}


void
AppearancePrefView::GetPreferredSize(float *_width, float *_height)
{
	if (_width)
		*_width = Bounds().Width();

	if (*_height)
		*_height = fColorControl->Frame().bottom;
}


void
AppearancePrefView::Revert()
{
	fColorField->Menu()->ItemAt(0)->SetMarked(true);
	fColorControl->SetValue(gTermPref->getRGB(PREF_TEXT_FORE_COLOR));
	
	fFont->Menu()->FindItem(gTermPref->getString(PREF_HALF_FONT_FAMILY))->SetMarked(true);
	fFontSize->Menu()->FindItem(gTermPref->getString(PREF_HALF_FONT_FAMILY))->SetMarked(true);
}


void
AppearancePrefView::AttachedToWindow()
{
	fFontSize->Menu()->SetTargetForItems(this);
	fFont->Menu()->SetTargetForItems(this);

  	fColorControl->SetTarget(this);
  	fColorField->Menu()->SetTargetForItems(this);
}


void
AppearancePrefView::MessageReceived(BMessage *msg)
{
	bool modified = false;

	switch (msg->what) { 
		case MSG_HALF_FONT_CHANGED:
			gTermPref->setString (PREF_HALF_FONT_FAMILY,
				fFont->Menu()->FindMarked()->Label());
			modified = true;
			break;

		case MSG_HALF_SIZE_CHANGED:
			gTermPref->setString(PREF_HALF_FONT_SIZE,
				fFontSize->Menu()->FindMarked()->Label());
			modified = true;
			break;

		case MSG_COLOR_CHANGED:
			gTermPref->setRGB(fColorField->Menu()->FindMarked()->Label(),
				fColorControl->ValueAsColor());    
			modified = true;
			break;

		case MSG_COLOR_FIELD_CHANGED:
			fColorControl->SetValue(gTermPref->getRGB(
				fColorField->Menu()->FindMarked()->Label()));
			break;

		default:
			PrefView::MessageReceived(msg);
			return;
	}

	if (modified) {
		fAppearancePrefViewMessenger.SendMessage(msg);
			// send message to fTermWindow
		
		// send the MSG_PREF_MODIFIED message
		// to fPrefWindow
		BMessenger messenger(this);
		messenger.SendMessage(MSG_PREF_MODIFIED);
	}
}


BMenu *
AppearancePrefView::_MakeFontMenu(uint32 command, const char *defaultFontName)
{
	BPopUpMenu *menu = new BPopUpMenu("");
	int32 numFamilies = count_font_families(); 

	for (int32 i = 0; i < numFamilies; i++) {
		font_family family;
		uint32 flags;

		if (get_font_family(i, &family, &flags) == B_OK) {
			menu->AddItem(new BMenuItem(family, new BMessage(command)));
			if (!strcmp(defaultFontName, family)) {
				BMenuItem* item = menu->ItemAt(i);
				item->SetMarked(true);
			}
		}
	}

	return menu;
}


BMenu *
AppearancePrefView::_MakeSizeMenu(uint32 command, uint8 defaultSize)
{
	BPopUpMenu *menu = new BPopUpMenu("size");
	int32 sizes[] = {9, 10, 11, 12, 14, 16, 18, 0};
	
	for (uint32 i = 0; sizes[i]; i++) {
		BString string;
		string << sizes[i];

		BMenuItem* item = new BMenuItem(string.String(), new BMessage(command));
		menu->AddItem(item);

		if (sizes[i] == defaultSize)
			item->SetMarked(true);
	}

	return menu;
}
