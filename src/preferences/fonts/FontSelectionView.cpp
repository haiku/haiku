/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "FontSelectionView.h"
#include "MainWindow.h"

#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>

#include <stdio.h>


#define INSTANT_UPDATE
	// if defined, the system font will be updated immediately, and not
	// only on exit

static const int32 kMsgSetFamily = 'fmly';
static const int32 kMsgSetStyle = 'styl';
static const int32 kMsgSetSize = 'size';

static const float kMinSize = 8.0;
static const float kMaxSize = 18.0;


// private font API
extern void _set_system_font_(const char *which, font_family family,
	font_style style, float size);
extern status_t _get_system_default_font_(const char* which,
	font_family family, font_style style, float* _size);


#ifdef B_BEOS_VERSION_DANO
// this call only exists under R5
void
_set_system_font_(const char *which, font_family family,
	font_style style, float size)
{
	puts("you don't have _set_system_font_()");
}
#endif

#if !defined(HAIKU_TARGET_PLATFORM_HAIKU) && !defined(HAIKU_TARGET_PLATFORM_LIBBE_TEST)
// this call only exists under Haiku (and the test environment)
status_t
_get_system_default_font_(const char* which, font_family family,
	font_style style, float* _size)
{
	puts("you don't have _get_system_default_font_()");
	return B_ERROR;
}
#endif


//	#pragma mark -


FontSelectionView::FontSelectionView(BRect _rect, const char* name,
	const char* label, const BFont& currentFont)
	: BView(_rect, name, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
	fSavedFont(currentFont),
	fCurrentFont(currentFont)
{
	fDivider = StringWidth(label) + 5;

	fSizesMenu = new BPopUpMenu("size menu");
	_BuildSizesMenu();

	fFontsMenu = new BPopUpMenu("font menu");

	// font menu
	BRect rect(Bounds());
	fFontsMenuField = new BMenuField(rect, "fonts", label, fFontsMenu, false);
	fFontsMenuField->SetDivider(fDivider);
	fFontsMenuField->SetAlignment(B_ALIGN_RIGHT);
	fFontsMenuField->ResizeToPreferred();
	AddChild(fFontsMenuField);

	// size menu
	rect.right = rect.left + StringWidth("Size: 99") + 30.0f;
	fSizesMenuField = new BMenuField(rect, "sizes", "Size:", fSizesMenu, true,
		B_FOLLOW_TOP);
	fSizesMenuField->SetDivider(StringWidth(fSizesMenuField->Label()) + 5.0f);
	fSizesMenuField->SetAlignment(B_ALIGN_RIGHT);
	fSizesMenuField->ResizeToPreferred();
	rect = Bounds();
	fSizesMenuField->MoveBy(rect.right - fSizesMenuField->Bounds().Width(), 0);
	AddChild(fSizesMenuField);

	rect.top += fFontsMenuField->Bounds().Height() + 5;
	rect.left = fDivider;
	BFont font = be_plain_font;
	font.SetSize(kMaxSize);
	font_height height;
	font.GetHeight(&height);
	rect.bottom = rect.top + ceil(height.ascent + height.descent + height.leading) + 5;

	fPreviewBox = new BBox(rect, "preview", B_FOLLOW_LEFT_RIGHT,
		B_WILL_DRAW | B_FRAME_EVENTS);
	AddChild(fPreviewBox);

	// Place the text slightly inside the entire box area, so
	// it doesn't overlap the box outline.
	rect = fPreviewBox->Bounds().InsetByCopy(3, 3);
	fPreviewText = new BStringView(rect, "preview text",
		"The quick brown fox jumps over the lazy dog.", 
		B_FOLLOW_LEFT_RIGHT);
	fPreviewText->SetFont(&fCurrentFont);
	fPreviewBox->AddChild(fPreviewText);
}


FontSelectionView::~FontSelectionView()
{
#ifndef INSTANT_UPDATE
	_UpdateSystemFont();
#endif
}


void
FontSelectionView::GetPreferredSize(float *_width, float *_height)
{
	// don't change the width if it is large enough
	if (_width) {
		*_width = fMaxFontNameWidth + 40 + fDivider
			+ fSizesMenuField->Bounds().Width();

		if (*_width < Bounds().Width())
			*_width = Bounds().Width();
	}

	if (_height) {
		BView* view = FindView("preview");
		if (view != NULL)
			*_height = view->Frame().bottom;
	}
}


void
FontSelectionView::SetDivider(float divider)
{
	fFontsMenuField->SetDivider(divider);

	fPreviewBox->ResizeBy(fDivider - divider, 0);
	fPreviewBox->MoveBy(divider - fDivider, 0);

	// if the view is not yet attached to a window, the resizing mode is ignored
	if (Window() == NULL)
		fPreviewText->ResizeBy(fDivider - divider, 0);

	fDivider = divider;
}


void
FontSelectionView::RelayoutIfNeeded()
{
	float width, height;
	GetPreferredSize(&width, &height);

	if (width > Bounds().Width()) {
		fSizesMenuField->MoveTo(fMaxFontNameWidth + fDivider + 40.0f, 
				fFontsMenuField->Bounds().top);
		ResizeTo(width, height);
	}
}


void
FontSelectionView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fSizesMenu->SetTargetForItems(this);
	UpdateFontsMenu();
}


void
FontSelectionView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgSetSize:
		{
			int32 size;
			if (msg->FindInt32("size", &size) != B_OK
				|| size == fCurrentFont.Size())
				break;

			fCurrentFont.SetSize(size);
			_UpdateFontPreview();

			Window()->PostMessage(kMsgUpdate);
			break;
		}

		case kMsgSetFamily:
		{
			const char* family;
			if (msg->FindString("family", &family) != B_OK)
				break;

			font_style style;
			fCurrentFont.GetFamilyAndStyle(NULL, &style);

			BMenuItem *familyItem = fFontsMenu->FindItem(family);
			if (familyItem != NULL) {
				_SelectCurrentFont(false);

				BMenuItem *item = familyItem->Submenu()->FindItem(style);
				if (item == NULL)
					item = familyItem->Submenu()->ItemAt(0);

				if (item != NULL) {
					item->SetMarked(true);
					fCurrentFont.SetFamilyAndStyle(family, item->Label());
					_UpdateFontPreview();
				}
			}

			Window()->PostMessage(kMsgUpdate);
			break;
		}

		case kMsgSetStyle:
		{
			const char* family;
			const char* style;
			if (msg->FindString("family", &family) != B_OK
				|| msg->FindString("style", &style) != B_OK)
				break;

			BMenuItem *familyItem = fFontsMenu->FindItem(family);
			if (!familyItem)
				break;

			_SelectCurrentFont(false);
			familyItem->SetMarked(true);

			fCurrentFont.SetFamilyAndStyle(family, style);
			_UpdateFontPreview();

			Window()->PostMessage(kMsgUpdate);
			break;
		}

		default:
			BView::MessageReceived(msg);
	}
}


void
FontSelectionView::_BuildSizesMenu()
{
	const int32 sizes[] = {7, 8, 9, 10, 11, 12, 13, 14, 18, 21, 24, 0};

	// build size menu
	for (int32 i = 0; sizes[i]; i++) {
		int32 size = sizes[i];
		if (size < kMinSize || size > kMaxSize)
			continue;

		char label[32];
		snprintf(label, sizeof(label), "%ld", size);

		BMessage* message = new BMessage(kMsgSetSize);
		message->AddInt32("size", size);

		BMenuItem* item = new BMenuItem(label, message);
		if (size == fCurrentFont.Size())
			item->SetMarked(true);

		fSizesMenu->AddItem(item);
	}
}


void
FontSelectionView::_SelectCurrentFont(bool select)
{
	font_family family;
	font_style style;
	fCurrentFont.GetFamilyAndStyle(&family, &style);

	BMenuItem *item = fFontsMenu->FindItem(family);
	if (item != NULL) {
		item->SetMarked(select);

		if (item->Submenu() != NULL) {
			item = item->Submenu()->FindItem(style);
			if (item != NULL)
				item->SetMarked(select);
		}
	}
}


void
FontSelectionView::_SelectCurrentSize(bool select)
{
	char label[16];
	snprintf(label, sizeof(label), "%ld", (int32)fCurrentFont.Size());

	BMenuItem* item = fSizesMenu->FindItem(label);
	if (item != NULL)
		item->SetMarked(select);
}


void
FontSelectionView::_UpdateFontPreview()
{
	fPreviewText->SetFont(&fCurrentFont);
	fPreviewText->Invalidate();

#ifdef INSTANT_UPDATE
	_UpdateSystemFont();
#endif
}


void
FontSelectionView::_UpdateSystemFont()
{
	font_family family;
	font_style style;
	fCurrentFont.GetFamilyAndStyle(&family, &style);

	_set_system_font_(Name(), family, style, fCurrentFont.Size());
}


void
FontSelectionView::SetDefaults()
{
	font_family family;
	font_style style;
	float size;
	if (_get_system_default_font_(Name(), family, style, &size) != B_OK) {
		Revert();
		return;
	}

	BFont defaultFont;
	defaultFont.SetFamilyAndStyle(family, style);
	defaultFont.SetSize(size);

	if (defaultFont == fCurrentFont)
		return;

	_SelectCurrentFont(false);

	fCurrentFont = defaultFont;
	_UpdateFontPreview();

	_SelectCurrentFont(true);
	_SelectCurrentSize(true);
}


void
FontSelectionView::Revert()
{
	if (!IsRevertable())
		return;

	_SelectCurrentFont(false);

	fCurrentFont = fSavedFont;
	_UpdateFontPreview();

	_SelectCurrentFont(true);
	_SelectCurrentSize(true);
}


bool
FontSelectionView::IsRevertable()
{
	return fCurrentFont != fSavedFont;
}


void
FontSelectionView::UpdateFontsMenu()
{
	int32 numFamilies = count_font_families(); 

	fFontsMenu->RemoveItems(0, fFontsMenu->CountItems(), true);
	BFont font;
	fFontsMenu->GetFont(&font);

	font_family currentFamily;
	font_style currentStyle;
	fCurrentFont.GetFamilyAndStyle(&currentFamily, &currentStyle);

	for (int32 i = 0; i < numFamilies; i++) { 
		font_family family; 
		uint32 flags;
		if (get_font_family(i, &family, &flags) != B_OK)
			continue;

		// if we're setting the fixed font, we only want to show fixed fonts
		if (!strcmp(Name(), "fixed") && (flags & B_IS_FIXED) == 0)
			continue;

		float width = font.StringWidth(family);
		if (width > fMaxFontNameWidth)
			fMaxFontNameWidth = width;

		BMenu* stylesMenu = new BMenu(family);
		stylesMenu->SetRadioMode(true);
		stylesMenu->SetFont(&font);

		BMessage* message = new BMessage(kMsgSetFamily);
		message->AddString("family", family);
		BMenuItem* familyItem = new BMenuItem(stylesMenu, message);
		fFontsMenu->AddItem(familyItem);

		int32 numStyles = count_font_styles(family); 

		for (int32 j = 0; j < numStyles; j++) {
			font_style style; 
			if (get_font_style(family, j, &style, &flags) != B_OK)
				continue;

			message = new BMessage(kMsgSetStyle);
			message->AddString("family", (char*)family);
			message->AddString("style", (char*)style);

			BMenuItem *item = new BMenuItem(style, message);

			if (!strcmp(style, currentStyle) && !strcmp(family, currentFamily)) {
				item->SetMarked(true);
				familyItem->SetMarked(true);
			}
			stylesMenu->AddItem(item);
		}

		stylesMenu->SetTargetForItems(this);
	}

	fFontsMenu->SetTargetForItems(this);
}
