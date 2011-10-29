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


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Terminal AppearancePrefView"


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
		B_TRANSLATE("Text"),
		B_TRANSLATE("Background"),
		B_TRANSLATE("Selected text"),
		B_TRANSLATE("Selected background"),
		NULL
	};

	fWarnOnExit = new BCheckBox(
		B_TRANSLATE("Confirm exit if active programs exist"),
			new BMessage(MSG_WARN_ON_EXIT_CHANGED));

	BMenu* fontMenu = _MakeFontMenu(MSG_HALF_FONT_CHANGED,
		PrefHandler::Default()->getString(PREF_HALF_FONT_FAMILY),
		PrefHandler::Default()->getString(PREF_HALF_FONT_STYLE));

	BMenu* sizeMenu = _MakeSizeMenu(MSG_HALF_SIZE_CHANGED,
		PrefHandler::Default()->getInt32(PREF_HALF_FONT_SIZE));

	fFont = new BMenuField(B_TRANSLATE("Font:"), fontMenu);
	fFontSize = new BMenuField(B_TRANSLATE("Size:"), sizeMenu);

	BPopUpMenu* schemasPopUp = _MakeColorSchemaMenu(MSG_COLOR_SCHEMA_CHANGED,
		gPredefinedSchemas, gPredefinedSchemas[0]);
	fColorSchemaField = new BMenuField(B_TRANSLATE("Color schema:"),
		schemasPopUp);

	BPopUpMenu* colorsPopUp = _MakeMenu(MSG_COLOR_FIELD_CHANGED, kColorTable,
		kColorTable[0]);

	fColorField = new BMenuField(B_TRANSLATE("Color:"),
			colorsPopUp);
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
			.Add(fFont->CreateLabelLayoutItem(), 0, 2)
			.Add(fFont->CreateMenuBarLayoutItem(), 1, 2)
			.Add(fFontSize->CreateLabelLayoutItem(), 0, 3)
			.Add(fFontSize->CreateMenuBarLayoutItem(), 1, 3)
			.Add(fColorSchemaField->CreateLabelLayoutItem(), 0, 4)
			.Add(fColorSchemaField->CreateMenuBarLayoutItem(), 1, 4)
			.Add(fColorField->CreateLabelLayoutItem(), 0, 5)
			.Add(fColorField->CreateMenuBarLayoutItem(), 1, 5)
			.End()
		.AddGlue()
		.Add(fColorControl = new BColorControl(BPoint(10, 10),
			B_CELLS_32x8, 8.0, "", new BMessage(MSG_COLOR_CHANGED)))
		.Add(fWarnOnExit);

	fTabTitle->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fWindowTitle->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fFont->SetAlignment(B_ALIGN_RIGHT);
	fFontSize->SetAlignment(B_ALIGN_RIGHT);
	fColorField->SetAlignment(B_ALIGN_RIGHT);
	fColorSchemaField->SetAlignment(B_ALIGN_RIGHT);

	fTabTitle->SetText(PrefHandler::Default()->getString(PREF_TAB_TITLE));
	fWindowTitle->SetText(PrefHandler::Default()->getString(PREF_WINDOW_TITLE));

	fColorControl->SetEnabled(false);
	fColorControl->SetValue(
		PrefHandler::Default()->getRGB(PREF_TEXT_FORE_COLOR));

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
	fTabTitle->SetText(PrefHandler::Default()->getString(PREF_TAB_TITLE));
	fWindowTitle->SetText(PrefHandler::Default()->getString(PREF_WINDOW_TITLE));

	fWarnOnExit->SetValue(PrefHandler::Default()->getBool(
		PREF_WARN_ON_EXIT));

	fColorSchemaField->Menu()->ItemAt(0)->SetMarked(true);
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
	fTabTitle->SetTarget(this);
	fWindowTitle->SetTarget(this);
	fWarnOnExit->SetTarget(this);

	fFontSize->Menu()->SetTargetForItems(this);
	fFont->Menu()->SetTargetForItems(this);

  	fColorControl->SetTarget(this);
  	fColorField->Menu()->SetTargetForItems(this);
  	fColorSchemaField->Menu()->SetTargetForItems(this);

  	_SetCurrentColorSchema(fColorSchemaField);
  	bool enableCustomColors =
		!strcmp(fColorSchemaField->Menu()->FindMarked()->Label(),
			gCustomSchema.name);

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

		case MSG_COLOR_CHANGED:
		{
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

		case MSG_COLOR_SCHEMA_CHANGED:
		{
			color_schema* newSchema = NULL;
			if (msg->FindPointer("color_schema",
				(void**)&newSchema) == B_OK) {

				if (newSchema == &gCustomSchema)
					_EnableCustomColors(true);
				else
					_EnableCustomColors(false);
				_ChangeColorSchema(newSchema);
				modified = true;
			}
			break;
		}

		case MSG_COLOR_FIELD_CHANGED:
			fColorControl->SetValue(PrefHandler::Default()->getRGB(
				fColorField->Menu()->FindMarked()->Label()));
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
AppearancePrefView::_ChangeColorSchema(color_schema* schema)
{
	PrefHandler* pref = PrefHandler::Default();

	pref->setRGB(PREF_TEXT_FORE_COLOR, schema->text_fore_color);
	pref->setRGB(PREF_TEXT_BACK_COLOR, schema->text_back_color);
	pref->setRGB(PREF_SELECT_FORE_COLOR, schema->select_fore_color);
	pref->setRGB(PREF_SELECT_BACK_COLOR, schema->select_back_color);
}


void
AppearancePrefView::_SetCurrentColorSchema(BMenuField* field)
{
	PrefHandler* pref = PrefHandler::Default();

	gCustomSchema.text_fore_color = pref->getRGB(PREF_TEXT_FORE_COLOR);
	gCustomSchema.text_back_color = pref->getRGB(PREF_TEXT_BACK_COLOR);
	gCustomSchema.select_fore_color = pref->getRGB(PREF_SELECT_FORE_COLOR);
	gCustomSchema.select_back_color = pref->getRGB(PREF_SELECT_BACK_COLOR);

	const char* currentSchemaName = NULL;

	color_schema** schemas = const_cast<color_schema**>(gPredefinedSchemas);
	while (*schemas) {
		if (gCustomSchema == **schemas) {
			currentSchemaName = (*schemas)->name;
			break;
		}
		schemas++;
	}

	bool found = false;
	for (int32 i = 0; i < fColorSchemaField->Menu()->CountItems(); i++) {
		BMenuItem* item = fColorSchemaField->Menu()->ItemAt(i);
		if (!strcmp(item->Label(), currentSchemaName)) {
			item->SetMarked(true);
			found = true;
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
				BMenuItem* item = new BMenuItem(string.String(),
					new BMessage(command));
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
		if (!strcmp((*items), ""))
			menu->AddSeparatorItem();
		else {
			BMessage* message = new BMessage(msg);
			menu->AddItem(new BMenuItem((*items), message));
		}

		items++;
		i++;
	}

	menu->FindItem(defaultItemName)->SetMarked(true);

	return menu;
}


/*static*/ BPopUpMenu*
AppearancePrefView::_MakeColorSchemaMenu(uint32 msg, const color_schema** items,
	const color_schema* defaultItemName)
{
	BPopUpMenu* menu = new BPopUpMenu("");

	int32 i = 0;
	while (*items) {
		if (!strcmp((*items)->name, ""))
			menu->AddSeparatorItem();
		else {
			BMessage* message = new BMessage(msg);
			message->AddPointer("color_schema", (const void*)*items);
			menu->AddItem(new BMenuItem((*items)->name, message));
		}

		items++;
		i++;
	}
	return menu;
}
