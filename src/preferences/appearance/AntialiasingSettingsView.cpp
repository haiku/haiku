/*
 * Copyright 2008, Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "AntialiasingSettingsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

#include "APRWindow.h"


#define INSTANT_UPDATE
	// if defined, the changes will take place immediately and not only on exit
//#define DISABLE_HINTING_CONTROL
	// if defined, the hinting menu is disabled (hinting not properly
	// implemented)
	
static const int32 kMsgSetAntialiasing = 'anti';
static const int32 kMsgSetHinting = 'hint';
static const int32 kMsgSetAverageWeight = 'avrg';
static const char* kSubpixelLabel = "Subpixel antialiasing";
static const char* kGrayscaleLabel = "Grayscale antialiasing";
static const char* kNoHintingLabel = "Off";
static const char* kFullHintingLabel = "On";

extern void set_subpixel_antialiasing(bool subpix);
extern status_t get_subpixel_antialiasing(bool* subpix);
extern void set_hinting(bool hinting);
extern status_t get_hinting(bool* hinting);
extern void set_average_weight(unsigned char averageWeight);
extern status_t get_average_weight(unsigned char* averageWeight);


//	#pragma mark -


AntialiasingSettingsView::AntialiasingSettingsView(BRect _rect, const char* name)
	: BView(_rect, name, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW)
{
	if (get_subpixel_antialiasing(&fCurrentSubpixelAntialiasing) != B_OK)
		fCurrentSubpixelAntialiasing = false;
	fSavedSubpixelAntialiasing = fCurrentSubpixelAntialiasing;
	
	if (get_hinting(&fCurrentHinting) != B_OK)
		fCurrentHinting = true;
	fSavedHinting = fCurrentHinting;
	
	if (get_average_weight(&fCurrentAverageWeight) != B_OK)
		fCurrentAverageWeight = 100;
	fSavedAverageWeight = fCurrentAverageWeight;

	fDivider = StringWidth("Strength of colours in sbpx AA:") + 5;
	
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
	
	//Average weight in subpixel filtering
	float shift = fAntialiasingMenuField->Bounds().Height()+5;
	rect.top +=shift;
	rect.bottom += shift;
	fAverageWeightControl = new BTextControl(rect, "averageWeightControl",
		"Strength of colours in sbpx AA:", NULL,
		new BMessage(kMsgSetAverageWeight), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fAverageWeightControl -> SetDivider(fDivider);
	fAverageWeightControl -> TextView() -> SetMaxBytes(3);
	fAverageWeightControl -> ResizeToPreferred();
	AddChild(fAverageWeightControl);
	for (int i = 0; i < 256; i++) {
		if (i < '0' || i > '9') {
			fAverageWeightControl->TextView()->DisallowChar(i);
			fAverageWeightControl->TextView()->DisallowChar(i);
		}
	}
	fAverageWeightControl -> SetEnabled(false);
	
	// hinting menu
	shift = fAverageWeightControl->Bounds().Height()+5;
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
	_SetCurrentAverageWeight();
}


AntialiasingSettingsView::~AntialiasingSettingsView()
{
#ifndef INSTANT_UPDATE
	set_subpixel_antialiasing(fCurrentSubpixelAntialiasing);
	set_hinting(fCurrentHinting);
	set_average_weight(fCurrentAverageWeight);
#endif
}


void
AntialiasingSettingsView::GetPreferredSize(float *_width, float *_height)
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
AntialiasingSettingsView::SetDivider(float divider)
{
	fAntialiasingMenuField -> SetDivider(divider);
	fHintingMenuField -> SetDivider(divider);
	fAverageWeightControl -> SetDivider(divider);
	fDivider = divider;
}


void
AntialiasingSettingsView::RelayoutIfNeeded()
{
	float width, height;
	GetPreferredSize(&width, &height);

	if (width > Bounds().Width() || height > Bounds().Height()) {
		ResizeTo(width, height);
	}
}


void
AntialiasingSettingsView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fAntialiasingMenu -> SetTargetForItems(this);
	fHintingMenu -> SetTargetForItems(this);
	fAverageWeightControl -> SetTarget(this);
}


void
AntialiasingSettingsView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgSetAntialiasing:
		{
			bool subpixelAntialiasing;
			if (msg->FindBool("antialiasing", &subpixelAntialiasing) != B_OK 
				|| subpixelAntialiasing == fCurrentSubpixelAntialiasing)
				break;
			fCurrentSubpixelAntialiasing = subpixelAntialiasing;
			fAverageWeightControl -> SetEnabled(fCurrentSubpixelAntialiasing);
#ifdef INSTANT_UPDATE
			set_subpixel_antialiasing(fCurrentSubpixelAntialiasing);
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
			set_hinting(fCurrentHinting);
#endif
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case kMsgSetAverageWeight:
		{
			int subpixelWeight;
			unsigned char averageWeight;
			if (fAverageWeightControl -> Text() != NULL) {
				subpixelWeight = atoi(fAverageWeightControl -> Text());
				if (subpixelWeight > 255) {
					subpixelWeight = 255;
					BString subpixelWeightString;
					subpixelWeightString << subpixelWeight;
					fAverageWeightControl -> SetText(
						subpixelWeightString.String());
				}
				averageWeight = 255 - subpixelWeight;
				if (averageWeight == fCurrentAverageWeight) break;
			} else break;
			fCurrentAverageWeight = averageWeight;
#ifdef INSTANT_UPDATE
			set_average_weight(fCurrentAverageWeight);
#endif
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
AntialiasingSettingsView::_BuildAntialiasingMenu()
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
AntialiasingSettingsView::_BuildHintingMenu()
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
AntialiasingSettingsView::_SetCurrentAntialiasing()
{
	BMenuItem *item = fAntialiasingMenu->FindItem(
		fCurrentSubpixelAntialiasing ? kSubpixelLabel : kGrayscaleLabel);
	if (item != NULL)
		item->SetMarked(true);
	if (fCurrentSubpixelAntialiasing) fAverageWeightControl -> SetEnabled(true);
}


void
AntialiasingSettingsView::_SetCurrentHinting()
{
	BMenuItem *item = fHintingMenu->FindItem(
		fCurrentHinting ? kFullHintingLabel : kNoHintingLabel);
	if (item != NULL)
		item->SetMarked(true);
}


void
AntialiasingSettingsView::_SetCurrentAverageWeight()
{
	BString subpixelWeightString;
	subpixelWeightString << (255 - fCurrentAverageWeight);
	fAverageWeightControl -> SetText(subpixelWeightString.String());
}


void
AntialiasingSettingsView::SetDefaults()
{
}


bool
AntialiasingSettingsView::IsDefaultable()
{
	return false;
}


bool
AntialiasingSettingsView::IsRevertable()
{
	return (fCurrentSubpixelAntialiasing != fSavedSubpixelAntialiasing) 
		|| (fCurrentHinting != fSavedHinting)
		|| (fCurrentAverageWeight != fSavedAverageWeight);
}


void
AntialiasingSettingsView::Revert()
{
	if (!IsRevertable())
		return;

	fCurrentSubpixelAntialiasing = fSavedSubpixelAntialiasing;
	fCurrentHinting = fSavedHinting;
	fCurrentAverageWeight = fSavedAverageWeight;
#ifdef INSTANT_UPDATE
	set_subpixel_antialiasing(fCurrentSubpixelAntialiasing);
	set_hinting(fCurrentHinting);
	set_average_weight(fCurrentAverageWeight);
#endif
	_SetCurrentAntialiasing();
	_SetCurrentHinting();
	_SetCurrentAverageWeight();
}
