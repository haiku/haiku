/*
 * Copyright 2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 */


#include "AdvancedSettingsView.h"
#include "MainWindow.h"

#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>

#include <stdio.h>


#define INSTANT_UPDATE
	// if defined, the changes will take place immediately and not only on exit
#define DISABLE_HINTING_CONTROL
	// if defined, the hinting menu is disabled (hinting not properly
	// implemented)
	
static const int32 kMsgSetAntialiasing = 'anti';
static const int32 kMsgSetHinting = 'hint';
static const char* kSubpixelLabel = "Subpixel antialiasing";
static const char* kGrayscaleLabel = "Grayscale antialiasing";
static const char* kNoHintingLabel = "Off";
static const char* kFullHintingLabel = "On";

// private font API
extern void _set_font_subpixel_antialiasing_(bool subpix);
extern status_t _get_font_subpixel_antialiasing_(bool* subpix);
extern void _set_hinting_(bool subpix);
extern status_t _get_hinting_(bool* subpix);


//	#pragma mark -


AdvancedSettingsView::AdvancedSettingsView(BRect _rect, const char* name)
	: BView(_rect, name, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	if (_get_font_subpixel_antialiasing_(&fCurrentSubpixelAntialiasing) != B_OK)
		fCurrentSubpixelAntialiasing = false;
	fSavedSubpixelAntialiasing = fCurrentSubpixelAntialiasing;
	
	if (_get_hinting_(&fCurrentHinting) != B_OK)
		fCurrentHinting = true;
	fSavedHinting = fCurrentHinting;

	fDivider = StringWidth("Character hinting:") + 5;
	
	fAntialiasingMenu = new BPopUpMenu("Antialiasing menu");
	fHintingMenu = new BPopUpMenu("Hinting menu");

	// antialiasing menu
	BRect rect(Bounds());
	fAntialiasingMenuField = new BMenuField(rect, "antialiasing",
		"Antialiasing type:", fAntialiasingMenu, false);
	fAntialiasingMenuField->SetDivider(fDivider);
	fAntialiasingMenuField->SetAlignment(B_ALIGN_RIGHT);
	fAntialiasingMenuField->ResizeToPreferred();
	AddChild(fAntialiasingMenuField);
	_BuildAntialiasingMenu();
	
	// hinting menu
	float shift = fAntialiasingMenuField->Bounds().Height()+5;
	rect.top += shift;
	rect.bottom += shift;
	fHintingMenuField = new BMenuField(rect, "hinting",
		"Character hinting:", fHintingMenu, false);
	fHintingMenuField->SetDivider(fDivider);
	fHintingMenuField->SetAlignment(B_ALIGN_RIGHT);
	fHintingMenuField->ResizeToPreferred();
	AddChild(fHintingMenuField);
	_BuildHintingMenu();
	
#ifdef DISABLE_HINTING_CONTROL
	fHintingMenuField->SetEnabled(false);
#endif
	
	_SetCurrentAntialiasing();
	_SetCurrentHinting();
}


AdvancedSettingsView::~AdvancedSettingsView()
{
#ifndef INSTANT_UPDATE
	_set_font_subpixel_antialiasing_(fCurrentSubpixelAntialiasing);
	_set_hinting(fCurrentHinting);
#endif
}


void
AdvancedSettingsView::GetPreferredSize(float *_width, float *_height)
{
	// don't change the width if it is large enough
	if (_width) {
		*_width = (StringWidth(kSubpixelLabel) > StringWidth(kGrayscaleLabel) ?
			StringWidth(kSubpixelLabel) : StringWidth(kGrayscaleLabel))
			+ fDivider + 30;
		if (*_width < Bounds().Width())
			*_width = Bounds().Width();
	}

	*_height = fHintingMenuField->Frame().bottom;
}


void
AdvancedSettingsView::SetDivider(float divider)
{
	fAntialiasingMenuField->SetDivider(divider);
	fHintingMenuField->SetDivider(divider);
	fDivider = divider;
}


void
AdvancedSettingsView::RelayoutIfNeeded()
{
	float width, height;
	GetPreferredSize(&width, &height);

	if (width > Bounds().Width() || height > Bounds().Height()) {
		ResizeTo(width, height);
	}
}


void
AdvancedSettingsView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fAntialiasingMenu->SetTargetForItems(this);
	fHintingMenu->SetTargetForItems(this);
}


void
AdvancedSettingsView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgSetAntialiasing:
		{
			bool subpixelAntialiasing;
			if (msg->FindBool("antialiasing", &subpixelAntialiasing) != B_OK 
				|| subpixelAntialiasing == fCurrentSubpixelAntialiasing)
				break;
			fCurrentSubpixelAntialiasing = subpixelAntialiasing;
#ifdef INSTANT_UPDATE
			_set_font_subpixel_antialiasing_(fCurrentSubpixelAntialiasing);
#endif
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case kMsgSetHinting:
		{
			bool hinting;
			if (msg->FindBool("hinting", &hinting) != B_OK 
				|| hinting == fCurrentHinting)
				break;
			fCurrentHinting = hinting;
#ifdef INSTANT_UPDATE
			_set_hinting_(fCurrentHinting);
#endif
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
AdvancedSettingsView::_BuildAntialiasingMenu()
{
	BMessage* message = new BMessage(kMsgSetAntialiasing);
	message->AddBool("antialiasing", false);

	BMenuItem* item = new BMenuItem(kGrayscaleLabel, message);

	fAntialiasingMenu->AddItem(item);
	
	BMessage* message2 = new BMessage(kMsgSetAntialiasing);
	message2->AddBool("antialiasing", true);

	BMenuItem* item2 = new BMenuItem(kSubpixelLabel, message2);

	fAntialiasingMenu->AddItem(item2);
}


void
AdvancedSettingsView::_BuildHintingMenu()
{
	BMessage* message = new BMessage(kMsgSetHinting);
	message->AddBool("hinting", false);

	BMenuItem* item = new BMenuItem(kNoHintingLabel, message);

	fHintingMenu->AddItem(item);
	
	BMessage* message2 = new BMessage(kMsgSetHinting);
	message2->AddBool("hinting", true);

	BMenuItem* item2 = new BMenuItem(kFullHintingLabel, message2);

	fHintingMenu->AddItem(item2);
}


void
AdvancedSettingsView::_SetCurrentAntialiasing()
{
	BMenuItem *item = fAntialiasingMenu->FindItem(
		fCurrentSubpixelAntialiasing ? kSubpixelLabel : kGrayscaleLabel);
	if (item != NULL)
		item->SetMarked(true);
}


void
AdvancedSettingsView::_SetCurrentHinting()
{
	BMenuItem *item = fHintingMenu->FindItem(
		fCurrentHinting ? kFullHintingLabel : kNoHintingLabel);
	if (item != NULL)
		item->SetMarked(true);
}


void
AdvancedSettingsView::SetDefaults()
{	
}


bool
AdvancedSettingsView::IsDefaultable()
{
	return false;
}


bool
AdvancedSettingsView::IsRevertable()
{
	return (fCurrentSubpixelAntialiasing != fSavedSubpixelAntialiasing) 
		|| (fCurrentHinting != fSavedHinting);
}


void
AdvancedSettingsView::Revert()
{
	if (!IsRevertable())
		return;

	fCurrentSubpixelAntialiasing = fSavedSubpixelAntialiasing;
	fCurrentHinting = fSavedHinting;
#ifdef INSTANT_UPDATE
	_set_font_subpixel_antialiasing_(fCurrentSubpixelAntialiasing);
	_set_hinting_(fCurrentHinting);
#endif
	_SetCurrentAntialiasing();
	_SetCurrentHinting();
}
