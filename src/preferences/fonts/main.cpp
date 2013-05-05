/*
 * Copyright 2001-2005, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "MainWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <TextView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "main"

class FontsApp : public BApplication {
	public:
		FontsApp();

		virtual void AboutRequested();
};


FontsApp::FontsApp()
	: BApplication("application/x-vnd.Haiku-Fonts")
{
	MainWindow *window = new MainWindow();
	window->Show();
}


void
FontsApp::AboutRequested()
{
	BAlert *alert = new BAlert("about", B_TRANSLATE("Fonts\n"
		"\tCopyright 2004-2005, Haiku.\n\n"), B_TRANSLATE("OK"));
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 5, &font);
	
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


//	#pragma mark -


int
main(int, char**)
{
	FontsApp app;
	app.Run();

	return 0;
}

