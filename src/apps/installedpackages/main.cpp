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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UninstallApplication"


class UninstallApplication : public BApplication {
public:
	UninstallApplication();
	~UninstallApplication();

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


int
main()
{
	UninstallApplication app;
	app.Run();
	return 0;
}

