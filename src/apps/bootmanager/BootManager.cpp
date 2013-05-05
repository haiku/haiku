/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 * 		Axel Dörfler <axeld@pinc-software.de>
 */


#include "BootManagerWindow.h"

#include <Application.h>
#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BootManager"


static const char* kSignature = "application/x-vnd.Haiku-BootManager";


class BootManager : public BApplication {
public:
								BootManager();

	virtual void				ReadyToRun();
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


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	BootManager application;
	application.Run();

	return 0;
}
