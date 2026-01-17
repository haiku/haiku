/*
 * Copyright 2003-2015, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Atsushi Takamatsu
 *		Jérôme Duval
 *		Oliver Ruiz Dorantes
 */


#include "HApp.h"
#include "HWindow.h"

#include <AboutWindow.h>
#include <Alert.h>
#include <Catalog.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoundsHApp"


HApp::HApp()
	:
	BApplication("application/x-vnd.Haiku-Sounds")
{
	HWindow* window = new HWindow(BRect(-1, -1, 390, 420),
		B_TRANSLATE_SYSTEM_NAME("Sounds"));
	window->Show();
}


HApp::~HApp()
{
}


void
HApp::AboutRequested()
{
	BAboutWindow* window = new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("Sounds"),
		"application/x-vnd.Haiku-Sounds");

	const char* authors[] = {
		"Atsushi Takamatsu",
		"Oliver Ruiz Dorantes",
		"Jérôme DUVAL",
		NULL
	};

	window->AddCopyright(2003, "Haiku, Inc.");
	window->AddAuthors(authors);
	window->Show();
}


//	#pragma mark -


int
main()
{
	HApp app;
	app.Run();

	return 0;
}

