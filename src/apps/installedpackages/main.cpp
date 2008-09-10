/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#include "UninstallWindow.h"
#include <Application.h>
#include <List.h>
#include <Alert.h>
#include <TextView.h>
#include <Entry.h>
#include <Autolock.h>
#include <stdio.h>


class UninstallApplication : public BApplication {
	public:
		UninstallApplication();
		~UninstallApplication();
		
		void MessageReceived(BMessage *msg);
		
		void AboutRequested();

	private:
		UninstallWindow *fWindow;
};


UninstallApplication::UninstallApplication()
	:	BApplication("application/x-vnd.Haiku-InstalledPackages")
{
	fWindow = new UninstallWindow();
	fWindow->Show();
}


UninstallApplication::~UninstallApplication()
{
}


void
UninstallApplication::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		default:
			BApplication::MessageReceived(msg);
	}
}


void
UninstallApplication::AboutRequested()
{
	BAlert *about = new BAlert("about",
			"InstalledPackages\n"
			"BeOS legacy .pkg package removing application for Haiku.\n\n"
			"Copyright 2007,\nŁukasz 'Sil2100' Zemczak\n\n"
			"Copyright (c) 2007 Haiku, Inc. \n",
			"Close");

	BTextView *view = about->TextView();
	BFont font;
	view->SetStylable(true);
	view->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(font.Size() * 1.5);
	view->SetFontAndColor(0, 15, &font);

	about->Go();
}


int
main(void)
{
	UninstallApplication app;
	app.Run();
	
	return 0;
}

