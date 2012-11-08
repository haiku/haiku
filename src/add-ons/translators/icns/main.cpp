/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2012, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>

#include "ICNSTranslator.h"
#include "TranslatorWindow.h"

int
main(int /*argc*/, char ** /*argv*/)
{
	BApplication app("application/x-vnd.Haiku-ICNSTranslator");

	status_t result;
	result = LaunchTranslatorWindow(new ICNSTranslator, 
		"ICNS Settings", BRect(0, 0, 320, 200));
	if (result != B_OK)
		return 1;

	app.Run();
	return 0;
}

