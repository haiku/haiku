// PatchApp.cpp
// ------------
// Implements the PatchBay application class and main().
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#include <Roster.h>
#include "PatchApp.h"
#include "PatchWin.h"
#include "TToolTip.h"

PatchApp::PatchApp()
	: BApplication("application/x-vnd.Be-DTS.PatchBay"), m_toolTip(NULL)
{
	m_toolTip = new TToolTip;
}

void PatchApp::ReadyToRun()
{
	new PatchWin;
}

void PatchApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case B_SOME_APP_ACTIVATED:
	case eToolTipStart:
	case eToolTipStop:
		if (m_toolTip) m_toolTip->PostMessage(msg);
		break;	
	default:
		BApplication::MessageReceived(msg);
		break;
	}
}

int main()
{
	PatchApp app;
	app.Run();
	return 0;
}

