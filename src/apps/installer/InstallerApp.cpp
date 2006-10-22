/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextView.h>
#include "InstallerApp.h"

const char *APP_SIG		= "application/x-vnd.Haiku-Installer";

#define EULA_TEXT "HAIKU INC - END USER LICENSE AGREEMENT\n\n\
NOTICE: READ THIS BEFORE INSTALLING OR USING Haiku\n\
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n\
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
"

int main(int, char **)
{
	InstallerApp theApp;
	theApp.Run();
	return 0;
}

InstallerApp::InstallerApp()
	: BApplication(APP_SIG)
{
	BRect windowFrame(0,0,INSTALLER_RIGHT, 160);
	BRect frame = BScreen().Frame();
	windowFrame.OffsetBy((frame.Width() - windowFrame.Width())/2, 
		frame.Height()/2 - windowFrame.Height()/4 - 113);
	fWindow = new InstallerWindow(windowFrame);

	// show the EULA
	BAlert *alert = new BAlert("", EULA_TEXT, " Disagree ", " Agree ", NULL,
		 B_WIDTH_FROM_WIDEST, B_EMPTY_ALERT);
	BTextView *alertView = alert->TextView();
	alertView->SetViewColor(255,255,255);
	BView *parent = alertView->Parent();
	alertView->RemoveSelf();
	alertView->MoveBy(3,7);
	alertView->ResizeTo(460,283);
	alertView->SetResizingMode(B_FOLLOW_ALL_SIDES);
	alert->ResizeTo(500,350);
	BScrollView *scroll = new BScrollView("", alertView, B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS, false, true, B_FANCY_BORDER);
	parent->AddChild(scroll);
	BRect alertFrame = alert->Frame();
	alertFrame.OffsetTo((frame.Width() - alertFrame.Width())/2,
		(frame.Height() - alertFrame.Height())/2);
	alert->MoveTo(alertFrame.LeftTop());
	if (alert->Go()!=1)
		PostMessage(B_QUIT_REQUESTED);
}

void
InstallerApp::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Installer\n"
		"\twritten by Jérôme Duval\n"
		"\tCopyright 2005, Haiku.\n\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE); 			
	view->SetFontAndColor(0, 14, &font);

	alert->Go();

}


void
InstallerApp::ReadyToRun()
{
	
}

