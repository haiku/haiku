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
#include <CheckBox.h>
#include <ColorControl.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <View.h>

#include "Colors.h"
#include "PrefHandler.h"
#include "TermConst.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal AppearancePrefView"


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

	for (int c = ' ' + 1; c <= 0x7e; c++) {
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
	BGroupView(name, B_VERTICAL, 5),
	fTerminalMessenger(messenger)
{
	const char* kColorTable[] = {
		B_TRANSLATE_MARK("Text"),
		B_TRANSLATE_MARK("Background"),
		B_TRANSLATE_MARK("Cursor"),
		B_TRANSLATE_MARK("Text under cursor"),
		B_TRANSLATE_MARK("Selected text"),
		B_TRANSLATE_MARK("Selected background"),
		NULL
	};

	fBlinkCursor = new BCheckBox(
		B_TRANSLATE("Blinking cursor"),
			new BMessage(MSG_BLINK_CURSOR_CHANGED));

	fWarnOnExit = new BCheckBox(
		B_TRANSLATE("Confirm exit if active programs exist"),
			new BMessage(MSG_WARN_ON_EXIT_CHANGED));

	BMenu* fontMenu = _MakeFontMenu(MSG_HALF_FONT_CHANGED,
		PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY),
		PrefHandler::Default()->getString(PREF_HALF_FONT_STYLE));
	fFontField = new BMenuField(B_TRANSLATE("Font:"), fontMenu);

	BPopUpMenu* schemesPopUp = _MakeColorSchemeMenu(MSG_COLOR_SCHEME_CHANGED,
		gPredefinedColorSchemes, gPredefinedColorSchemes[0]);
	fColorSchemeField = new BMenuField(B_TRANSLATE("Color scheme:"),
		schemesPopUp);

	BPopUpMenu* colorsPopUp = _MakeMenu(MSG_COLOR_FIELD_CHANGED, kColorTable,
		kColorTable[0]);

	fColorField = new BMenuField(B_TRANSLATE("Color:"), colorsPopUp);
	fColorField->SetEnabled(false);

	fTabTitle = new BTextControl("tabTitle", B_TRANSLATE("Tab title:"), "",
		NULL);
	fTabTitle->SetModificationMessage(
		new BMessage(MSG_TAB_TITLE_SETTING_CHANGED));
	fTabTitle->SetToolTip(BString(B_TRANSLATE(
		"The pattern specifying the tab titles. The following placeholders\n"
		"can be used:\n")) << kTooTipSetTabTitlePlaceholders);

	fWindowTitle = new BTextControl("windowTitle", B_TRANSLATE("Window title:"),
		"", NULL);
	fWindowTitle->SetModificationMessage(
		new BMessage(MSG_WINDOW_TITLE_SETTING_CHANGED));
	fWindowTitle->SetToolTip(BString(B_TRANSLATE(
		"The pattern specifying the window titles. The following placeholders\n"
		"can be used:\n")) << kTooTipSetWindowTitlePlaceholders);

	BLayoutBuilder::Group<>(this)
		.SetInsets(5, 5, 5, 5)
		.AddGrid(5, 5)
			.Add(fTabTitle->CreateLabelLayoutItem(), 0, 0)
			.Add(fTabTitle->CreateTextViewLayoutItem(), 1, 0)
			.Add(fWindowTitle->CreateLabelLayoutItem(), 0, 1)
			.Add(fWindowTitle->CreateTextViewLayoutItem(), 1, 1)
			.Add(fFontField->CreateLabelLayoutItem(), 0, 2)
			.Add(fFontField->CreateMenuBarLayoutItem(), 1, 2)
			.Add(fColorSchemeField->CreateLabelLayoutItem(), 0, 3)
			.Add(fColorSchemeField->CreateMenuBarLayoutItem(), 1, 3)
			.Add(fColorField->CreateLabelLayoutItem(), 0, 4)
			.Add(fColorField->CreateMenuBarLayoutItem(), 1, 4)
			.End()
		.AddGlue()
		.Add(fColorControl = new BColorControl(BPoint(10, 10),
			B_CELLS_32x8, 8.0, "", new BMessage(MSG_COLOR_CHANGED)))
		.Add(fBlinkCursor)
		.Add(fWarnOnExit);

	fTabTitle->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fWindowTitle->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fFontField->SetAlignment(B_ALIGN_RIGHT);
	fColorField->SetAlignment(B_ALIGN_RIGHT);
	fColorSchemeField->SetAlignment(B_ALIGN_RIGHT);

	fTabTitle->SetText(PrefHandler::Default()->getString(PREF_TAB_TITLE));
	fWindowTitle->SetText(PrefHandler::Default()->getString(PREF_WINDOW_TITLE));

	fColorControl->SetEnabled(false);
	fColorControl->SetValue(
		PrefHandler::Default()->getRGB(PREF_TEXT_FORE_COLOR));

	fBlinkCursor->SetValue(PrefHandler::Default()->getBool(PREF_BLINK_CURSOR));
	fWarnOnExit->SetValue(PrefHandler::Default()->getBool(PREF_WARN_ON_EXIT));

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
	PrefHandler* pref = PrefHandler::Default();

	fTabTitle->SetText(pref->getString(PREF_TAB_TITLE));
	fWindowTitle->SetText(pref->getString(PREF_WINDOW_TITLE));

	fWarnOnExit->SetValue(pref->getBool(
		PREF_WARN_ON_EXIT));

	fColorSchemeField->Menu()->ItemAt(0)->SetMarked(true);
	fColorControl->SetValue(pref->
		getRGB(PREF_TEXT_FORE_COLOR));

	const char* family = pref->getString(PREF_HALF_FONT_FAMILY);
	const char* style = pref->getString(PREF_HALF_FONT_STYLE);
	const char* size = pref->getString(PREF_HALF_FONT_SIZE);

	_MarkSelectedFont(family, style, size);
}


void
AppearancePrefView::AttachedToWindow()
{
	fTabTitle->SetTarget(this);
	fWindowTitle->SetTarget(this);
	fBlinkCursor->SetTarget(this);
	fWarnOnExit->SetTarget(this);

	fFontField->Menu()->SetTargetForItems(this);
	for (int32 i = 0; i < fFontField->Menu()->CountItems(); i++) {
		BMenu* fontSizeMenu = fFontField->Menu()->SubmenuAt(i);
		if (fontSizeMenu == NULL)
			continue;

		fontSizeMenu->SetTargetForItems(this);
	}

  	fColorControl->SetTarget(this);
  	fColorField->Menu()->SetTargetForItems(this);
  	fColorSchemeField->Menu()->SetTargetForItems(this);

  	_SetCurrentColorScheme(fColorSchemeField);
  	bool enableCustomColors =
		strcmp(fColorSchemeField->Menu()->FindMarked()->Label(),
			gCustomColorScheme.name) == 0;

  	_EnableCustomColors(enableCustomColors);
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
			const char* size = NULL;
			if (msg->FindString("font_family", &family) != B_OK
				|| msg->FindString("font_style", &style) != B_OK
				|| msg->FindString("font_size", &size) != B_OK) {
				break;
			}

			PrefHandler* pref = PrefHandler::Default();
			const char* currentFamily
				= pref->getString(PREF_HALF_FONT_FAMILY);
			const char* currentStyle
				= pref->getString(PREF_HALF_FONT_STYLE);
			const char* currentSize
				= pref->getString(PREF_HALF_FONT_SIZE);

			if (currentFamily == NULL || strcmp(currentFamily, family) != 0
				|| currentStyle == NULL || strcmp(currentStyle, style) != 0
				|| currentSize == NULL || strcmp(currentSize, size) != 0) {
				pref->setString(PREF_HALF_FONT_FAMILY, family);
				pref->setString(PREF_HALF_FONT_STYLE, style);
				pref->setString(PREF_HALF_FONT_SIZE, size);
				_MarkSelectedFont(family, style, size);
				modified = true;
			}
			break;
		}

		case MSG_COLOR_CHANGED:
		{
			const BMessage* itemMessage
				= fColorField->Menu()->FindMarked()->Message();
			const char* label = NULL;
			if (itemMessage->FindString("label", &label) != B_OK)
				break;
			rgb_color oldColor = PrefHandler::Default()->getRGB(label);
			if (oldColor != fColorControl->ValueAsColor()) {
				PrefHandler::Default()->setRGB(label,
					fColorControl->ValueAsColor());
				modified = true;
			}
			break;
		}

		case MSG_COLOR_SCHEME_CHANGED:
		{
			color_scheme* newScheme = NULL;
			if (msg->FindPointer("color_scheme",
					(void**)&newScheme) == B_OK) {
				if (newScheme == &gCustomColorScheme)
					_EnableCustomColors(true);
				else
					_EnableCustomColors(false);

				_ChangeColorScheme(newScheme);
				modified = true;
			}
			break;
		}

		case MSG_COLOR_FIELD_CHANGED:
		{
			const char* label = NULL;
			if (msg->FindString("label", &label) == B_OK)
				fColorControl->SetValue(PrefHandler::Default()->getRGB(label));
			break;
		}

		case MSG_BLINK_CURSOR_CHANGED:
			if (PrefHandler::Default()->getBool(PREF_BLINK_CURSOR)
				!= fBlinkCursor->Value()) {
					PrefHandler::Default()->setBool(PREF_BLINK_CURSOR,
						fBlinkCursor->Value());
					modified = true;
			}
			break;

		case MSG_WARN_ON_EXIT_CHANGED:
			if (PrefHandler::Default()->getBool(PREF_WARN_ON_EXIT)
				!= fWarnOnExit->Value()) {
					PrefHandler::Default()->setBool(PREF_WARN_ON_EXIT,
						fWarnOnExit->Value());
					modified = true;
			}
			break;

		case MSG_TAB_TITLE_SETTING_CHANGED:
		{
			BString oldValue(PrefHandler::Default()->getString(PREF_TAB_TITLE));
			if (oldValue != fTabTitle->Text()) {
				PrefHandler::Default()->setString(PREF_TAB_TITLE,
					fTabTitle->Text());
				modified = true;
			}
			break;
		}

		case MSG_WINDOW_TITLE_SETTING_CHANGED:
		{
			BString oldValue(PrefHandler::Default()->getString(
				PREF_WINDOW_TITLE));
			if (oldValue != fWindowTitle->Text()) {
				PrefHandler::Default()->setString(PREF_WINDOW_TITLE,
					fWindowTitle->Text());
				modified = true;
			}
			break;
		}

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


void
AppearancePrefView::_EnableCustomColors(bool enable)
{
	fColorField->SetEnabled(enable);
	fColorControl->SetEnabled(enable);
}


void
AppearancePrefView::_ChangeColorScheme(color_scheme* scheme)
{
	PrefHandler* pref = PrefHandler::Default();

	pref->setRGB(PREF_TEXT_FORE_COLOR, scheme->text_fore_color);
	pref->setRGB(PREF_TEXT_BACK_COLOR, scheme->text_back_color);
	pref->setRGB(PREF_SELECT_FORE_COLOR, scheme->select_fore_color);
	pref->setRGB(PREF_SELECT_BACK_COLOR, scheme->select_back_color);
	pref->setRGB(PREF_CURSOR_FORE_COLOR, scheme->cursor_fore_color);
	pref->setRGB(PREF_CURSOR_BACK_COLOR, scheme->cursor_back_color);
}


void
AppearancePrefView::_SetCurrentColorScheme(BMenuField* field)
{
	PrefHandler* pref = PrefHandler::Default();

	gCustomColorScheme.text_fore_color = pref->getRGB(PREF_TEXT_FORE_COLOR);
	gCustomColorScheme.text_back_color = pref->getRGB(PREF_TEXT_BACK_COLOR);
	gCustomColorScheme.select_fore_color = pref->getRGB(PREF_SELECT_FORE_COLOR);
	gCustomColorScheme.select_back_color = pref->getRGB(PREF_SELECT_BACK_COLOR);
	gCustomColorScheme.cursor_fore_color = pref->getRGB(PREF_CURSOR_FORE_COLOR);
	gCustomColorScheme.cursor_back_color = pref->getRGB(PREF_CURSOR_BACK_COLOR);

	const char* currentSchemeName = NULL;

	for (const color_scheme** schemes = gPredefinedColorSchemes;
			*schemes != NULL; schemes++) {
		if (gCustomColorScheme == **schemes) {
			currentSchemeName = (*schemes)->name;
			break;
		}
	}

	for (int32 i = 0; i < fColorSchemeField->Menu()->CountItems(); i++) {
		BMenuItem* item = fColorSchemeField->Menu()->ItemAt(i);
		if (strcmp(item->Label(), currentSchemeName) == 0) {
			item->SetMarked(true);
			break;
		}
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
						const char* size
							= PrefHandler::Default()->getString(PREF_HALF_FONT_SIZE);
						message->AddString("font_family", family);
						message->AddString("font_style", style);
						message->AddString("font_size", size);
						char fontMenuLabel[134];
						snprintf(fontMenuLabel, sizeof(fontMenuLabel),
							"%s - %s", family, style);
						BMenu* fontSizeMenu = _MakeFontSizeMenu(fontMenuLabel,
							MSG_HALF_FONT_CHANGED, family, style, size);
						BMenuItem* item = new BMenuItem(fontSizeMenu, message);
						menu->AddItem(item);
						if (strcmp(defaultFamily, family) == 0
							&& strcmp(defaultStyle, style) == 0)
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
AppearancePrefView::_MakeFontSizeMenu(const char* label, uint32 command,
	const char* family, const char* style, const char* size)
{
	BMenu* menu = new BMenu(label);
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(false);

	int32 sizes[] = {
		8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36, 0
	};

	bool found = false;

	for (uint32 i = 0; sizes[i]; i++) {
		BString fontSize;
		fontSize << sizes[i];
		BMessage* message = new BMessage(command);
		message->AddString("font_family", family);
		message->AddString("font_style", style);
		message->AddString("font_size", fontSize.String());
		BMenuItem* item = new BMenuItem(fontSize.String(), message);
		menu->AddItem(item);
		if (sizes[i] == atoi(size)) {
			item->SetMarked(true);
			found = true;
		}
	}

	if (!found) {
		for (uint32 i = 0; sizes[i]; i++) {
			if (sizes[i] > atoi(size)) {
				BMessage* message = new BMessage(command);
				message->AddString("font_family", family);
				message->AddString("font_style", style);
				message->AddString("font_size", size);
				BMenuItem* item = new BMenuItem(size, message);
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

	while (*items) {
		if (strcmp((*items), "") == 0)
			menu->AddSeparatorItem();
		else {
			BMessage* message = new BMessage(msg);
			message->AddString("label", *items);
			BMenuItem* item = new BMenuItem(B_TRANSLATE(*items), message);
			menu->AddItem(item);
			if (strcmp(*items, defaultItemName) == 0)
				item->SetMarked(true);
		}

		items++;
	}

	return menu;
}


/*static*/ BPopUpMenu*
AppearancePrefView::_MakeColorSchemeMenu(uint32 msg, const color_scheme** items,
	const color_scheme* defaultItemName)
{
	BPopUpMenu* menu = new BPopUpMenu("");

	int32 i = 0;
	while (*items) {
		if (strcmp((*items)->name, "") == 0)
			menu->AddSeparatorItem();
		else {
			BMessage* message = new BMessage(msg);
			message->AddPointer("color_scheme", (const void*)*items);
			menu->AddItem(new BMenuItem((*items)->name, message));
		}

		items++;
		i++;
	}
	return menu;
}


void
AppearancePrefView::_MarkSelectedFont(const char* family, const char* style,
	const char* size)
{
	char fontMenuLabel[134];
	snprintf(fontMenuLabel, sizeof(fontMenuLabel), "%s - %s", family, style);

	// mark the selected font
	BMenuItem* selectedFont = fFontField->Menu()->FindItem(fontMenuLabel);
	if (selectedFont != NULL)
		selectedFont->SetMarked(true);

	// mark the selected font size on all font menus
	for (int32 i = 0; i < fFontField->Menu()->CountItems(); i++) {
		BMenu* fontSizeMenu = fFontField->Menu()->SubmenuAt(i);
		if (fontSizeMenu == NULL)
			continue;

		BMenuItem* item = fontSizeMenu->FindItem(size);
		if (item != NULL)
			item->SetMarked(true);
	}
}
