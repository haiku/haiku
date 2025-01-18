/*
 * Copyright 2022-2025 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ThemeView.h"

#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <ColorItem.h>
#include <ColorListView.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <SpaceLayoutItem.h>

#include "ThemeWindow.h"
#include "Colors.h"
#include "TermConst.h"
#include "PrefHandler.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal ThemeView"


#define COLOR_DROPPED 'cldp'
#define DECORATOR_CHANGED 'dcch'


using BPrivate::BColorItem;
using BPrivate::BColorListView;
using BPrivate::BColorPreview;


/* static */ const char*
ThemeView::kColorTable[] = {
	B_TRANSLATE_MARK("Text"),
	B_TRANSLATE_MARK("Background"),
	B_TRANSLATE_MARK("Cursor"),
	B_TRANSLATE_MARK("Text under cursor"),
	B_TRANSLATE_MARK("Selected text"),
	B_TRANSLATE_MARK("Selected background"),
	B_TRANSLATE_MARK("ANSI black color"),
	B_TRANSLATE_MARK("ANSI red color"),
	B_TRANSLATE_MARK("ANSI green color"),
	B_TRANSLATE_MARK("ANSI yellow color"),
	B_TRANSLATE_MARK("ANSI blue color"),
	B_TRANSLATE_MARK("ANSI magenta color"),
	B_TRANSLATE_MARK("ANSI cyan color"),
	B_TRANSLATE_MARK("ANSI white color"),
	B_TRANSLATE_MARK("ANSI bright black color"),
	B_TRANSLATE_MARK("ANSI bright red color"),
	B_TRANSLATE_MARK("ANSI bright green color"),
	B_TRANSLATE_MARK("ANSI bright yellow color"),
	B_TRANSLATE_MARK("ANSI bright blue color"),
	B_TRANSLATE_MARK("ANSI bright magenta color"),
	B_TRANSLATE_MARK("ANSI bright cyan color"),
	B_TRANSLATE_MARK("ANSI bright white color"),
	NULL
};


ThemeView::ThemeView(const char* name, const BMessenger& messenger)
	:
	BGroupView(name, B_VERTICAL, 5),
	fTerminalMessenger(messenger)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// Set up list of color attributes
	fAttrList = new BColorListView("AttributeList");

	fScrollView = new BScrollView("ScrollView", fAttrList, 0, false, true);
	fScrollView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	PrefHandler* prefHandler = PrefHandler::Default();

	for (const char** table = kColorTable; *table != NULL; ++table) {
		fAttrList->AddItem(
			new BColorItem(B_TRANSLATE_NOCOLLECT(*table), prefHandler->getRGB(*table)));
	}

	fColorSchemeMenu = new BPopUpMenu("");
	fColorSchemeField = new BMenuField(B_TRANSLATE("Color scheme:"), fColorSchemeMenu);

	fColorPreview = new BColorPreview("color preview", "", new BMessage(COLOR_DROPPED));

	fPicker = new BColorControl(B_ORIGIN, B_CELLS_32x8, 8.0,
		"picker", new BMessage(MSG_UPDATE_COLOR));

	fPreview = new BTextView("preview");

	BLayoutBuilder::Group<>(this)
		.AddGrid()
			.Add(fColorSchemeField->CreateLabelLayoutItem(), 0, 5)
			.Add(fColorSchemeField->CreateMenuBarLayoutItem(), 1, 5)
			.End()
		.Add(fScrollView, 10.0)
		.Add(fPreview)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fColorPreview)
			.AddGlue()
			.Add(fPicker);

	fColorPreview->Parent()->SetExplicitMaxSize(BSize(B_SIZE_UNSET, fPicker->Bounds().Height()));
	fAttrList->SetSelectionMessage(new BMessage(MSG_COLOR_ATTRIBUTE_CHOSEN));
	float minHeight = be_control_look->DefaultItemSpacing() * 20;
	fScrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, minHeight));
	fColorSchemeField->SetAlignment(B_ALIGN_RIGHT);

	_MakeColorSchemeMenu();

	_UpdateStyle();

	InvalidateLayout(true);
}


ThemeView::~ThemeView()
{
}


void
ThemeView::_UpdateStyle()
{
	PrefHandler* prefHandler = PrefHandler::Default();

	const char* colours[] = {
		B_TRANSLATE("black"),
		B_TRANSLATE("red"),
		B_TRANSLATE("green"),
		B_TRANSLATE("yellow"),
		B_TRANSLATE("blue"),
		B_TRANSLATE("magenta"),
		B_TRANSLATE("cyan"),
		B_TRANSLATE("white"),
	};

	text_run_array *array = (text_run_array*)malloc(sizeof(text_run_array)
			+ sizeof(text_run) * 16);
	int offset = 1;
	int index = 0;
	array->count = 16;
	array->runs[0].offset = offset;
	array->runs[0].font = *be_fixed_font;
	array->runs[0].color = prefHandler->getRGB(PREF_ANSI_BLACK_COLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[1].offset = offset;
	array->runs[1].font = *be_fixed_font;
	array->runs[1].color = prefHandler->getRGB(PREF_ANSI_RED_COLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[2].offset = offset;
	array->runs[2].font = *be_fixed_font;
	array->runs[2].color = prefHandler->getRGB(PREF_ANSI_GREEN_COLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[3].offset = offset;
	array->runs[3].font = *be_fixed_font;
	array->runs[3].color = prefHandler->getRGB(PREF_ANSI_YELLOW_COLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[4].offset = offset;
	array->runs[4].font = *be_fixed_font;
	array->runs[4].color = prefHandler->getRGB(PREF_ANSI_BLUE_COLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[5].offset = offset;
	array->runs[5].font = *be_fixed_font;
	array->runs[5].color = prefHandler->getRGB(PREF_ANSI_MAGENTA_COLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[6].offset = offset;
	array->runs[6].font = *be_fixed_font;
	array->runs[6].color = prefHandler->getRGB(PREF_ANSI_CYAN_COLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[7].offset = offset;
	array->runs[7].font = *be_fixed_font;
	array->runs[7].color = prefHandler->getRGB(PREF_ANSI_WHITE_COLOR);
	offset += strlen(colours[index++]) + 1;
	index = 0;
	offset++;
	array->runs[8].offset = offset;
	array->runs[8].font = *be_fixed_font;
	array->runs[8].color = prefHandler->getRGB(PREF_ANSI_BLACK_HCOLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[9].offset = offset;
	array->runs[9].font = *be_fixed_font;
	array->runs[9].color = prefHandler->getRGB(PREF_ANSI_RED_HCOLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[10].offset = offset;
	array->runs[10].font = *be_fixed_font;
	array->runs[10].color = prefHandler->getRGB(PREF_ANSI_GREEN_HCOLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[11].offset = offset;
	array->runs[11].font = *be_fixed_font;
	array->runs[11].color = prefHandler->getRGB(PREF_ANSI_YELLOW_HCOLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[12].offset = offset;
	array->runs[12].font = *be_fixed_font;
	array->runs[12].color = prefHandler->getRGB(PREF_ANSI_BLUE_HCOLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[13].offset = offset;
	array->runs[13].font = *be_fixed_font;
	array->runs[13].color = prefHandler->getRGB(PREF_ANSI_MAGENTA_HCOLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[14].offset = offset;
	array->runs[14].font = *be_fixed_font;
	array->runs[14].color = prefHandler->getRGB(PREF_ANSI_CYAN_HCOLOR);
	offset += strlen(colours[index++]) + 1;
	array->runs[15].offset = offset;
	array->runs[15].font = *be_fixed_font;
	array->runs[15].color = prefHandler->getRGB(PREF_ANSI_WHITE_HCOLOR);

	fPreview->SetStylable(true);
	fPreview->MakeEditable(false);
	fPreview->MakeSelectable(false);
	fPreview->SetViewColor(prefHandler->getRGB(PREF_TEXT_BACK_COLOR));

	BString previewText;
	previewText << '\n';
	for (int ix = 0; ix < 8; ++ix) {
		previewText << ' ' << colours[ix];
	}
	previewText << '\n';
	for (int ix = 0; ix < 8; ++ix) {
		previewText << ' ' << colours[ix];
	}
	previewText << '\n';

	fPreview->SetAlignment(B_ALIGN_CENTER);
	fPreview->SetText(previewText.String(), array);
	font_height height;
	be_fixed_font->GetHeight(&height);
	fPreview->SetExplicitMinSize(BSize(B_SIZE_UNSET, (height.ascent + height.descent) * 5));
}


void
ThemeView::AttachedToWindow()
{
	fPicker->SetTarget(this);
	fAttrList->SetTarget(this);
	fColorPreview->SetTarget(this);
	fColorSchemeField->Menu()->SetTargetForItems(this);

	fAttrList->Select(0);
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	_SetCurrentColorScheme();
}


void
ThemeView::WindowActivated(bool active)
{
	if (!active)
		return;

	UpdateMenu();
}


void
ThemeView::UpdateMenu()
{
	PrefHandler::Default()->LoadThemes();

	_MakeColorSchemeMenu();
	_SetCurrentColorScheme();

	fColorSchemeField->Menu()->SetTargetForItems(this);
}


void
ThemeView::MessageReceived(BMessage* message)
{
	bool modified = false;

	if (message->WasDropped()) {
		char* name;
		type_code type;
		rgb_color* color;
		ssize_t size;
		if (message->GetInfo(B_RGB_COLOR_TYPE, 0, &name, &type) == B_OK
			&& message->FindData(name, type, (const void**)&color, &size) == B_OK) {
			_SetCurrentColor(*color);
			modified = true;
		}
	}

	switch (message->what) {
		case MSG_COLOR_SCHEME_CHANGED:
		{
			color_scheme* newScheme = NULL;
			if (message->FindPointer("color_scheme", (void**)&newScheme) == B_OK) {
				_ChangeColorScheme(newScheme);
				modified = true;
			}
			break;
		}

		case BColorListView::B_MESSAGE_SET_CURRENT_COLOR:
		case BColorListView::B_MESSAGE_SET_COLOR:
		{
			// Received from color list view when color changes
			char* name;
			type_code type;
			rgb_color* color;
			ssize_t size;
			if (message->GetInfo(B_RGB_COLOR_TYPE, 0, &name, &type) == B_OK
				&& message->FindData(name, type, (const void**)&color, &size) == B_OK) {
				bool current = message->GetBool("current", false);
				if (current)
					_SetCurrentColor(*color);
				else
					_SetColor(name, *color);
				modified = true;
			}
			break;
		}

		case MSG_UPDATE_COLOR:
		{
			// Received from the color fPicker when its color changes
			rgb_color color = fPicker->ValueAsColor();
			_SetCurrentColor(color);
			modified = true;

			break;
		}

		case MSG_COLOR_ATTRIBUTE_CHOSEN:
		{
			// Received when the user chooses a GUI fAttribute from the list
			const int32 currentIndex = fAttrList->CurrentSelection();
			if (currentIndex < 0)
				break;

			rgb_color color = PrefHandler::Default()->getRGB(kColorTable[currentIndex]);
			_SetCurrentColor(color);
			break;
		}

		case MSG_THEME_MODIFIED:
			_SetCurrentColorScheme();
			BView::MessageReceived(message);
			return;

		default:
			BView::MessageReceived(message);
			break;
	}

	if (modified) {
		_UpdateStyle();
		fTerminalMessenger.SendMessage(message);

		BMessenger messenger(this);
		messenger.SendMessage(MSG_THEME_MODIFIED);
	}
}


void
ThemeView::SetDefaults()
{
	PrefHandler* prefHandler = PrefHandler::Default();

	int32 count = fAttrList->CountItems();
	for (int32 index = 0; index < count; ++index) {
		BColorItem* item = static_cast<BColorItem*>(fAttrList->ItemAt(index));
		item->SetColor(prefHandler->getRGB(kColorTable[index]));
		fAttrList->InvalidateItem(index);
	}

	int32 currentIndex = fAttrList->CurrentSelection();
	BColorItem* item = static_cast<BColorItem*>(fAttrList->ItemAt(currentIndex));
	if (item != NULL) {
		rgb_color color = item->Color();
		fPicker->SetValue(color);
		fColorPreview->SetColor(color);
		fColorPreview->Invalidate();
	}

	_UpdateStyle();

	BMessage message(MSG_COLOR_SCHEME_CHANGED);
	fTerminalMessenger.SendMessage(&message);
}


void
ThemeView::Revert()
{
	_SetCurrentColorScheme();

	SetDefaults();
}


void
ThemeView::_ChangeColorScheme(color_scheme* scheme)
{
	PrefHandler* pref = PrefHandler::Default();

	pref->setRGB(PREF_TEXT_FORE_COLOR, scheme->text_fore_color);
	pref->setRGB(PREF_TEXT_BACK_COLOR, scheme->text_back_color);
	pref->setRGB(PREF_SELECT_FORE_COLOR, scheme->select_fore_color);
	pref->setRGB(PREF_SELECT_BACK_COLOR, scheme->select_back_color);
	pref->setRGB(PREF_CURSOR_FORE_COLOR, scheme->cursor_fore_color);
	pref->setRGB(PREF_CURSOR_BACK_COLOR, scheme->cursor_back_color);
	pref->setRGB(PREF_ANSI_BLACK_COLOR, scheme->ansi_colors.black);
	pref->setRGB(PREF_ANSI_RED_COLOR, scheme->ansi_colors.red);
	pref->setRGB(PREF_ANSI_GREEN_COLOR, scheme->ansi_colors.green);
	pref->setRGB(PREF_ANSI_YELLOW_COLOR, scheme->ansi_colors.yellow);
	pref->setRGB(PREF_ANSI_BLUE_COLOR, scheme->ansi_colors.blue);
	pref->setRGB(PREF_ANSI_MAGENTA_COLOR, scheme->ansi_colors.magenta);
	pref->setRGB(PREF_ANSI_CYAN_COLOR, scheme->ansi_colors.cyan);
	pref->setRGB(PREF_ANSI_WHITE_COLOR, scheme->ansi_colors.white);
	pref->setRGB(PREF_ANSI_BLACK_HCOLOR, scheme->ansi_colors_h.black);
	pref->setRGB(PREF_ANSI_RED_HCOLOR, scheme->ansi_colors_h.red);
	pref->setRGB(PREF_ANSI_GREEN_HCOLOR, scheme->ansi_colors_h.green);
	pref->setRGB(PREF_ANSI_YELLOW_HCOLOR, scheme->ansi_colors_h.yellow);
	pref->setRGB(PREF_ANSI_BLUE_HCOLOR, scheme->ansi_colors_h.blue);
	pref->setRGB(PREF_ANSI_MAGENTA_HCOLOR, scheme->ansi_colors_h.magenta);
	pref->setRGB(PREF_ANSI_CYAN_HCOLOR, scheme->ansi_colors_h.cyan);
	pref->setRGB(PREF_ANSI_WHITE_HCOLOR, scheme->ansi_colors_h.white);

	int32 count = fAttrList->CountItems();
	for (int32 index = 0; index < count; ++index) {
		BColorItem* item = static_cast<BColorItem*>(fAttrList->ItemAt(index));
		rgb_color color = pref->getRGB(kColorTable[index]);
		item->SetColor(color);

		if (item->IsSelected()) {
			fPicker->SetValue(color);
			fColorPreview->SetColor(color);
			fColorPreview->Invalidate();
		}
	}

	fAttrList->Invalidate();
}


void
ThemeView::_SetCurrentColorScheme()
{
	PrefHandler* pref = PrefHandler::Default();

	pref->LoadColorScheme(&gCustomColorScheme);

	const char* currentSchemeName = NULL;

	int32 i = 0;
	while (i < gColorSchemes->CountItems()) {
		const color_scheme *item = gColorSchemes->ItemAt(i);
		i++;

		if (gCustomColorScheme == *item) {
			currentSchemeName = item->name;
			break;
		}
	}

	// If the scheme is not one of the known ones, assume a custom one.
	if (currentSchemeName == NULL)
		currentSchemeName = "Custom";

	for (int32 i = 0; i < fColorSchemeField->Menu()->CountItems(); i++) {
		BMenuItem* item = fColorSchemeField->Menu()->ItemAt(i);
		if (strcmp(item->Label(), currentSchemeName) == 0) {
			item->SetMarked(true);
			break;
		}
	}
}


void
ThemeView::_MakeColorSchemeMenuItem(const color_scheme *item)
{
	if (item == NULL)
		return;

	BMessage* message = new BMessage(MSG_COLOR_SCHEME_CHANGED);
	message->AddPointer("color_scheme", (const void*)item);
	fColorSchemeMenu->AddItem(new BMenuItem(item->name, message));
}


void
ThemeView::_MakeColorSchemeMenu()
{
	while (fColorSchemeMenu->CountItems() > 0)
		delete(fColorSchemeMenu->RemoveItem((int32)0));

	FindColorSchemeByName comparator("Default");

	const color_scheme* defaultItem = gColorSchemes->FindIf(comparator);

	_MakeColorSchemeMenuItem(defaultItem);

	int32 index = 0;
	while (index < gColorSchemes->CountItems()) {
		const color_scheme *item = gColorSchemes->ItemAt(index);
		index++;

		if (strcmp(item->name, "") == 0)
			fColorSchemeMenu->AddSeparatorItem();
		else if (item == defaultItem)
			continue;
		else
			_MakeColorSchemeMenuItem(item);
	}

	// Add the custom item at the very end
	fColorSchemeMenu->AddSeparatorItem();
	_MakeColorSchemeMenuItem(&gCustomColorScheme);
}


void
ThemeView::_SetCurrentColor(rgb_color color)
{
	int32 currentIndex = fAttrList->CurrentSelection();
	BColorItem* item = static_cast<BColorItem*>(fAttrList->ItemAt(currentIndex));
	if (item != NULL) {
		item->SetColor(color);
		fAttrList->InvalidateItem(currentIndex);

		PrefHandler::Default()->setRGB(kColorTable[currentIndex], color);
	}

	fPicker->SetValue(color);
	fColorPreview->SetColor(color);
}


void
ThemeView::_SetColor(const char* name, rgb_color color)
{
	PrefHandler::Default()->setRGB(name, color);

	_UpdateStyle();
}
