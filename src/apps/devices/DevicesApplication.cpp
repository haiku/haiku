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


DevicesWindow::DevicesWindow()
	:
	BWindow(BRect(50, 50, 750, 550), B_TRANSLATE_SYSTEM_NAME("Devices"),
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
