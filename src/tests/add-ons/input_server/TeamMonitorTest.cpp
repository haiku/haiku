/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>

#include "KeyboardInputDevice.h"
#include "TeamMonitorWindow.h"

#include <stdio.h>
#include <stdlib.h>

#if DEBUG
FILE *KeyboardInputDevice::sLogFile = NULL;
#endif


int
main()
{
	BApplication app("application/x-vnd.tmwindow-test");
	TeamMonitorWindow *window = new TeamMonitorWindow();

	// The window should quit the app when it is done
	window->SetFlags(window->Flags() | B_QUIT_ON_WINDOW_CLOSE);
	window->Enable();

	// The window is designed to never quit unless this message was received.
	// And in our case we would like it to quit.
	BMessage message(SYSTEM_SHUTTING_DOWN);
	window->PostMessage(&message);

	app.Run();
}
