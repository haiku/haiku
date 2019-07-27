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
#include <TabView.h>
#include <stdio.h>


#include "InputWindow.h"
#include "MouseSettings.h"
#include "InputMouse.h"
#include "InputConstants.h"
#include "SettingsView.h"
#include "InputTouchpadPref.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputWindow"


InputWindow::InputWindow(BRect rect)
	:
	BWindow(rect, B_TRANSLATE_SYSTEM_NAME("Input"), B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
		| B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
{
	fInputMouse = new InputMouse();
	fTouchpadPrefView = new TouchpadPrefView(B_TRANSLATE("Touchpad"));
	fDeviceListView = new DeviceListView(B_TRANSLATE("Device List"));

	fCardView = new BCardView();
	fCardView->AddChild(fInputMouse);
	fCardView->AddChild(fTouchpadPrefView);

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
			PostMessage(message, fInputMouse);
			break;
		}
		case SCROLL_AREA_CHANGED:
		case SCROLL_CONTROL_CHANGED:
		case TAP_CONTROL_CHANGED:
		case DEFAULT_SETTINGS:
		case REVERT_SETTINGS:
		{
			PostMessage(message, fTouchpadPrefView);
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
