/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2005, Jérôme DUVAL.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "InstallerApp.h"

#include <Alert.h>
#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <Screen.h>
#include <ScrollView.h>
#include <TextView.h>


static const uint32 kMsgAgree = 'agre';

static const char* kEULAText =
"NOTICE: READ THIS BEFORE INSTALLING OR USING HAIKU\n\n"

"Copyright " B_UTF8_COPYRIGHT " 2001-2009 The Haiku Project. All rights "
"reserved. The copyright to the Haiku code is property of Haiku, Inc. or of "
"the respective authors where expressly noted in the source.\n\n"

"Permission is hereby granted, free of charge, to any person obtaining a "
"copy of this software and associated documentation files (the \"Software\"), "
"to deal in the Software without restriction, including without limitation "
"the rights to use, copy, modify, merge, publish, distribute, sublicense, "
"and/or sell copies of the Software, and to permit persons to whom the "
"Software is furnished to do so, subject to the following conditions:\n\n"
"The above copyright notice and this permission notice shall be included in "
"all copies or substantial portions of the Software.\n\n"

"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS "
"OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING "
"FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS "
"IN THE SOFTWARE.";


int main(int, char **)
{
	InstallerApp theApp;
	theApp.Run();
	return 0;
}


InstallerApp::InstallerApp()
	: BApplication("application/x-vnd.Haiku-Installer")
{
}


void
InstallerApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgAgree:
			fEULAWindow->Lock();
			fEULAWindow->Quit();
			new InstallerWindow();
			break;

		default:
			BApplication::MessageReceived(message);
	}
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
#if 1
	// Show the EULA first.
	BTextView* textView = new BTextView("eula", be_plain_font, NULL,
		B_WILL_DRAW);
	textView->SetInsets(10, 10, 10, 10);
	textView->MakeEditable(false);
	textView->MakeSelectable(false);
	textView->SetText(kEULAText);

	BScrollView* scrollView = new BScrollView("eulaScroll",
		textView, B_WILL_DRAW, false, true);

	BButton* disagreeButton = new BButton("Disagree",
		new BMessage(B_QUIT_REQUESTED));
	disagreeButton->SetTarget(this);

	BButton* agreeButton = new BButton("Agree",
		new BMessage(kMsgAgree));
	agreeButton->SetTarget(this);
	agreeButton->MakeDefault(true);

	BRect eulaFrame = BRect(0, 0, 600, 450);
	fEULAWindow = new BWindow(eulaFrame, "EULA",
		B_MODAL_WINDOW, B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
			| B_AUTO_UPDATE_SIZE_LIMITS);

	fEULAWindow->SetLayout(new BGroupLayout(B_HORIZONTAL));
	fEULAWindow->AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(scrollView)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.AddGlue()
			.Add(disagreeButton)
			.Add(agreeButton)
		)
		.SetInsets(10, 10, 10, 10)
	);

	BRect frame = BScreen().Frame();
	fEULAWindow->MoveTo(frame.left + (frame.Width() - eulaFrame.Width()) / 2,
		frame.top + (frame.Height() - eulaFrame.Height()) / 2);

	fEULAWindow->Show();
#else
	// Show the installer window without EULA.
	new InstallerWindow();
#endif
}

