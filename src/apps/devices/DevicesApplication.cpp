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
#include <LayoutBuilder.h>
#include <TextView.h>

#include "DevicesView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DevicesApplication"

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
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS  | B_AUTO_UPDATE_SIZE_LIMITS
			| B_QUIT_ON_WINDOW_CLOSE)
{
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(0)
		.Add(fDevicesView = new DevicesView());
	GetLayout()->SetExplicitMinSize(BSize(600, 300));
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
