/*
 * Part of the MiniTerminal.
 *
 * Copyright 2005 Michael Lotz. All rights reserved.
 * Distributed under the MIT License.
 */

#include <stdio.h>

#include "Arguments.h"
#include "MiniApp.h"
#include "MiniView.h"
#include "MiniWin.h"

MiniApp::MiniApp(const Arguments &args)
	:	BApplication("application/x-vnd.Haiku.MiniTerminal")
{
	fWindow = new MiniWin(args);
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
main(int argc, const char *const argv[])
{
	Arguments args;
	args.Parse(argc, argv);
	
	MiniApp *app = new MiniApp(args);
	app->Run();
	delete app;
	return 0;
}
