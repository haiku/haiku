//
// StickIt
// File: jwindow.cpp
// Joystick window.
// Sampel code used in "Getting a Grip on BJoystick" by Eric Shepherd
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>

#include <Application.h>
#include <Joystick.h>
#include <String.h>
#include <StringView.h>
#include <Box.h>
#include <ListView.h>
#include <ScrollView.h>
#include <ListItem.h>

#define SPACE 10

#include "JoystickWindow.h"

#define SELECTED  'sele'
#define INVOKE    'invo'

#include "StickItWindow.h"

StickItWindow::StickItWindow(BRect frame)
	: BWindow(frame, "StickIt", B_TITLED_WINDOW, 0)
{
	frame = Bounds();
	frame.InsetBy(5, 5);
	BBox* box = new BBox(frame, NULL, B_FOLLOW_ALL_SIDES);


	// Allocate object
	BView* view = new BView(Bounds(), "", B_FOLLOW_ALL_SIDES, 0);
	view->SetViewColor(216, 216, 216);
	view->SetLowColor(216, 216, 216);

	BRect rectString = BRect(frame.left, frame.top-10, frame.right -30, 30);
	BStringView* stringview1 = new BStringView(rectString,"StringView1",
		"This list, lists action that StickIt makes.");

	BRect rect = BRect(rectString.left, rectString.bottom + SPACE,
		rectString.right, rectString.bottom + SPACE + 200);
	fListView1 = new BListView(rect,"ListView1");

	rectString = BRect(rect.left, rect.bottom + SPACE, rect.right,
		rect.bottom + SPACE + 15);
	BStringView* stringview2 = new BStringView(rectString,"StringView2",
		"Choose Joystick below if any exists");

	rect = BRect(rectString.left, rectString.bottom + SPACE, rectString.right,
		Bounds().bottom -20);
	fListView2 = new BListView(rect,"ListView2");
	fListView2->SetSelectionMessage(new BMessage(SELECTED));
	fListView2->SetInvocationMessage(new BMessage(INVOKE));


	// Adding object
	box->AddChild(new BScrollView("fListView1", fListView1,
		B_FOLLOW_LEFT_RIGHT, 0, false, true));

	box->AddChild(new BScrollView("fListView2", fListView2,
		B_FOLLOW_ALL_SIDES, 0, false, true));

	box->AddChild(stringview1);
	box->AddChild(stringview2);
	view->AddChild(box);
	AddChild(view);

	fJoystick = new BJoystick;
	PickJoystick(fJoystick);
}


bool
StickItWindow::QuitRequested(void) {
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
StickItWindow::MessageReceived(BMessage *message)
{
//	message->PrintToStream();
	switch(message->what)
	{
		case INVOKE:
		case SELECTED:
		{
			int32 id = fListView2->CurrentSelection();
			BString temp1;
			if (id > -1) {
				char devName[B_OS_NAME_LENGTH];
				status_t err = fJoystick->GetDeviceName(id, devName);
				if (err == B_OK) {
					temp1 << "BJoystick::GetDeviceName(), id = " << id
						<< ", name = " << devName;
					temp1 = AddToList(fListView1, temp1.String());
					err = fJoystick->Open(devName);
					if (err != B_ERROR) {
						temp1 = AddToList(fListView1, "BJoystick::Open()");
						temp1 = AddToList(fListView1, "BJoystick::Open()");
						if(fJoystick->IsCalibrationEnabled())
							temp1 = AddToList(fListView1,
								"BJoystick::IsCalibrationEnabled() - True");
						else
							temp1 = AddToList(fListView1,
								"BJoystick::IsCalibrationEnabled() - False");
						fJoystickWindow = new JoystickWindow(fJoystick,
							BRect(50, 50, 405, 350));
						fJoystickWindow->Show();
					} else
						AddToList(fListView1,
							"No controller connected on that port. Try again.");
				} else
					AddToList(fListView1, "Can't use that stick.  Try again.");
			}
		break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


BString
StickItWindow::AddToList(BListView *bl, const char * str)
{
	bl->AddItem(new BStringItem(str));
	return BString ("");
}


// PickJoystick
// Present a list of all game controllers and let the user choose the one to
// use.  This will configure the "stick" object for that controller.
void
StickItWindow::PickJoystick(BJoystick *stick)
{
	int32 numDevices = stick->CountDevices();
	BString temp1("BJoystick::CountDevices()");
	temp1 << ", Num = " << numDevices;
	temp1 = AddToList(fListView1, temp1.String());
	status_t err = B_ERROR;
	if (numDevices) {
		temp1 << "There are " << numDevices
			<< " Joysticks device types available";
		temp1 = AddToList(fListView1, temp1.String());
		for (int32 i = 0; i < numDevices; i++) {
			char devName[B_OS_NAME_LENGTH];
			err = stick->GetDeviceName(i, devName);
			temp1 << "BJoystick::GetDeviceName(), id = " << i << ", name = "
				<< devName;
			temp1 = AddToList(fListView1, temp1.String());
			if (err == B_OK) {
				err = stick->Open(devName);
				temp1 = AddToList(fListView1, "BJoystick::Open()");
				int32 count = stick->CountSticks();
				temp1 << "BJoystick::CountSticks(), number of sticks = "
					<< count;
				temp1 = AddToList(fListView1, temp1.String());

				count = stick->CountAxes();
				temp1 << "BJoystick::CountAxes(), number of Axes = "
					<< count;
				temp1 = AddToList(fListView1, temp1.String());

				count = stick->CountButtons();
				temp1 << "BJoystick::CountButtons(), number of Buttons = "
					<< count;
				temp1 = AddToList(fListView1, temp1.String());

				count = stick->CountHats();
				temp1 << "BJoystick::CountHats(), number of Hats = "
					<< count;
				temp1 = AddToList(fListView1, temp1.String());

				count = stick->CountDevices();
				temp1 << "BJoystick::CountDevices(), number of Devices = "
					<< count;
				temp1 = AddToList(fListView1, temp1.String());

				if (err != B_ERROR) {
					BString name;
					err = stick->GetControllerModule(&name);
					temp1 << "BJoystick::GetControllerModule(), name = "
						<< name;
					temp1 = AddToList(fListView1, temp1.String());

					if (name == "Legacy") {
						bool b = stick->EnterEnhancedMode();
						if (b) {
							temp1 << "BJoystick::EnterEnhancedMode(), OK";
							temp1 = AddToList(fListView1, temp1.String());
						} else {
							temp1 << "BJoystick::EnterEnhancedMode(), Not OK";
							temp1 = AddToList(fListView1, temp1.String());
						}
					}

					err = stick->GetControllerName(&name);
					temp1 << "BJoystick::GetControllerName(), name = " << name;
					temp1 = AddToList(fListView1, temp1.String());
					if (err == B_OK) {
						stick->Close();
						temp1 = AddToList(fListView1, "BJoystick::Close()");
						temp1 << i+1 << " " << name.String();
						temp1 = AddToList(fListView2, temp1.String());
					} else {
						temp1 << "Error = " << strerror(err);
						temp1 = AddToList(fListView1, temp1.String());
						temp1 << "*** Can't get name of controller "
							<< devName;
						temp1 = AddToList(fListView1, temp1.String());
					}
				} else {
					temp1 << "Error = " << strerror(err) << "err nr = " << err;
					temp1 = AddToList(fListView1, temp1.String());
					temp1 << "No controller on " << devName;
					temp1 = AddToList(fListView1, temp1.String());
				}
			} else {
				temp1 << "Error = " << strerror(err);
				temp1 = AddToList(fListView1, temp1.String());
				temp1 << "*** Error while reading controller list.";
				temp1 = AddToList(fListView1, temp1.String());
			}
		}
	} else {
		temp1 << "Error = " << strerror(err);
		temp1 = AddToList(fListView1, temp1.String());
		temp1 = AddToList(fListView1, "*** No game ports detected.");
	}
}
