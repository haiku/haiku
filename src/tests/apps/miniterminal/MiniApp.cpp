/*
 * Part of the MiniTerminal.
 *
 * Copyright 2005 Michael Lotz. All rights reserved.
 * Distributed under the Haiku License.
 */

#include <stdio.h>
#include "MiniApp.h"
#include "MiniWin.h"
#include "MiniView.h"

MiniApp::MiniApp(BRect bounds)
	:	BApplication("application/x-vnd.Haiku.MiniTerminal")
{
	fWindow = new MiniWin(bounds);
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
main(int argc, const char *argv[])
{
	BRect bounds(50, 50, 630, 435);
	
	if (argc >= 3) {
		BPoint offset;
		sscanf(argv[1], "%f", &offset.x);
		sscanf(argv[2], "%f", &offset.y);
		bounds.OffsetTo(offset);
	}
	
	if (argc >= 5) {
		BPoint size;
		sscanf(argv[3], "%f", &size.x);
		sscanf(argv[4], "%f", &size.y);
		bounds.right = bounds.left + size.x;
		bounds.bottom = bounds.top + size.y;
	}
	
	MiniApp *app = new MiniApp(bounds);
	app->Run();
	delete app;
	return 0;
}
