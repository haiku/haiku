/*
 * Copyright 2001-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "FontSelectionView.h"

#include <Box.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <LayoutItem.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>
#include <Spinner.h>

#include <FontPrivate.h>

#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Font Selection view"


#define INSTANT_UPDATE
	// if defined, the system font will be updated immediately, and not
	// only on exit

static const float kMinSize = 8.0;
static const float kMaxSize = 72.0;

static const char *kPreviewText = B_TRANSLATE_COMMENT(
	"The quick brown fox jumps over the lazy dog.",
	"Don't translate this literally ! Use a phrase showing all chars "
	"from A to Z.");


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


FontSelectionView::FontSelectionView(const char* name,
	const char* label, const BFont* currentFont)
	:
	BView(name, B_WILL_DRAW),
	fMessageTarget(this)
{
	if (currentFont == NULL) {
		if (!strcmp(Name(), "plain"))
			fCurrentFont = *be_plain_font;
		else if (!strcmp(Name(), "bold"))
			fCurrentFont = *be_bold_font;
		else if (!strcmp(Name(), "fixed"))
			fCurrentFont = *be_fixed_font;
		else if (!strcmp(Name(), "menu")) {
			menu_info info;
			get_menu_info(&info);

			fCurrentFont.SetFamilyAndStyle(info.f_family, info.f_style);
			fCurrentFont.SetSize(info.font_size);
		}
	} else
		fCurrentFont = *currentFont;

	fSavedFont = fCurrentFont;

	fFontsMenu = new BPopUpMenu("font menu");

	// font menu
	fFontsMenuField = new BMenuField("fonts", label, fFontsMenu);
	fFontsMenuField->SetAlignment(B_ALIGN_RIGHT);

	// font size
	BMessage* fontSizeMessage = new BMessage(kMsgSetSize);
	fontSizeMessage->AddString("name", Name());

	fFontSizeSpinner = new BSpinner("font size", B_TRANSLATE("Size:"),
		fontSizeMessage);

	fFontSizeSpinner->SetRange(kMinSize, kMaxSize);
	fFontSizeSpinner->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	// preview
	fPreviewTextView = new BStringView("preview text", kPreviewText);

	fPreviewTextView->SetFont(&fCurrentFont);
	fPreviewTextView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	// box around preview
	fPreviewBox = new BBox("preview box", B_WILL_DRAW | B_FRAME_EVENTS);
	fPreviewBox->AddChild(BGroupLayoutBuilder(B_HORIZONTAL)
		.Add(fPreviewTextView)
		.SetInsets(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING,
			B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
		.TopView()
	);

	_SelectCurrentSize();
}


FontSelectionView::~FontSelectionView()
{
#ifndef INSTANT_UPDATE
	_UpdateSystemFont();
#endif
}


void
FontSelectionView::SetTarget(BHandler* messageTarget)
{
	fMessageTarget = messageTarget;
	fFontSizeSpinner->SetTarget(messageTarget);
}


void
FontSelectionView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kMsgSetSize:
		{
			int32 size = fFontSizeSpinner->Value();
			if (size == fCurrentFont.Size())
				break;

			fCurrentFont.SetSize(size);
			_UpdateFontPreview();
			break;
		}

		case kMsgSetFamily:
		{
			const char* family;
			if (msg->FindString("family", &family) != B_OK)
				break;

			font_style style;
			fCurrentFont.GetFamilyAndStyle(NULL, &style);

			BMenuItem* familyItem = fFontsMenu->FindItem(family);
			if (familyItem != NULL) {
				_SelectCurrentFont(false);

				BMenuItem* item = familyItem->Submenu()->FindItem(style);
				if (item == NULL)
					item = familyItem->Submenu()->ItemAt(0);

				if (item != NULL) {
					item->SetMarked(true);
					fCurrentFont.SetFamilyAndStyle(family, item->Label());
					_UpdateFontPreview();
				}
			}
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
			break;
		}

		default:
			BView::MessageReceived(msg);
	}
}

BView*
FontSelectionView::GetPreviewBox() const
{
	return fPreviewBox;
}


BView*
FontSelectionView::GetFontSizeSpinner() const
{
	return fFontSizeSpinner;
}


BLayoutItem*
FontSelectionView::CreateFontsLabelLayoutItem() const
{
	return fFontsMenuField->CreateLabelLayoutItem();
}


BLayoutItem*
FontSelectionView::CreateFontsMenuBarLayoutItem() const
{
	return fFontsMenuField->CreateMenuBarLayoutItem();
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
FontSelectionView::_SelectCurrentSize()
{
	fFontSizeSpinner->SetValue((int32)fCurrentFont.Size());
}

void
FontSelectionView::_UpdateFontPreview()
{
	const BRect textFrame = fPreviewTextView->Bounds();
	BString text = BString(kPreviewText);
	fCurrentFont.TruncateString(&text, B_TRUNCATE_END, textFrame.Width());
	fPreviewTextView->SetFont(&fCurrentFont);
	fPreviewTextView->SetText(text);

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

	if (strcmp(Name(), "menu") == 0) {
		// The menu font is not handled as a system font
		menu_info info;
		get_menu_info(&info);

		strlcpy(info.f_family, (const char*)family, B_FONT_FAMILY_LENGTH);
		strlcpy(info.f_style, (const char*)style, B_FONT_STYLE_LENGTH);
		info.font_size = fCurrentFont.Size();

		set_menu_info(&info);
	} else
		_set_system_font_(Name(), family, style, fCurrentFont.Size());
}


void
FontSelectionView::SetDefaults()
{
	font_family family;
	font_style style;
	float size;
	const char* fontName;

	if (strcmp(Name(), "menu") == 0)
		fontName = "plain";
	else
		fontName = Name();

	if (_get_system_default_font_(fontName, family, style, &size) != B_OK) {
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
	_SelectCurrentSize();
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
	_SelectCurrentSize();
}


bool
FontSelectionView::IsDefaultable()
{
	font_family defaultFamily;
	font_style defaultStyle;
	float defaultSize;
	const char* fontName;

	if (strcmp(Name(), "menu") == 0)
		fontName = "plain";
	else
		fontName = Name();

	if (_get_system_default_font_(fontName, defaultFamily, defaultStyle,
		&defaultSize) != B_OK) {
		return false;
	}

	font_family currentFamily;
	font_style currentStyle;
	float currentSize;

	fCurrentFont.GetFamilyAndStyle(&currentFamily, &currentStyle);
	currentSize = fCurrentFont.Size();

	return strcmp(currentFamily, defaultFamily) != 0
		|| strcmp(currentStyle, defaultStyle) != 0
		|| currentSize != defaultSize;
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

		// if we're setting the fixed font, we only want to show fixed and
		// full-and-half-fixed fonts
		if (strcmp(Name(), "fixed") == 0
			&& (flags
				& (B_IS_FIXED | B_PRIVATE_FONT_IS_FULL_AND_HALF_FIXED)) == 0) {
			continue;
		}

		float width = font.StringWidth(family);
		if (width > fMaxFontNameWidth)
			fMaxFontNameWidth = width;

		BMenu* stylesMenu = new BMenu(family);
		stylesMenu->SetRadioMode(true);
		stylesMenu->SetFont(&font);

		BMessage* message = new BMessage(kMsgSetFamily);
		message->AddString("family", family);
		message->AddString("name", Name());

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
			message->AddString("name", Name());

			BMenuItem* item = new BMenuItem(style, message);

			if (!strcmp(style, currentStyle)
				&& !strcmp(family, currentFamily)) {
				item->SetMarked(true);
				familyItem->SetMarked(true);
			}
			stylesMenu->AddItem(item);
		}

		stylesMenu->SetTargetForItems(fMessageTarget);
	}

	fFontsMenu->SetTargetForItems(fMessageTarget);
}
