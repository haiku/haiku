/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "InputMouse.h"

#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Debug.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <Slider.h>
#include <TextControl.h>
#include <TranslationUtils.h>
#include <Window.h>

#include "InputConstants.h"
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

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsView"

SettingsView::SettingsView(MouseSettings& settings)
	: BBox("main_view"),
	fSettings(settings)
{
	if (fSettings.MouseType() > 6)
		debugger("Mouse type is invalid");

	// Add the "Mouse Type" pop up menu
	fTypeMenu = new BOptionPopUp("type", B_TRANSLATE("Mouse type:"),
		new BMessage(kMsgMouseType));
	fTypeMenu->AddOption(B_TRANSLATE("1-Button"), 1);
	fTypeMenu->AddOption(B_TRANSLATE("2-Button"), 2);
	fTypeMenu->AddOption(B_TRANSLATE("3-Button"), 3);
	fTypeMenu->AddOption(B_TRANSLATE("4-Button"), 4);
	fTypeMenu->AddOption(B_TRANSLATE("5-Button"), 5);
	fTypeMenu->AddOption(B_TRANSLATE("6-Button"), 6);

	// Create the "Double-click speed slider...
	fClickSpeedSlider = new BSlider("double_click_speed",
		B_TRANSLATE("Double-click speed"), new BMessage(kMsgDoubleClickSpeed),
		0, 1000, B_HORIZONTAL);
	fClickSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fClickSpeedSlider->SetHashMarkCount(7);

	// Create the "Mouse Speed" slider...
	fMouseSpeedSlider = new BSlider("mouse_speed", B_TRANSLATE("Mouse speed"),
		new BMessage(kMsgMouseSpeed), 0, 1000, B_HORIZONTAL);
	fMouseSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fMouseSpeedSlider->SetHashMarkCount(7);

	// Create the "Mouse Acceleration" slider...
	fAccelerationSlider = new BSlider("mouse_acceleration",
		B_TRANSLATE("Mouse acceleration"),
		new BMessage(kMsgAccelerationFactor), 0, 1000, B_HORIZONTAL);
	fAccelerationSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fAccelerationSlider->SetHashMarkCount(7);

	fMouseView = new MouseView(fSettings);

	// Create the "Double-click test area" text box...
	const char* label = B_TRANSLATE("Double-click test area");
	BTextControl* doubleClickTextControl = new BTextControl(NULL,
		label, NULL);
	doubleClickTextControl->SetAlignment(B_ALIGN_LEFT, B_ALIGN_CENTER);
	doubleClickTextControl->SetExplicitMinSize(
		BSize(StringWidth(label), B_SIZE_UNSET));

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
	focusField->SetAlignment(B_ALIGN_LEFT);

	// Add the "Focus follows mouse mode" pop up menu
	fFocusFollowsMouseMenu = new BPopUpMenu(B_TRANSLATE("Normal"));

	const char *focusFollowsMouseLabels[] = {B_TRANSLATE_MARK("Normal"),
		B_TRANSLATE_MARK("Warp"), B_TRANSLATE_MARK("Instant warp")};
	const mode_focus_follows_mouse focusFollowsMouseModes[]
		= {B_NORMAL_FOCUS_FOLLOWS_MOUSE, B_WARP_FOCUS_FOLLOWS_MOUSE,
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

	// Build the layout

	// Layout is :
	// A | B
	// -----
	//   C

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		// Horizontal : A|B
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)

			// Vertical block A: mouse type/view/test
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
				.Add(fTypeMenu)
				.AddGroup(B_HORIZONTAL, 0)
					.AddGlue()
					.Add(fMouseView)
					.AddGlue()
					.End()
				.AddGlue()
				.Add(doubleClickTextControl)
				.End()
			.Add(new BSeparatorView(B_VERTICAL))

			// Vertical block B: speed settings
			.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING, 3)
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fClickSpeedSlider)
					.End()
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fMouseSpeedSlider)
					.End()
				.AddGroup(B_HORIZONTAL, 0)
					.Add(fAccelerationSlider)
					.End()
				.End()
			.End()
		.AddStrut(B_USE_DEFAULT_SPACING)

		// Horizontal Block C: focus mode
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(focusField)
			.AddGlue()
			.AddGroup(B_VERTICAL, 0)
				.Add(fAcceptFirstClickBox)
				.End()
			.End();

	SetBorder(B_NO_BORDER);
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
	if (type > 6)
		debugger("Mouse type is invalid");
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

	fTypeMenu->SelectOptionFor(fSettings.MouseType());
	fMouseView->SetMouseType(fSettings.MouseType());

	BMenuItem* item = fFocusMenu->ItemAt(
		mouse_mode_to_index(fSettings.MouseMode()));
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
