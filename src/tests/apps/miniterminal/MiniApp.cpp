/*
 * Part of the MiniTerminal.
 *
 * Copyright 2005 Michael Lotz. All rights reserved.
 * Distributed under the Haiku License.
 */

#include "MiniApp.h"
#include "MiniWin.h"
#include "MiniView.h"

MiniApp::MiniApp()
	:	BApplication("application/x-vnd.Haiku.MiniTerminal")
{
	fWindow = new MiniWin(BRect(50, 50, 630, 435).OffsetToCopy(640, 480));
	fWindow->Show();
}


void
MiniApp::ReadyToRun()
{
	fWindow->View()->Start();
}


MiniApp::~MiniApp()
{
}


int
main(void)
{
	MiniApp *app = new MiniApp();
	app->Run();
	delete app;
	return 0;
}
