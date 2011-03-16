/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 * 		Axel Dörfler <axeld@pinc-software.de>
 */


#include "BootManagerWindow.h"

#include <AboutWindow.h>
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
	const char* authors[] = {
		"David Dengg",
		"Michael Pfeiffer",
		NULL
	};
	
	BAboutWindow about(B_TRANSLATE_APP_NAME("BootManager"), 2008, authors);
	about.Show();
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	BootManager application;
	application.Run();

	return 0;
}
