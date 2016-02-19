////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFView.cpp
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

// Additional authors:	Stephan Aßmus, <superstippi@gmx.de>
//						Philippe Saint-Pierre, <stpere@gmail.com>
//						Maxime Simon, <maxime.simon@gmail.com>
//						John Scipione, <jscipione@gmail.com>

#include "GIFView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Box.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>
#include <String.h>
#include <TextControl.h>

#include "GIFTranslator.h"
#include "SavePalette.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GIFView"


GIFView::GIFView(TranslatorSettings* settings)
	:
	BGroupView("GIFView", B_VERTICAL),
	fSettings(settings)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fTitle = new BStringView("Title", B_TRANSLATE("GIF image translator"));
	fTitle->SetFont(be_bold_font);

	char version_string[100];
	snprintf(version_string, sizeof(version_string),
		B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(GIF_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(GIF_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(GIF_TRANSLATOR_VERSION)),
		__DATE__);
	fVersion = new BStringView("Version", version_string);

	const char* copyrightString
		= "©2003 Daniel Switkin, software@switkin.com";
	fCopyright = new BStringView("Copyright", copyrightString);

	// menu fields (Palette & Colors)
	fWebSafeMI = new BMenuItem(B_TRANSLATE("Websafe"),
		new BMessage(GV_WEB_SAFE), 0, 0);
	fBeOSSystemMI = new BMenuItem(B_TRANSLATE("BeOS system"),
		new BMessage(GV_BEOS_SYSTEM), 0, 0);
	fGreyScaleMI = new BMenuItem(B_TRANSLATE("Greyscale"),
		new BMessage(GV_GREYSCALE), 0, 0);
	fOptimalMI = new BMenuItem(B_TRANSLATE("Optimal"),
		new BMessage(GV_OPTIMAL), 0, 0);
	fPaletteM = new BPopUpMenu("PalettePopUpMenu", true, true,
		B_ITEMS_IN_COLUMN);
	fPaletteM->AddItem(fWebSafeMI);
	fPaletteM->AddItem(fBeOSSystemMI);
	fPaletteM->AddItem(fGreyScaleMI);
	fPaletteM->AddItem(fOptimalMI);

	fColorCountM = new BPopUpMenu("ColorCountPopUpMenu", true, true,
		B_ITEMS_IN_COLUMN);
	int32 count = 2;
	for (int32 i = 0; i < 8; i++) {
		BMessage* message = new BMessage(GV_SET_COLOR_COUNT);
		message->AddInt32(GIF_SETTING_PALETTE_SIZE, i + 1);
		BString label;
		label << count;
		fColorCountMI[i] = new BMenuItem(label.String(), message, 0, 0);
		fColorCountM->AddItem(fColorCountMI[i]);
		count *= 2;
	}
	fColorCount256MI = fColorCountMI[7];

	fPaletteMF = new BMenuField(B_TRANSLATE("Palette:"), fPaletteM);
	fPaletteMF->SetAlignment(B_ALIGN_RIGHT);
	fColorCountMF = new BMenuField(B_TRANSLATE("Colors:"), fColorCountM);
	fColorCountMF->SetAlignment(B_ALIGN_RIGHT);

	// check boxes
	fUseDitheringCB = new BCheckBox(B_TRANSLATE("Use dithering"),
		new BMessage(GV_USE_DITHERING));
	fDitheringBox = new BBox("dithering", B_WILL_DRAW, B_NO_BORDER);
	fDitheringBox->SetLabel(fUseDitheringCB);

	fInterlacedCB = new BCheckBox(B_TRANSLATE("Write interlaced images"),
		new BMessage(GV_INTERLACED));
	fInterlacedBox = new BBox("interlaced", B_WILL_DRAW, B_NO_BORDER);
	fInterlacedBox->SetLabel(fInterlacedCB);

	fUseTransparentCB = new BCheckBox(B_TRANSLATE("Write transparent images"),
		new BMessage(GV_USE_TRANSPARENT));

	// radio buttons
	fUseTransparentAutoRB = new BRadioButton(
		B_TRANSLATE("Automatic (from alpha channel)"),
		new BMessage(GV_USE_TRANSPARENT_AUTO));

	fUseTransparentColorRB = new BRadioButton(B_TRANSLATE("Use RGB color"),
		new BMessage(GV_USE_TRANSPARENT_COLOR));

	// text controls
	fRedTextControl = new BTextControl("", "0",
		new BMessage(GV_TRANSPARENT_RED));
	fGreenTextControl = new BTextControl("", "0",
		new BMessage(GV_TRANSPARENT_GREEN));
	fBlueTextControl = new BTextControl("", "0",
		new BMessage(GV_TRANSPARENT_BLUE));

	fTransparentBox = new BBox(B_FANCY_BORDER,
		BLayoutBuilder::Grid<>(3.0f, 5.0f)
			.Add(fUseTransparentAutoRB, 0, 0, 4, 1)
			.Add(fUseTransparentColorRB, 0, 1)
			.Add(fRedTextControl, 1, 1)
			.Add(fGreenTextControl, 2, 1)
			.Add(fBlueTextControl, 3, 1)
			.SetInsets(10.0f, 6.0f, 10.0f, 10.0f)
			.View());
	fTransparentBox->SetLabel(fUseTransparentCB);

	BTextView* redTextView = fRedTextControl->TextView();
	BTextView* greenTextView = fGreenTextControl->TextView();
	BTextView* bluetextView = fBlueTextControl->TextView();

	for (uint32 x = 0; x < 256; x++) {
		if (x < '0' || x > '9') {
			redTextView->DisallowChar(x);
			greenTextView->DisallowChar(x);
			bluetextView->DisallowChar(x);
		}
	}

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fTitle)
		.Add(fVersion)
		.Add(fCopyright)
		.AddGlue()
		.AddGrid(10.0f, 5.0f)
			.Add(fPaletteMF->CreateLabelLayoutItem(), 0, 0)
			.Add(fPaletteMF->CreateMenuBarLayoutItem(), 1, 0)
			.Add(fColorCountMF->CreateLabelLayoutItem(), 0, 1)
			.Add(fColorCountMF->CreateMenuBarLayoutItem(), 1, 1)
		.End()
		.AddStrut(B_USE_SMALL_SPACING)
		.Add(fDitheringBox)
		.Add(fInterlacedBox)
		.Add(fTransparentBox)
		.AddGlue()
		.End();

	fSettings->Acquire();

	RestorePrefs();
}


GIFView::~GIFView()
{
	fSettings->Release();

	delete fTitle;
	delete fVersion;
	delete fCopyright;
	delete fPaletteMF;
	delete fColorCountMF;
	delete fDitheringBox;
	delete fInterlacedBox;
	delete fTransparentBox;
}


void
GIFView::RestorePrefs()
{
	fColorCountMF->SetEnabled(false);
	fUseDitheringCB->SetEnabled(true);

	switch (fSettings->SetGetInt32(GIF_SETTING_PALETTE_MODE)) {
		case WEB_SAFE_PALETTE:
			fWebSafeMI->SetMarked(true);
			break;

		case BEOS_SYSTEM_PALETTE:
			fBeOSSystemMI->SetMarked(true);
			break;

		case GREYSCALE_PALETTE:
			fGreyScaleMI->SetMarked(true);
			fUseDitheringCB->SetEnabled(false);
			break;

		case OPTIMAL_PALETTE:
			fOptimalMI->SetMarked(true);
			fColorCountMF->SetEnabled(true);
			break;

		default:
		{
			int32 value = WEB_SAFE_PALETTE;
			fSettings->SetGetInt32(GIF_SETTING_PALETTE_MODE, &value);
			fSettings->SaveSettings();
			fWebSafeMI->SetMarked(true);
		}
	}

	if (fColorCountMF->IsEnabled()
		&& fSettings->SetGetInt32(GIF_SETTING_PALETTE_SIZE) > 0
		&& fSettings->SetGetInt32(GIF_SETTING_PALETTE_SIZE) <= 8) {
		// display the stored color count
		fColorCountMI[fSettings->SetGetInt32(GIF_SETTING_PALETTE_SIZE) - 1]
			->SetMarked(true);
	} else {
		// display 256 colors
		fColorCount256MI->SetMarked(true);
		int32 value = 8;
		fSettings->SetGetInt32(GIF_SETTING_PALETTE_SIZE, &value);
		fSettings->SaveSettings();
	}

	fInterlacedCB->SetValue(fSettings->SetGetBool(GIF_SETTING_INTERLACED));

	if (fGreyScaleMI->IsMarked())
		fUseDitheringCB->SetValue(false);
	else {
		fUseDitheringCB->SetValue(
			fSettings->SetGetBool(GIF_SETTING_USE_DITHERING));
	}
	fUseTransparentCB->SetValue(
		fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT));
	fUseTransparentAutoRB->SetValue(
		fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO));
	fUseTransparentColorRB->SetValue(
		!fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO));
	if (fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT)) {
		fUseTransparentAutoRB->SetEnabled(true);
		fUseTransparentColorRB->SetEnabled(true);
		fRedTextControl->SetEnabled(
			!fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO));
		fGreenTextControl->SetEnabled(
			!fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO));
		fBlueTextControl->SetEnabled(
			!fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO));
	} else {
		fUseTransparentAutoRB->SetEnabled(false);
		fUseTransparentColorRB->SetEnabled(false);
		fRedTextControl->SetEnabled(false);
		fGreenTextControl->SetEnabled(false);
		fBlueTextControl->SetEnabled(false);
	}

	char temp[4];
	sprintf(temp, "%d",
		(int)fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_RED));
	fRedTextControl->SetText(temp);
	sprintf(temp, "%d",
		(int)fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_GREEN));
	fGreenTextControl->SetText(temp);
	sprintf(temp, "%d",
		(int)fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_BLUE));
	fBlueTextControl->SetText(temp);
}


void
GIFView::AllAttached()
{
	BMessenger messenger(this);
	fInterlacedCB->SetTarget(messenger);
	fUseDitheringCB->SetTarget(messenger);
	fUseTransparentCB->SetTarget(messenger);
	fUseTransparentAutoRB->SetTarget(messenger);
	fUseTransparentColorRB->SetTarget(messenger);
	fRedTextControl->SetTarget(messenger);
	fGreenTextControl->SetTarget(messenger);
	fBlueTextControl->SetTarget(messenger);
	fPaletteM->SetTargetForItems(messenger);
	fColorCountM->SetTargetForItems(messenger);

	BView::AllAttached();

	if (Parent() == NULL && Window()->GetLayout() == NULL) {
		Window()->SetLayout(new BGroupLayout(B_VERTICAL));
		Window()->ResizeTo(PreferredSize().Width(), PreferredSize().Height());
	}
}


void
GIFView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case GV_WEB_SAFE:
		{
			int32 value = WEB_SAFE_PALETTE;
			fSettings->SetGetInt32(GIF_SETTING_PALETTE_MODE, &value);
			fUseDitheringCB->SetEnabled(true);
			fColorCountMF->SetEnabled(false);
			fColorCount256MI->SetMarked(true);
			break;
		}

		case GV_BEOS_SYSTEM:
		{
			int32 value = BEOS_SYSTEM_PALETTE;
			fSettings->SetGetInt32(GIF_SETTING_PALETTE_MODE, &value);
			fUseDitheringCB->SetEnabled(true);
			fColorCountMF->SetEnabled(false);
			fColorCount256MI->SetMarked(true);
			break;
		}

		case GV_GREYSCALE:
		{
			int32 value = GREYSCALE_PALETTE;
			bool usedithering = false;
			fSettings->SetGetInt32(GIF_SETTING_PALETTE_MODE, &value);
			fSettings->SetGetBool(GIF_SETTING_USE_DITHERING, &usedithering);
			fUseDitheringCB->SetEnabled(false);
			fUseDitheringCB->SetValue(false);
			fColorCountMF->SetEnabled(false);
			fColorCount256MI->SetMarked(true);
			break;
		}

		case GV_OPTIMAL:
		{
			int32 value = OPTIMAL_PALETTE;
			fSettings->SetGetInt32(GIF_SETTING_PALETTE_MODE, &value);
			fUseDitheringCB->SetEnabled(true);
			fColorCountMF->SetEnabled(true);
			fColorCountMI[fSettings->SetGetInt32(GIF_SETTING_PALETTE_SIZE) - 1]
				->SetMarked(true);
			break;
		}

		case GV_SET_COLOR_COUNT:
			if (fColorCountMF->IsEnabled()) {
				int32 sizeInBits;
				if (message->FindInt32(GIF_SETTING_PALETTE_SIZE, &sizeInBits)
						>= B_OK && sizeInBits > 0 && sizeInBits <= 8) {
					fSettings->SetGetInt32(GIF_SETTING_PALETTE_SIZE,
						&sizeInBits);
				}
			}
			break;

		case GV_INTERLACED:
		{
			bool value = fInterlacedCB->Value();
			fSettings->SetGetBool(GIF_SETTING_INTERLACED, &value);
			break;
		}

		case GV_USE_DITHERING:
		{
			bool value = fUseDitheringCB->Value();
			fSettings->SetGetBool(GIF_SETTING_USE_DITHERING, &value);
			break;
		}

		case GV_USE_TRANSPARENT:
		{
			bool value = fUseTransparentCB->Value();
			fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT, &value);
			if (value) {
				fUseTransparentAutoRB->SetEnabled(true);
				fUseTransparentColorRB->SetEnabled(true);
				fRedTextControl->SetEnabled(fUseTransparentColorRB->Value());
				fGreenTextControl->SetEnabled(fUseTransparentColorRB->Value());
				fBlueTextControl->SetEnabled(fUseTransparentColorRB->Value());
			} else {
				fUseTransparentAutoRB->SetEnabled(false);
				fUseTransparentColorRB->SetEnabled(false);
				fRedTextControl->SetEnabled(false);
				fGreenTextControl->SetEnabled(false);
				fBlueTextControl->SetEnabled(false);
			}
			break;
		}

		case GV_USE_TRANSPARENT_AUTO:
		{
			bool value = true;
			fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO, &value);
			fRedTextControl->SetEnabled(false);
			fGreenTextControl->SetEnabled(false);
			fBlueTextControl->SetEnabled(false);
			break;
		}

		case GV_USE_TRANSPARENT_COLOR:
		{
			bool value = false;
			fSettings->SetGetBool(GIF_SETTING_USE_TRANSPARENT_AUTO, &value);
			fRedTextControl->SetEnabled(true);
			fGreenTextControl->SetEnabled(true);
			fBlueTextControl->SetEnabled(true);
			break;
		}

		case GV_TRANSPARENT_RED:
		{
			int32 value = CheckInput(fRedTextControl);
			fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_RED, &value);
			break;
		}

		case GV_TRANSPARENT_GREEN:
		{
			int32 value = CheckInput(fGreenTextControl);
			fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_GREEN, &value);
			break;
		}

		case GV_TRANSPARENT_BLUE:
		{
			int32 value = CheckInput(fBlueTextControl);
			fSettings->SetGetInt32(GIF_SETTING_TRANSPARENT_BLUE, &value);
			break;
		}

		default:
			BView::MessageReceived(message);
	}

	fSettings->SaveSettings();
}


int
GIFView::CheckInput(BTextControl* control)
{
	int value = atoi(control->Text());
	if (value < 0 || value > 255) {
		value = (value < 0) ? 0 : 255;
		char temp[4];
		sprintf(temp, "%d", value);
		control->SetText(temp);
	}

	return value;
}
