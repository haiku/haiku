/*
	NodeHarnessApp.cpp

	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "NodeHarnessApp.h"
#include "NodeHarnessWin.h"

NodeHarnessApp::NodeHarnessApp(const char *signature)
	: BApplication(signature)
{
}

void 
NodeHarnessApp::ReadyToRun()
{
	BWindow* win = new NodeHarnessWin(BRect(100, 200, 210, 330), "ToneProducer");
	win->Show();
}
