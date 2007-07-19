/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood, leavengood@gmail.com
 */


#include "JoyWin.h"
#include "MessagedItem.h"
#include "MessageWin.h"
#include "CalibWin.h"
#include "Global.h"

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <ListView.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <Application.h>
#include <View.h>


JoyWin::JoyWin(BRect frame, const char *title, window_look look, 
	window_feel feel, uint32 flags, uint32 workspace = B_CURRENT_WORKSPACE)
	: BWindow(frame, title, look, feel, flags, workspace)
{
	fProbeButton = new BButton(BRect(15.00, 260.00, 115.00, 285.00), "ProbeButton", "Probe", NULL);
	fCalibrateButton = new BButton(BRect(270.00, 260.00, 370.00, 285.00), "CalibrateButton", "Calibrate", NULL);

	fGamePortL = new BListView(BRect(15.00, 30.00, 145.00, 250.00), "gamePort");
	fGamePortL->SetSelectionMessage(new BMessage(PORT_SELECTED));
	fGamePortL->SetInvocationMessage(new BMessage(PORT_INVOKE));
 
	fConControllerL = new BListView(BRect(175.00,30.00,370.00,250.00),"conController");
	fConControllerL->SetSelectionMessage(new BMessage(JOY_SELECTED));
	fConControllerL->SetInvocationMessage(new BMessage(JOY_INVOKE));

	fGamePortS = new BStringView(BRect(15, 5, 160, 25), "gpString", "Game Port");
	fGamePortS->SetFont(be_bold_font);
	fConControllerS = new BStringView(BRect(170, 5, 330, 25), "ccString", "Connected Controller");
	fConControllerS->SetFont(be_bold_font);

	fCheckbox = new BCheckBox(BRect(131.00, 260.00, 227.00, 280.00), "Disabled", "Disabled", NULL);
	fBox = new BBox( Bounds(),"box", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE, 
		B_PLAIN_BORDER);
	fBox->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Adding object
	fBox->AddChild(fCheckbox);

	fBox->AddChild(fGamePortS);
	fBox->AddChild(fConControllerS);

	// Add listViews with their scrolls
	fBox->AddChild(new BScrollView("PortScroll", fGamePortL, 
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, false, true));
	fBox->AddChild(new BScrollView("ConScroll", fConControllerL, 
		B_FOLLOW_ALL, B_WILL_DRAW, false, true));

	fBox->AddChild(fProbeButton);
	fBox->AddChild(fCalibrateButton);

	AddChild(fBox);

	// This resizable feature is not in R5 but.
	// making horizontal resizing allows 
	// long joysticks names to be seen completelly
	SetSizeLimits(400, 600, Bounds().Height(), Bounds().Height());

	/* Add all the devices */	 
	AddDevices();

	/* Add the joysticks specifications */	 
	AddJoysticks();
}


void JoyWin::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
		case PORT_SELECTED:
			break;

		case PORT_INVOKE:		 
			// Do we have any port?

			// And some joysticks ?				

			// Probe!
			PerformProbe();

			break;

		case JOY_SELECTED:
			break;

		case JOY_INVOKE:

			// Do we have a selected definition?

			// And a suitale selected port?

			// Calibrate it!
			Calibrate();

			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}	
}


bool JoyWin::QuitRequested()
{
	// Apply changes and save configurations etc etc

	be_app->PostMessage(B_QUIT_REQUESTED);

	return BWindow::QuitRequested();
}


/* Initialization */
status_t JoyWin::AddDevices()
{
	char buf[BEOS_NAME_LENGTH];
	int devId = 0;
	MessagedItem* device;
	BMessage*	 message;
	BString		 str;
	 
	while (!fJoystick.GetDeviceName(devId, buf, BEOS_NAME_LENGTH)) {
		 message = new BMessage(PORT_SELECTED);
		 message->AddString("devname", buf);
		 // NOTE: Adding the index in the list might be useful.
	
		// TODO: Change it with leaf path
		str.SetTo(buf);
		//str = str.Remove(0, str.FindLast('/') );
	
		device = new MessagedItem(str.String(), message);
		fGamePortL->AddItem(device);
	
		devId++;
	}
	
	/* TODO: Add the Disabled devices */
	 
	/* We do not have any Joystick dev */
	if (devId == 0) {
		// keep track of it
		// Alert it

		return B_ERROR;
	}

	return B_OK;
}


status_t JoyWin::AddJoysticks() 
{
	return B_OK;
}


/* Management */
status_t JoyWin::Calibrate() 
{
	CalibWin* calibw;

	calibw = new CalibWin(BRect(100, 100, 500, 400), "Calibrate", 
		B_DOCUMENT_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE);

	calibw->Show();

	return B_OK;
}


status_t JoyWin::PerformProbe() 
{
	MessageWin* mesgw;

	// [...]
	
	mesgw = new MessageWin(Frame(),"Probing",
		B_MODAL_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE );
			
	mesgw->Show();
	mesgw->SetText("Looking for: X in port Y ...");
	return B_OK;
}


/* Configuration */
status_t JoyWin::ApplyChanges() 
{
	return B_OK;
}


/* Configuration */
status_t JoyWin::GetSettings() 
{
	return B_OK;
}

