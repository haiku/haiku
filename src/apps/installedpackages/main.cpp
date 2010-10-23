/*
 * Copyright (c) 2007-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "UninstallWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Entry.h>
#include <Locale.h>
#include <List.h>
#include <TextView.h>

#include <stdio.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "UninstallApplication"


class UninstallApplication : public BApplication {
public:
	UninstallApplication();
	~UninstallApplication();

	void AboutRequested();

private:
	UninstallWindow*	fWindow;
};


UninstallApplication::UninstallApplication()
	:
	BApplication("application/x-vnd.Haiku-InstalledPackages"),
	fWindow(NULL)
{
	fWindow = new UninstallWindow();
	fWindow->Show();
}


UninstallApplication::~UninstallApplication()
{
}


void
UninstallApplication::AboutRequested()
{
	BString aboutString = B_TRANSLATE("InstalledPackages");
	int appNameLength = aboutString.Length();
	aboutString << "\n";
	aboutString << B_TRANSLATE(
		"BeOS legacy .pkg package removing application "
		"for Haiku.\n\n"
		"Copyright 2007,\nŁukasz 'Sil2100' Zemczak\n\n"
		"Copyright (c) 2007 Haiku, Inc.\n");
	BAlert* about = new BAlert("about", aboutString.String(),
		B_TRANSLATE("Close"));

	BTextView* view = about->TextView();
	BFont font;
	view->SetStylable(true);
	view->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(font.Size() * 1.5);
	view->SetFontAndColor(0, appNameLength, &font);

	about->Go();
}


int
main()
{
	UninstallApplication app;
	app.Run();
	return 0;
}

