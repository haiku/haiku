/*
 * Copyright 2003-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "PNGView.h"
#include "PNGTranslator.h"

#include <Alert.h>
#include <Catalog.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>
#include <TextView.h>

#include <stdio.h>
#define PNG_NO_PEDANTIC_WARNINGS
#include <png.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PNGTranslator"

PNGView::PNGView(const BRect &frame, const char *name, uint32 resizeMode,
		uint32 flags, TranslatorSettings *settings)
	: BView(frame, name, resizeMode, flags | B_FRAME_EVENTS)
{
	fSettings = settings;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);
	float height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	BRect rect(10, 10, 200, 10 + height);
	BStringView *stringView = new BStringView(rect, "title", 
		B_TRANSLATE("PNG image translator"));
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	float maxWidth = stringView->Bounds().Width();

	rect.OffsetBy(0, height + 10);
	char version[256];
	snprintf(version, sizeof(version), B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(PNG_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(PNG_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(PNG_TRANSLATOR_VERSION)),
		__DATE__);
	stringView = new BStringView(rect, "version", version);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	if (stringView->Bounds().Width() > maxWidth)
		maxWidth = stringView->Bounds().Width();

	GetFontHeight(&fontHeight);
	height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	rect.OffsetBy(0, height + 5);
	stringView = new BStringView(rect, 
		"Copyright", B_UTF8_COPYRIGHT "2003-2006 Haiku Inc.");
	stringView->ResizeToPreferred();
	AddChild(stringView);

	// setup PNG interlace options

	fInterlaceMenu = new BPopUpMenu(B_TRANSLATE("Interlace Option"));
	BMenuItem* item = new BMenuItem(B_TRANSLATE("None"), 
		_InterlaceMessage(PNG_INTERLACE_NONE));
	if (fSettings->SetGetInt32(PNG_SETTING_INTERLACE) == PNG_INTERLACE_NONE)
		item->SetMarked(true);
	fInterlaceMenu->AddItem(item);

	item = new BMenuItem("Adam7", _InterlaceMessage(PNG_INTERLACE_ADAM7));
	if (fSettings->SetGetInt32(PNG_SETTING_INTERLACE) == PNG_INTERLACE_ADAM7)
		item->SetMarked(true);
	fInterlaceMenu->AddItem(item);

	rect.OffsetBy(0, stringView->Frame().Height() + 20.0f);
	BMenuField* menuField = new BMenuField(rect, 
		B_TRANSLATE("PNG Interlace Menu"),
		B_TRANSLATE("Interlacing type:"), fInterlaceMenu);
	menuField->SetDivider(menuField->StringWidth(menuField->Label()) + 7.0f);
	menuField->ResizeToPreferred();
	AddChild(menuField);

	rect.OffsetBy(0, height + 15);
	rect.right = Bounds().right;
	rect.bottom = Bounds().bottom;
	fCopyrightView = new BTextView(rect, "PNG copyright", 
		rect.OffsetToCopy(B_ORIGIN),
		B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	fCopyrightView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fCopyrightView->SetLowColor(fCopyrightView->ViewColor());
	fCopyrightView->MakeEditable(false);
	fCopyrightView->SetText(png_get_copyright(NULL));

	BFont font;
	font.SetSize(font.Size() * 0.8);
	fCopyrightView->SetFontAndColor(&font, B_FONT_SIZE, NULL);
	AddChild(fCopyrightView);

	if (maxWidth + 20 > Bounds().Width())
		ResizeTo(maxWidth + 20, Bounds().Height());
	if (Bounds().Height() < rect.top + stringView->Bounds().Height() 
		* 3.0f + 8.0f)
		ResizeTo(Bounds().Width(), rect.top + height * 3.0f + 8.0f);

	fCopyrightView->SetTextRect(fCopyrightView->Bounds());
}


PNGView::~PNGView()
{
	fSettings->Release();
}


BMessage *
PNGView::_InterlaceMessage(int32 kind)
{
	BMessage* message = new BMessage(M_PNG_SET_INTERLACE);
	message->AddInt32(PNG_SETTING_INTERLACE, kind);

	return message;
}


void
PNGView::AttachedToWindow()
{
	BView::AttachedToWindow();

	// set target for interlace options menu items
	fInterlaceMenu->SetTargetForItems(this);
}


void
PNGView::FrameResized(float width, float height)
{
	// This works around a flaw of BTextView
	fCopyrightView->SetTextRect(fCopyrightView->Bounds());
}


void
PNGView::MessageReceived(BMessage *message)
{
	if (message->what == M_PNG_SET_INTERLACE) {
		// change setting for interlace option
		int32 option;
		if (message->FindInt32(PNG_SETTING_INTERLACE, &option) == B_OK) {
			fSettings->SetGetInt32(PNG_SETTING_INTERLACE, &option);
			fSettings->SaveSettings();
		}
	} else
		BView::MessageReceived(message);
}

