/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>

#include "KeyboardInputDevice.h"
#include "TMWindow.h"

#include <stdio.h>
#include <stdlib.h>

#if DEBUG
FILE *KeyboardInputDevice::sLogFile = NULL;
#endif


int
main()
{
	BApplication app("application/x-vnd.tmwindow-test");
	TMWindow *window = new TMWindow();
	window->Enable();

	// we don't even quit when the window is closed...
	app.Run();
}
