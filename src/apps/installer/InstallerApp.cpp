/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <TextView.h>
#include "InstallerApp.h"

const char *APP_SIG		= "application/x-vnd.haiku-Installer";

int main(int, char **)
{
	InstallerApp theApp;
	theApp.Run();
	return 0;
}

InstallerApp::InstallerApp()
	: BApplication(APP_SIG)
{
	BRect windowFrame(0,0,332,160);
	windowFrame.OffsetBy(154,87);
	fWindow = new InstallerWindow(windowFrame);
}

void
InstallerApp::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Installer\n"
		"\twritten by JÃ©rÃ´me Duval\n"
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

