/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 * 		Axel Dörfler <axeld@pinc-software.de>
 */


#include "BootManagerWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <TextView.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "BootManager"


static const char* kSignature = "application/x-vnd.Haiku-BootManager";


class BootManager : public BApplication {
public:
								BootManager();

	virtual void				ReadyToRun();
	virtual void				AboutRequested();
};


BootManager::BootManager()
	:
	BApplication(kSignature)
{
}


void
BootManager::ReadyToRun()
{
	BootManagerWindow* window = new BootManagerWindow();
	window->Show();
}


void
BootManager::AboutRequested()
{
	BString aboutText;
	const char* title = B_TRANSLATE_COMMENT("BootManager", "Application name");
	aboutText << title << "\n\n"
		<< B_TRANSLATE("written by")
		<< "\n"
			"\tDavid Dengg\n"
			"\tMichael Pfeiffer\n"
			"\n"
		<< B_TRANSLATE_COMMENT("Copyright %year, Haiku Inc.\n",
			"Leave %year untranslated");
	aboutText.ReplaceLast("%year", "2008-2010");
	BAlert *alert = new BAlert("about",
		aboutText.String(), B_TRANSLATE("OK"));
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, strlen(title), &font);

	alert->Go();
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	BootManager application;
	application.Run();

	return 0;
}
