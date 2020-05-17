/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#include "Input.h"

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>

#include "InputConstants.h"
#include "InputMouse.h"
#include "InputWindow.h"
#include "MouseSettings.h"
#include "MouseView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InputApplication"

const char* kSignature = "application/x-vnd.Haiku-Input";


InputApplication::InputApplication()
	:
	BApplication(kSignature),
	fIcons()
{
	BRect rect(0, 0, 600, 500);
	InputWindow* window = new InputWindow(rect);
	DeviceListItemView::SetIcons(&fIcons);
	window->Show();
}

void
InputApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
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
			fWindow->PostMessage(message);
			break;
		}
		case SCROLL_AREA_CHANGED:
		case SCROLL_CONTROL_CHANGED:
		case TAP_CONTROL_CHANGED:
		case DEFAULT_SETTINGS:
		case REVERT_SETTINGS:
		{
			fWindow->PostMessage(message);
			break;
		}
		case kMsgSliderrepeatrate:
		case kMsgSliderdelayrate:
		{
			fWindow->PostMessage(message);
			break;
		}
	default:
		BApplication::MessageReceived(message);
	}
};


int
main(int /*argc*/, char** /*argv*/)
{
	InputApplication app;
	app.Run();

	return 0;
}
