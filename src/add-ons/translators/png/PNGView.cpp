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
#include <LayoutBuilder.h>
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
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BStringView *titleView  = new BStringView("title",
		B_TRANSLATE("PNG image translator"));
	titleView->SetFont(be_bold_font);

	char version[256];
	snprintf(version, sizeof(version), B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(PNG_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(PNG_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(PNG_TRANSLATOR_VERSION)),
		__DATE__);
	BStringView *versionView =  new BStringView("version", version);

	BStringView *copyrightView =  new BStringView(
		"Copyright", B_UTF8_COPYRIGHT "2003-2006 Haiku Inc.");

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

	BMenuField* menuField = new BMenuField(
		B_TRANSLATE("PNG Interlace Menu"),
		B_TRANSLATE("Interlacing type:"), fInterlaceMenu);
	menuField->SetDivider(menuField->StringWidth(menuField->Label()) + 7.0f);
	menuField->ResizeToPreferred();

	fCopyrightView = new BTextView("PNG copyright");
	fCopyrightView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fCopyrightView->SetLowColor(fCopyrightView->ViewColor());
	fCopyrightView->MakeEditable(false);
	fCopyrightView->SetWordWrap(false);
	fCopyrightView->MakeResizable(true);
	fCopyrightView->SetText(png_get_copyright(NULL));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(versionView)
		.Add(copyrightView)
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.Add(menuField)
			.AddGlue()
			.End()
		.AddGlue()
		.Add(fCopyrightView);

	BFont font;
	GetFont(&font);
	SetExplicitPreferredSize(BSize((font.Size() * 390) / 12,
		(font.Size() * 180) / 12));

	// TODO: remove this workaround for ticket #4217
	fCopyrightView->SetExplicitPreferredSize(
		BSize(fCopyrightView->LineWidth(4), fCopyrightView->TextHeight(0, 80)));
	fCopyrightView->SetExplicitMaxSize(fCopyrightView->ExplicitPreferredSize());
	fCopyrightView->SetExplicitMinSize(fCopyrightView->ExplicitPreferredSize());
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

