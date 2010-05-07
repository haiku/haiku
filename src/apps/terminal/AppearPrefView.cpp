/*
 * Copyright 2001-2010, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "AppearPrefView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Button.h>
#include <Catalog.h>
#include <ColorControl.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <View.h>

#include "PrefHandler.h"
#include "TermConst.h"


#undef TR_CONTEXT
#define TR_CONTEXT "Terminal AppearancePrefView"


static bool
IsFontUsable(const BFont& font)
{
	// TODO: If BFont::IsFullAndHalfFixed() was implemented, we could
	// use that. But I don't think it's easily implementable using
	// Freetype.

	if (font.IsFixed())
		return true;

	// manually check if all applicable chars are the same width
	char buffer[2] = { ' ', 0 };
	int firstWidth = (int)ceilf(font.StringWidth(buffer));

	// TODO: Workaround for broken fonts/font_subsystem
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


// #pragma mark -


AppearancePrefView::AppearancePrefView(const char* name,
		const BMessenger& messenger)
	:
	BView(name, B_WILL_DRAW),
	fTerminalMessenger(messenger)
{
  	const char* kColorTable[] = {
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

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	BMenu* fontMenu = _MakeFontMenu(MSG_HALF_FONT_CHANGED,
		PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY),
		PrefHandler::Default()->getString(PREF_HALF_FONT_STYLE));

	BMenu* sizeMenu = _MakeSizeMenu(MSG_HALF_SIZE_CHANGED,
		PrefHandler::Default()->getInt32(PREF_HALF_FONT_SIZE));

	fFont = new BMenuField(B_TRANSLATE("Font:"), fontMenu);
	fFontSize = new BMenuField(B_TRANSLATE("Size:"), sizeMenu);
	fColorField = new BMenuField(B_TRANSLATE("Color:"),
		_MakeMenu(MSG_COLOR_FIELD_CHANGED, kColorTable,
		kColorTable[0]));

	BView* layoutView = BLayoutBuilder::Group<>()
		.SetInsets(5, 5, 5, 5)
		.AddGroup(B_VERTICAL, 5)
			.Add(BGridLayoutBuilder(5, 5)
				.Add(fFont->CreateLabelLayoutItem(), 0, 0)
				.Add(fFont->CreateMenuBarLayoutItem(), 1, 0)
				.Add(fFontSize->CreateLabelLayoutItem(), 0, 1)
				.Add(fFontSize->CreateMenuBarLayoutItem(), 1, 1)
				.Add(fColorField->CreateLabelLayoutItem(), 0, 2)
				.Add(fColorField->CreateMenuBarLayoutItem(), 1, 2)
				)
			.AddGroup(B_VERTICAL, 5)
				.AddGlue()
				.Add(fColorControl = new BColorControl(BPoint(10, 10),
					B_CELLS_32x8, 8.0, "", new BMessage(MSG_COLOR_CHANGED)))
			.End()
		.End();

	AddChild(layoutView);

	fFont->SetAlignment(B_ALIGN_RIGHT);
	fFontSize->SetAlignment(B_ALIGN_RIGHT);
	fColorField->SetAlignment(B_ALIGN_RIGHT);

	fColorControl->SetValue(PrefHandler::Default()->getRGB(PREF_TEXT_FORE_COLOR));

	BTextControl* redInput = (BTextControl*)fColorControl->ChildAt(0);
	BTextControl* greenInput = (BTextControl*)fColorControl->ChildAt(1);
	BTextControl* blueInput = (BTextControl*)fColorControl->ChildAt(2);

	redInput->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	greenInput->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	blueInput->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
}


void
AppearancePrefView::GetPreferredSize(float* _width, float* _height)
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
	fColorControl->SetValue(PrefHandler::Default()->
		getRGB(PREF_TEXT_FORE_COLOR));

	fFont->Menu()->FindItem(PrefHandler::Default()->getString(
		PREF_HALF_FONT_FAMILY))->SetMarked(true);
	fFontSize->Menu()->FindItem(PrefHandler::Default()->getString(
		PREF_HALF_FONT_FAMILY))->SetMarked(true);
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
AppearancePrefView::MessageReceived(BMessage* msg)
{
	bool modified = false;

	switch (msg->what) {
		case MSG_HALF_FONT_CHANGED:
		{
			const char* family = NULL;
			const char* style = NULL;
			msg->FindString("font_family", &family);
			msg->FindString("font_style", &style);

			PrefHandler* pref = PrefHandler::Default();
			const char* currentFamily
				= pref->getString(PREF_HALF_FONT_FAMILY);
			const char* currentStyle
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
			BView::MessageReceived(msg);
			return;
	}

	if (modified) {
		fTerminalMessenger.SendMessage(msg);

		BMessenger messenger(this);
		messenger.SendMessage(MSG_PREF_MODIFIED);
	}
}


/*static*/ BMenu*
AppearancePrefView::_MakeFontMenu(uint32 command,
	const char* defaultFamily, const char* defaultStyle)
{
	BPopUpMenu* menu = new BPopUpMenu("");
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
						BMessage* message = new BMessage(command);
						message->AddString("font_family", family);
						message->AddString("font_style", style);
						char itemLabel[134];
						snprintf(itemLabel, sizeof(itemLabel),
							"%s - %s", family, style);
						BMenuItem* item = new BMenuItem(itemLabel,
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


/*static*/ BMenu*
AppearancePrefView::_MakeSizeMenu(uint32 command, uint8 defaultSize)
{
	BPopUpMenu* menu = new BPopUpMenu("size");
	int32 sizes[] = {9, 10, 11, 12, 14, 16, 18, 0};

	bool found = false;

	for (uint32 i = 0; sizes[i]; i++) {
		BString string;
		string << sizes[i];

		BMenuItem* item = new BMenuItem(string.String(), new BMessage(command));
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
				BMenuItem* item = new BMenuItem(string.String(), new BMessage(command));
				item->SetMarked(true);
				menu->AddItem(item, i);
				break;
			}
		}
	}

	return menu;
}


/*static*/ BPopUpMenu*
AppearancePrefView::_MakeMenu(uint32 msg, const char** items,
	const char* defaultItemName)
{
	BPopUpMenu* menu = new BPopUpMenu("");

	int32 i = 0;
	while (*items) {
		if (!strcmp(*items, ""))
			menu->AddSeparatorItem();
		else
			menu->AddItem(new BMenuItem(*items, new BMessage(msg)));
		if (!strcmp(*items, defaultItemName))
			menu->ItemAt(i)->SetMarked(true);

		items++;
		i++;
	}
	return menu;
}
