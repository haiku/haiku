/*
 * Copyright 2003-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Atsushi Takamatsu
 *		Jérôme Duval
 *		Oliver Ruiz Dorantes
 */


#include "HApp.h"
#include "HWindow.h"

#include <Alert.h>
#include <Catalog.h>
#include <Locale.h>


#undef TR_CONTEXT
#define TR_CONTEXT "SoundsHApp"


HApp::HApp()
	:
	BApplication("application/x-vnd.Haiku-Sounds")
{
	be_locale->GetAppCatalog(&fCatalog);

	BRect rect;
	rect.Set(200, 150, 590, 570);

	HWindow* window = new HWindow(rect, TR("Sounds"));
	window->Show();
}


HApp::~HApp()
{
}


void
HApp::AboutRequested()
{
	BAlert* alert = new BAlert(TR("About Sounds"),
				TR("Sounds\n"
				"  Brought to you by :\n"
				"\tOliver Ruiz Dorantes\n"
				"\tJérôme DUVAL.\n"
				"  Original work from Atsushi Takamatsu.\n"
				"Copyright ©2003-2006 Haiku"),
				TR("OK"));
	alert->Go();
}


//	#pragma mark -


int
main(int, char**)
{
	HApp app;
	app.Run();

	return 0;
}

