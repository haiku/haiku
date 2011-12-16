/*
 * Copyright 2003-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval,
 *		Axel Dörfler (axeld@pinc-software.de)
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Brecht Machiels (brecht@mos6581.org)
 */


#include "SettingsView.h"

#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Debug.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <InterfaceDefs.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <Window.h>

#include "MouseConstants.h"
#include "MouseSettings.h"
#include "MouseView.h"


static int32
mouse_mode_to_index(mode_mouse mode)
{
	switch (mode) {
		case B_NORMAL_MOUSE:
		default:
			return 0;
		case B_CLICK_TO_FOCUS_MOUSE:
			return 1;
		case B_FOCUS_FOLLOWS_MOUSE:
			return 2;
	}
}

static int32
focus_follows_mouse_mode_to_index(mode_focus_follows_mouse mode)
{
	switch (mode) {
		case B_NORMAL_FOCUS_FOLLOWS_MOUSE:
		default:
			return 0;
		case B_WARP_FOCUS_FOLLOWS_MOUSE:
			return 1;
		case B_INSTANT_WARP_FOCUS_FOLLOWS_MOUSE:
			return 2;
	}
}


//	#pragma mark -

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "SettingsView"

SettingsView::SettingsView(MouseSettings& settings)
	: BBox("main_view"),
	fSettings(settings)
{
	// Add the "Mouse Type" pop up menu
	fTypeMenu = new BPopUpMenu("unknown");
	fTypeMenu->AddItem(new BMenuItem(B_TRANSLATE("1-Button"),
		new BMessage(kMsgMouseType)));
	fTypeMenu->AddItem(new BMenuItem(B_TRANSLATE("2-Button"),
		new BMessage(kMsgMouseType)));
	fTypeMenu->AddItem(new BMenuItem(B_TRANSLATE("3-Button"),
		new BMessage(kMsgMouseType)));

	BMenuField* typeField = new BMenuField(B_TRANSLATE("Mouse type:"),
		fTypeMenu);
	typeField->SetAlignment(B_ALIGN_RIGHT);

	// Create the "Double-click speed slider...
	fClickSpeedSlider = new BSlider("double_click_speed",
		B_TRANSLATE("Double-click speed"), new BMessage(kMsgDoubleClickSpeed),
		0, 1000, B_HORIZONTAL);
	fClickSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fClickSpeedSlider->SetHashMarkCount(5);
	fClickSpeedSlider->SetLimitLabels(B_TRANSLATE("Slow"),
		B_TRANSLATE("Fast"));

	// Create the "Mouse Speed" slider...
	fMouseSpeedSlider = new BSlider("mouse_speed", B_TRANSLATE("Mouse speed"),
		new BMessage(kMsgMouseSpeed), 0, 1000, B_HORIZONTAL);
	fMouseSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fMouseSpeedSlider->SetHashMarkCount(7);
	fMouseSpeedSlider->SetLimitLabels(B_TRANSLATE("Slow"),
		B_TRANSLATE("Fast"));

	// Create the "Mouse Acceleration" slider...
	fAccelerationSlider = new BSlider("mouse_acceleration",
		B_TRANSLATE("Mouse acceleration"),
		new BMessage(kMsgAccelerationFactor), 0, 1000, B_HORIZONTAL);
	fAccelerationSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fAccelerationSlider->SetHashMarkCount(5);
	fAccelerationSlider->SetLimitLabels(B_TRANSLATE("Slow"),
		B_TRANSLATE("Fast"));

	// Mouse image...
	fMouseView = new MouseView(fSettings);

	// Create the "Double-click test area" text box...
	BTextControl* doubleClickTextControl = new BTextControl(NULL,
		B_TRANSLATE("Double-click test area"), NULL);
	doubleClickTextControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_CENTER);

	// Add the "Mouse focus mode" pop up menu
	fFocusMenu = new BPopUpMenu(B_TRANSLATE("Click to focus and raise"));

	const char *focusLabels[] = {B_TRANSLATE_MARK("Click to focus and raise"),
		B_TRANSLATE_MARK("Click to focus"),
		B_TRANSLATE_MARK("Focus follows mouse")};
	const mode_mouse focusModes[] = {B_NORMAL_MOUSE, B_CLICK_TO_FOCUS_MOUSE,
										B_FOCUS_FOLLOWS_MOUSE};

	for (int i = 0; i < 3; i++) {
		BMessage* message = new BMessage(kMsgMouseFocusMode);
		message->AddInt32("mode", focusModes[i]);

		fFocusMenu->AddItem(new BMenuItem(B_TRANSLATE_NOCOLLECT(focusLabels[i]),
			message));
	}

	BMenuField* focusField = new BMenuField(B_TRANSLATE("Focus mode:"),
		fFocusMenu);
	focusField->SetAlignment(B_ALIGN_RIGHT);

	// Add the "Focus follows mouse mode" pop up menu
	fFocusFollowsMouseMenu = new BPopUpMenu(B_TRANSLATE("Normal"));

	const char *focusFollowsMouseLabels[] = {B_TRANSLATE_MARK("Normal"),
		B_TRANSLATE_MARK("Warp"), B_TRANSLATE_MARK("Instant warp")};
	const mode_focus_follows_mouse focusFollowsMouseModes[] =
		{B_NORMAL_FOCUS_FOLLOWS_MOUSE, B_WARP_FOCUS_FOLLOWS_MOUSE,
			B_INSTANT_WARP_FOCUS_FOLLOWS_MOUSE};

	for (int i = 0; i < 3; i++) {
		BMessage* message = new BMessage(kMsgFollowsMouseMode);
		message->AddInt32("mode_focus_follows_mouse",
			focusFollowsMouseModes[i]);

		fFocusFollowsMouseMenu->AddItem(new BMenuItem(
			B_TRANSLATE_NOCOLLECT(focusFollowsMouseLabels[i]), message));
	}

	BMenuField* focusFollowsMouseField = new BMenuField(
		"Focus follows mouse mode:", fFocusFollowsMouseMenu);
	focusFollowsMouseField->SetAlignment(B_ALIGN_RIGHT);

	// Add the "Click-through" check box
	fAcceptFirstClickBox = new BCheckBox(B_TRANSLATE("Accept first click"),
		new BMessage(kMsgAcceptFirstClick));

	// dividers
	BBox* hdivider = new BBox(
		BRect(0, 0, 1, 1), B_EMPTY_STRING, B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);
	hdivider->SetExplicitMaxSize(BSize(1, B_SIZE_UNLIMITED));

	BBox* vdivider = new BBox(
		BRect(0, 0, 1, 1), B_EMPTY_STRING, B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER);
	vdivider->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	// Build the layout
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.AddGroup(B_HORIZONTAL, 10)
			.AddGroup(B_VERTICAL, 10, 1)
				.AddGroup(B_HORIZONTAL, 10)
					.AddGlue()
					.Add(typeField)
					.AddGlue()
				.End()
				.AddGlue()
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
					.AddGlue()
					.Add(fMouseView)
					.AddGlue()
				)
				.AddGlue()
				.Add(doubleClickTextControl)
			.End()
			.Add(hdivider)
			.AddGroup(B_VERTICAL, 5, 3)
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
					.Add(fClickSpeedSlider)
				)
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
					.Add(fMouseSpeedSlider)
				)
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
					.Add(fAccelerationSlider)
				)
			.End()
		.End()
		.Add(vdivider)
		.AddGroup(B_HORIZONTAL, 10)
			.Add(focusField)
			.AddGlue()
			.AddGroup(B_VERTICAL, 0)
				.Add(fAcceptFirstClickBox)
			.End()
		.End()
		.SetInsets(5, 5, 5, 5)
	);
}


SettingsView::~SettingsView()
{

}


void
SettingsView::AttachedToWindow()
{
	UpdateFromSettings();


}


void
SettingsView::SetMouseType(int32 type)
{
	fMouseView->SetMouseType(type);
}


void
SettingsView::MouseMapUpdated()
{
	fMouseView->MouseMapUpdated();
}


void
SettingsView::UpdateFromSettings()
{
	int32 value = int32(fSettings.ClickSpeed() / 1000);
		// slow = 1000000, fast = 0
	fClickSpeedSlider->SetValue(value);

	value = int32((log(fSettings.MouseSpeed() / 8192.0) / log(2)) * 1000 / 6);
		// slow = 8192, fast = 524287
	fMouseSpeedSlider->SetValue(value);

	value = int32(sqrt(fSettings.AccelerationFactor() / 16384.0) * 1000 / 4);
		// slow = 0, fast = 262144
	fAccelerationSlider->SetValue(value);

	BMenuItem* item = fTypeMenu->ItemAt(fSettings.MouseType() - 1);
	if (item != NULL)
		item->SetMarked(true);

	fMouseView->SetMouseType(fSettings.MouseType());

	item = fFocusMenu->ItemAt(mouse_mode_to_index(fSettings.MouseMode()));
	if (item != NULL)
		item->SetMarked(true);

	item = fFocusFollowsMouseMenu->ItemAt(
		focus_follows_mouse_mode_to_index(fSettings.FocusFollowsMouseMode()));
	if (item != NULL)
		item->SetMarked(true);

	fFocusFollowsMouseMenu->SetEnabled(fSettings.MouseMode()
		== B_FOCUS_FOLLOWS_MOUSE);

	fAcceptFirstClickBox->SetValue(fSettings.AcceptFirstClick()
		? B_CONTROL_ON : B_CONTROL_OFF);
	fAcceptFirstClickBox->SetEnabled(fSettings.MouseMode()
		!= B_FOCUS_FOLLOWS_MOUSE);
}

