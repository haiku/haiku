/*
 * Copyright 2001-2009, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AppearPrefView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Button.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <View.h>

#include "MenuUtil.h"
#include "PrefHandler.h"
#include "TermConst.h"



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
#if 0
		"",
		PREF_IM_FORE_COLOR,
		PREF_IM_BACK_COLOR,
		PREF_IM_SELECT_COLOR,
		"",
		PREF_ANSI_BLACK_COLOR,
		PREF_ANSI_RED_COLOR,
		PREF_ANSI_GREEN_COLOR,
		PREF_ANSI_YELLOW_COLOR,
		PREF_ANSI_BLUE_COLOR,
		PREF_ANSI_MAGENTA_COLOR,
		PREF_ANSI_CYAN_COLOR,
		PREF_ANSI_WHITE_COLOR,
#endif
		NULL
  	};

	float greenDividerSize = StringWidth("Green:") + 8.0;
	float colorDividerSize = StringWidth("Color:") + 8.0;

	BRect r(5, 5, 261, 25);

	BMenu *menu = _MakeFontMenu(MSG_HALF_FONT_CHANGED,
		PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY),
		PrefHandler::Default()->getString(PREF_HALF_FONT_STYLE));
	fFont = new BMenuField(r, "font", "Font:", menu);
	fFont->SetDivider(colorDividerSize);
	fFont->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fFont);

	r.OffsetBy(r.Width() + 10, 0);
	menu = _MakeSizeMenu(MSG_HALF_SIZE_CHANGED,
		PrefHandler::Default()->getInt32(PREF_HALF_FONT_SIZE));
	fFontSize = new BMenuField(r, "size", "Size:", menu);
	fFontSize->SetDivider(greenDividerSize);
	fFontSize->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fFontSize);

	r.OffsetBy(-r.Width() - 10,r.Height() + 10);
	fColorField = new BMenuField(r, "color", "Color:",
		MakeMenu(MSG_COLOR_FIELD_CHANGED, color_tbl, color_tbl[0]));
	fColorField->SetDivider(colorDividerSize);
	fColorField->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fColorField);

	fColorControl = SetupColorControl(BPoint(r.left, r.bottom + 10),
		B_CELLS_32x8, 8.0, MSG_COLOR_CHANGED);
	fColorControl->SetValue(PrefHandler::Default()->getRGB(PREF_TEXT_FORE_COLOR));

	BTextControl* redInput = (BTextControl*)fColorControl->ChildAt(0);
	BTextControl* greenInput = (BTextControl*)fColorControl->ChildAt(1);
	BTextControl* blueInput = (BTextControl*)fColorControl->ChildAt(2);
	
	redInput->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	greenInput->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	blueInput->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);

	redInput->SetDivider(greenDividerSize);
	greenInput->SetDivider(greenDividerSize);
	blueInput->SetDivider(greenDividerSize);
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
	fColorControl->SetValue(PrefHandler::Default()->getRGB(PREF_TEXT_FORE_COLOR));

	fFont->Menu()->FindItem(PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY))->SetMarked(true);
	fFontSize->Menu()->FindItem(PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY))->SetMarked(true);
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
		{
			const char *family = NULL;
			const char *style = NULL;
			msg->FindString("font_family", &family);
			msg->FindString("font_style", &style);
			
			PrefHandler *pref = PrefHandler::Default();
			const char *currentFamily 
				= pref->getString(PREF_HALF_FONT_FAMILY);
			const char *currentStyle
				= pref->getString(PREF_HALF_FONT_STYLE);
			if (currentFamily == NULL || strcmp(currentFamily, family)
				|| currentStyle == NULL || strcmp(currentStyle, style)) {
				pref->setString(PREF_HALF_FONT_FAMILY, family);
				pref->setString(PREF_HALF_FONT_STYLE, style);		
				modified = true;
			}
			break;
		}
		case MSG_HALF_SIZE_CHANGED:
			if (strcmp(PrefHandler::Default()->getString(PREF_HALF_FONT_SIZE),
					fFontSize->Menu()->FindMarked()->Label())) {
	
				PrefHandler::Default()->setString(PREF_HALF_FONT_SIZE,
					fFontSize->Menu()->FindMarked()->Label());
				modified = true;
			}
			break;

		case MSG_COLOR_CHANGED: {
				rgb_color oldColor = PrefHandler::Default()->getRGB(
					fColorField->Menu()->FindMarked()->Label());
				if (oldColor != fColorControl->ValueAsColor()) {
					PrefHandler::Default()->setRGB(
						fColorField->Menu()->FindMarked()->Label(),
						fColorControl->ValueAsColor());
					modified = true;
				}
			}
			break;

		case MSG_COLOR_FIELD_CHANGED:
			fColorControl->SetValue(PrefHandler::Default()->getRGB(
				fColorField->Menu()->FindMarked()->Label()));
			break;

		default:
			PrefView::MessageReceived(msg);
			return;
	}

	if (modified) {
		fAppearancePrefViewMessenger.SendMessage(msg);

		BMessenger messenger(this);
		messenger.SendMessage(MSG_PREF_MODIFIED);
	}
}


static bool
IsFontUsable(const BFont &font)
{
	// TODO: If BFont::IsFullAndHalfFixed() was implemented, we could
	// use that. But I don't think it's easily implementable using
	// Freetype.

	if (font.IsFixed())
		return true;
	
	//font_height fontHeight;
	
	// manually check if all applicable chars are the same width
	char buffer[2] = { ' ', 0 };
	int firstWidth = (int)ceilf(font.StringWidth(buffer));
	if (firstWidth <= 0)
		return false;
	for (int c = ' '+1; c <= 0x7e; c++) {
		buffer[0] = c;
		int width = (int)ceilf(font.StringWidth(buffer));
		
		if (width != firstWidth)
			return false;
	}
	
	return true;
}


BMenu *
AppearancePrefView::_MakeFontMenu(uint32 command,
	const char *defaultFamily, const char *defaultStyle)
{
	BPopUpMenu *menu = new BPopUpMenu("");
	int32 numFamilies = count_font_families();
	uint32 flags;
	
	for (int32 i = 0; i < numFamilies; i++) {
		font_family family;
		if (get_font_family(i, &family, &flags) == B_OK) {
			BFont font;
			font_style style;
			int32 numStyles = count_font_styles(family);
			for (int32 j = 0; j < numStyles; j++) {
				if (get_font_style(family, j, &style) == B_OK) {
					font.SetFamilyAndStyle(family, style);
					if (IsFontUsable(font)) {
						BMessage *message = new BMessage(command);
						message->AddString("font_family", family);
						message->AddString("font_style", style);
						char itemLabel[134];
						snprintf(itemLabel, sizeof(itemLabel),
							"%s - %s", family, style);
						BMenuItem *item = new BMenuItem(itemLabel,
							message);
						menu->AddItem(item);
						if (!strcmp(defaultFamily, family)
							&& !strcmp(defaultStyle, style))
							item->SetMarked(true);
					}
				}
			}
		}
	}
	
	if (menu->FindMarked() == NULL)
		menu->ItemAt(0)->SetMarked(true);

	return menu;
}


BMenu *
AppearancePrefView::_MakeSizeMenu(uint32 command, uint8 defaultSize)
{
	BPopUpMenu *menu = new BPopUpMenu("size");
	int32 sizes[] = {9, 10, 11, 12, 14, 16, 18, 0};

	bool found = false;

	for (uint32 i = 0; sizes[i]; i++) {
		BString string;
		string << sizes[i];

		BMenuItem *item = new BMenuItem(string.String(), new BMessage(command));
		menu->AddItem(item);

		if (sizes[i] == defaultSize) {
			item->SetMarked(true);
			found = true;
		}
	}
	if (!found) {
		for (uint32 i = 0; sizes[i]; i++) {
			if (sizes[i] > defaultSize) {
				BString string;
				string << defaultSize;
				BMenuItem *item = new BMenuItem(string.String(), new BMessage(command));
				item->SetMarked(true);
				menu->AddItem(item, i);
				break;
			}
		}
	}

	return menu;
}
