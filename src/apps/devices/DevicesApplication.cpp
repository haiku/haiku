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
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Path.h>
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
	virtual	bool				QuitRequested();
private:
			status_t			_OpenSettings(BFile& file, uint32 mode);
			status_t			_LoadSettings(BMessage& settings);
			status_t			_SaveSettings();

			DevicesView*		fDevicesView;
};


DevicesApplication::DevicesApplication()
	:
	BApplication("application/x-vnd.Haiku-Devices")
{
	DevicesWindow* window = new DevicesWindow();
	window->Show();
}


DevicesWindow::DevicesWindow()
	:
	BWindow(BRect(50, 50, 960, 540), B_TRANSLATE_SYSTEM_NAME("Devices"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS  | B_AUTO_UPDATE_SIZE_LIMITS
			| B_QUIT_ON_WINDOW_CLOSE)
{
	BMessage settings;
	_LoadSettings(settings);

	int32 orderBy = ORDER_BY_CATEGORY;
	settings.FindInt32("sort_by", &orderBy);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(0)
		.Add(fDevicesView = new DevicesView((OrderByType)orderBy));
	GetLayout()->SetExplicitMinSize(BSize(640, 360));

	BRect frame;
	if (settings.FindRect("window_frame", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
		MoveOnScreen(B_MOVE_IF_PARTIALLY_OFFSCREEN);
	} else {
		CenterOnScreen();
	}
}


bool
DevicesWindow::QuitRequested()
{
	_SaveSettings();
	return true;
}


void
DevicesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgRefresh:
		case kMsgReportCompatibility:
		case kMsgGenerateSysInfo:
		case kMsgSelectionChanged:
		case kMsgOrderBus:
		case kMsgOrderCategory:
		case kMsgOrderConnection:
		case kMsgReboot:
		case kMsgToggleDriver:
			fDevicesView->MessageReceived(message);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


status_t
DevicesWindow::_OpenSettings(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Devices_settings");

	return file.SetTo(path.Path(), mode);
}


status_t
DevicesWindow::_LoadSettings(BMessage& settings)
{
	BFile file;
	status_t status = _OpenSettings(file, B_READ_ONLY);
	if (status != B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
DevicesWindow::_SaveSettings()
{
	BFile file;
	status_t status = _OpenSettings(file, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage settings('devs');
	status = settings.AddRect("window_frame", Frame());
	if (status == B_OK)
		status = settings.AddInt32("sort_by", (int32)fDevicesView->OrderBy());

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


int
main()
{
	DevicesApplication app;
	app.Run();
	return 0;
}
