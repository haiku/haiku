/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include <Alert.h>
#include <Alignment.h>
#include <Application.h>
#include <Button.h>
#include <CardLayout.h>
#include <CardView.h>
#include <Catalog.h>
#include <Control.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <SplitView.h>
#include <Screen.h>
#include <stdio.h>


#include "InputConstants.h"
#include "InputDeviceView.h"
#include "InputMouse.h"
#include "InputTouchpadPref.h"
#include "InputWindow.h"
#include "MouseSettings.h"
#include "SettingsView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputWindow"


InputWindow::InputWindow(BRect rect)
	:
	BWindow(rect, B_TRANSLATE_SYSTEM_NAME("Input"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
{
	FindDevice();

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 10)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(fDeviceListView)
		.AddGroup(B_VERTICAL, 0)
			.Add(fCardView)
		.End();
}

void
InputWindow::MessageReceived(BMessage* message)
{
	int32 name = message->GetInt32("index", 0);

	switch (message->what) {

		case ITEM_SELECTED:
		{
			fCardView->CardLayout()->SetVisibleItem(name);
		}
		case kMsgMouseType:
		case kMsgMouseMap:
		case kMsgMouseFocusMode:
		case kMsgFollowsMouseMode:
		case kMsgAcceptFirstClick:
		case kMsgDoubleClickSpeed:
		case kMsgMouseSpeed:
		case kMsgAccelerationFactor:
		case kMsgDefaults:
		case kMsgRevert:
		{
			PostMessage(message,
				fCardView->CardLayout()->VisibleItem()->View());
			break;
		}
		case SCROLL_AREA_CHANGED:
		case SCROLL_CONTROL_CHANGED:
		case TAP_CONTROL_CHANGED:
		case DEFAULT_SETTINGS:
		case REVERT_SETTINGS:
		{
			PostMessage(message,
				fCardView->CardLayout()->VisibleItem()->View());
			break;
		}
		case kMsgSliderrepeatrate:
		case kMsgSliderdelayrate:
		{
			PostMessage(message,
				fCardView->CardLayout()->VisibleItem()->View());
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

status_t
InputWindow::FindDevice()
{
	BList devList;
	status_t status = get_input_devices(&devList);
	if (status != B_OK)
		return status;

	int32 i = 0;

	fDeviceListView = new DeviceListView(B_TRANSLATE("Device List"));
	fCardView = new BCardView();

	while (true) {
		BInputDevice* dev = (BInputDevice*)devList.ItemAt(i);
		if (dev == NULL) {
			break;
		}
		i++;

		BString name = dev->Name();

		if (dev->Type() == B_POINTING_DEVICE
			&& name.FindFirst("Touchpad") >= 0) {

			fTouchPad = dev;
			TouchpadPrefView* view = new TouchpadPrefView(dev);
			fCardView->AddChild(view);
			fDeviceListView->fDeviceList->AddItem(new BStringItem(name));
		} else if (dev->Type() == B_POINTING_DEVICE) {

			fMouse = dev;
			InputMouse* view = new InputMouse(dev);
			fCardView->AddChild(view);
			fDeviceListView->fDeviceList->AddItem(new BStringItem(name));
		} else if (dev->Type() == B_KEYBOARD_DEVICE) {

			fKeyboard = dev;
			InputKeyboard* view = new InputKeyboard(dev);
			fCardView->AddChild(view);
			fDeviceListView->fDeviceList->AddItem(new BStringItem(name));
		} else {
			delete dev;
		}
	}
	return B_ENTRY_NOT_FOUND;
}
