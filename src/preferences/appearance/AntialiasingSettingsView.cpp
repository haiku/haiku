/*
 * Copyright 2008, Stephan Aßmus, <superstippi@gmx.de>
 * Copyright 2008, Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "AntialiasingSettingsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <freetype/config/ftoption.h>
	// for detected the availablility of subpixel anti-aliasing

#include <Box.h>
#include <CardLayout.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TextView.h>

#include "APRWindow.h"


//#define DISABLE_HINTING_CONTROL
	// if defined, the hinting menu is disabled (hinting not properly
	// implemented)
	
static const int32 kMsgSetAntialiasing = 'anti';
static const int32 kMsgSetHinting = 'hint';
static const int32 kMsgSetAverageWeight = 'avrg';
static const char* kSubpixelLabel = "LCD subpixel";
static const char* kGrayscaleLabel = "Grayscale";
static const char* kNoHintingLabel = "Off";
static const char* kFullHintingLabel = "On";


// #pragma mark - private libbe API


extern void set_subpixel_antialiasing(bool subpix);
extern status_t get_subpixel_antialiasing(bool* subpix);
extern void set_hinting(bool hinting);
extern status_t get_hinting(bool* hinting);
extern void set_average_weight(unsigned char averageWeight);
extern status_t get_average_weight(unsigned char* averageWeight);


//	#pragma mark -


AntialiasingSettingsView::AntialiasingSettingsView(BRect rect, const char* name)
	: BView(rect, name, B_FOLLOW_ALL, B_SUPPORTS_LAYOUT)
{
	// collect the current system settings
	if (get_subpixel_antialiasing(&fCurrentSubpixelAntialiasing) != B_OK)
		fCurrentSubpixelAntialiasing = false;
	fSavedSubpixelAntialiasing = fCurrentSubpixelAntialiasing;
	
	if (get_hinting(&fCurrentHinting) != B_OK)
		fCurrentHinting = true;
	fSavedHinting = fCurrentHinting;
	
	if (get_average_weight(&fCurrentAverageWeight) != B_OK)
		fCurrentAverageWeight = 100;
	fSavedAverageWeight = fCurrentAverageWeight;

	// create the controls

	// antialiasing menu
	_BuildAntialiasingMenu();
	fAntialiasingMenuField = new BMenuField("antialiasing",
		"Anti-aliasing type:", fAntialiasingMenu, NULL);
	
	// "average weight" in subpixel filtering
	fAverageWeightControl = new BSlider("averageWeightControl",
		"Reduce colored edges filter strength:",
		new BMessage(kMsgSetAverageWeight), 0, 255, B_HORIZONTAL);
	fAverageWeightControl->SetLimitLabels("Off", "Strong");
	fAverageWeightControl->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fAverageWeightControl->SetHashMarkCount(255 / 15);
	fAverageWeightControl->SetEnabled(false);
	
	// hinting menu
	_BuildHintingMenu();
	fHintingMenuField = new BMenuField("hinting", "Character hinting:",
		fHintingMenu, NULL);

#ifdef DISABLE_HINTING_CONTROL
	fHintingMenuField->SetEnabled(false);
#endif

	// subpixelAntialiasingDisabledLabel
	// TODO: Replace with layout friendly constructor once available.
	BTextView* subpixelAntialiasingDisabledLabel = new BTextView(
		rect.OffsetToCopy(B_ORIGIN), "unavailable label",
		rect.OffsetToCopy(B_ORIGIN).InsetBySelf(10, 10),
		B_FOLLOW_NONE, B_WILL_DRAW | B_SUPPORTS_LAYOUT);
	subpixelAntialiasingDisabledLabel->SetText("Subpixel based anti-aliasing "
		"is not available in this build of Haiku to avoid possible patent "
		"issues. To enable this feature, you have to build Haiku yourself "
		"and enable certain options in the libfreetype configuration header.");
	subpixelAntialiasingDisabledLabel->SetViewColor(
		ui_color(B_PANEL_BACKGROUND_COLOR));
	subpixelAntialiasingDisabledLabel->MakeEditable(false);
	subpixelAntialiasingDisabledLabel->MakeSelectable(false);

	BCardLayout* cardLayout = new BCardLayout();
	SetLayout(cardLayout);

	// controls pane
	AddChild(BGridLayoutBuilder(10, 10)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 0, 2)

		.Add(fHintingMenuField->CreateLabelLayoutItem(), 0, 1)
		.Add(fHintingMenuField->CreateMenuBarLayoutItem(), 1, 1)

		.Add(fAntialiasingMenuField->CreateLabelLayoutItem(), 0, 2)
		.Add(fAntialiasingMenuField->CreateMenuBarLayoutItem(), 1, 2)

		.Add(fAverageWeightControl, 0, 3, 2)

		.Add(BSpaceLayoutItem::CreateGlue(), 0, 4, 2)

		.SetInsets(10, 10, 10, 10)
	);

	// unavailable info pane
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BSpaceLayoutItem::CreateGlue())
		.Add(subpixelAntialiasingDisabledLabel)
		.Add(BSpaceLayoutItem::CreateGlue())
	);

	_SetCurrentAntialiasing();
	_SetCurrentHinting();
	_SetCurrentAverageWeight();

	// TODO: Remove once these two lines once the entire window uses
	// layout management.
	MoveTo(rect.LeftTop());
	ResizeTo(rect.Width(), rect.Height());

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	cardLayout->SetVisibleItem(0L);
#else
	cardLayout->SetVisibleItem(1L);
#endif
}


AntialiasingSettingsView::~AntialiasingSettingsView()
{
}


void
AntialiasingSettingsView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fAntialiasingMenu->SetTargetForItems(this);
	fHintingMenu->SetTargetForItems(this);
	fAverageWeightControl->SetTarget(this);
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
			fAverageWeightControl->SetEnabled(fCurrentSubpixelAntialiasing);

			set_subpixel_antialiasing(fCurrentSubpixelAntialiasing);

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

			set_hinting(fCurrentHinting);

			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case kMsgSetAverageWeight:
		{
			int32 averageWeight = fAverageWeightControl->Value();
			if (averageWeight == fCurrentAverageWeight)
				break;

			fCurrentAverageWeight = averageWeight;

			set_average_weight(fCurrentAverageWeight);

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
	fAntialiasingMenu = new BPopUpMenu("Antialiasing menu");

	BMessage* message = new BMessage(kMsgSetAntialiasing);
	message->AddBool("antialiasing", false);

	BMenuItem* item = new BMenuItem(kGrayscaleLabel, message);

	fAntialiasingMenu->AddItem(item);
	
	message = new BMessage(kMsgSetAntialiasing);
	message->AddBool("antialiasing", true);

	item = new BMenuItem(kSubpixelLabel, message);

	fAntialiasingMenu->AddItem(item);
}


void
AntialiasingSettingsView::_BuildHintingMenu()
{
	fHintingMenu = new BPopUpMenu("Hinting menu");

	BMessage* message = new BMessage(kMsgSetHinting);
	message->AddBool("hinting", false);

	BMenuItem* item = new BMenuItem(kNoHintingLabel, message);

	fHintingMenu->AddItem(item);
	
	message = new BMessage(kMsgSetHinting);
	message->AddBool("hinting", true);

	item = new BMenuItem(kFullHintingLabel, message);

	fHintingMenu->AddItem(item);
}


void
AntialiasingSettingsView::_SetCurrentAntialiasing()
{
	BMenuItem *item = fAntialiasingMenu->FindItem(
		fCurrentSubpixelAntialiasing ? kSubpixelLabel : kGrayscaleLabel);
	if (item != NULL)
		item->SetMarked(true);
	if (fCurrentSubpixelAntialiasing)
		fAverageWeightControl->SetEnabled(true);
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
	fAverageWeightControl->SetValue(fCurrentAverageWeight);
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

	set_subpixel_antialiasing(fCurrentSubpixelAntialiasing);
	set_hinting(fCurrentHinting);
	set_average_weight(fCurrentAverageWeight);

	_SetCurrentAntialiasing();
	_SetCurrentHinting();
	_SetCurrentAverageWeight();
}
