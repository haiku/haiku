/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Pieter Panman
 */

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <TextView.h>

#include "DevicesView.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "DevicesApplication"

class DevicesApplication : public BApplication {
public:
								DevicesApplication();
	virtual	void				AboutRequested();
	static	void				ShowAbout();
};


class DevicesWindow : public BWindow {
public:
								DevicesWindow();
	virtual	void				MessageReceived(BMessage* message);
private:
			DevicesView*		fDevicesView;
};


DevicesApplication::DevicesApplication()
	:
	BApplication("application/x-vnd.Haiku-Devices")
{
	DevicesWindow* window = new DevicesWindow();
	window->CenterOnScreen();
	window->Show();
}


void
DevicesApplication::AboutRequested()
{
	ShowAbout();
}


void
DevicesApplication::ShowAbout()
{
	BAlert* alert = new BAlert(B_TRANSLATE("About"), B_TRANSLATE("Devices\n"
		"\twritten by Pieter Panman\n"
		"\n"
		"\tBased on listdev by Jérôme Duval\n"
		"\tand the previous Devices preference\n"
		"\tby Jérôme Duval and Sikosis\n"
		"\tCopyright 2009, Haiku, Inc.\n"), B_TRANSLATE("OK"));
	BTextView* view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 7, &font);

	alert->Go();
}


DevicesWindow::DevicesWindow()
	:
	BWindow(BRect(50, 50, 750, 550), B_TRANSLATE("Devices"), 
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS 
		| B_AUTO_UPDATE_SIZE_LIMITS
		| B_QUIT_ON_WINDOW_CLOSE)
{
	float minWidth;
	float maxWidth;
	float minHeight;
	float maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidth = 600;
	minHeight = 300;
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
	fDevicesView = new DevicesView(Bounds());
	AddChild(fDevicesView);
}


void
DevicesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgRefresh:
		case kMsgReportCompatibility:
		case kMsgGenerateSysInfo:
		case kMsgSelectionChanged:
		case kMsgOrderCategory:
		case kMsgOrderConnection:
			fDevicesView->MessageReceived(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


int
main()
{
	DevicesApplication app;
	app.Run();
	return 0;
}
