/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "EXRTranslator.h"

#include "TranslatorWindow.h"
#include <Application.h>


int
main(int /*argc*/, char **/*argv*/)
{
	BApplication app("application/x-vnd.Haiku-EXRTranslator");

	status_t result;
	result = LaunchTranslatorWindow(new EXRTranslator, "EXR Settings");
	if (result != B_OK)
		return 1;

	app.Run();
	return 0;
}

