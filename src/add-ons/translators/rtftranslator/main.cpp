/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "RTFTranslator.h"
#include "RTF.h"

#include <TranslatorWindow.h>
#include <TranslatorRoster.h>
#include <Application.h>

#include <stdio.h>
#include <string.h>


int
main(int /*argc*/, char **/*argv*/)
{
	BApplication app("application/x-vnd.haiku-rtf-translator");

	status_t result;
	result = LaunchTranslatorWindow(new RTFTranslator, "RTF Settings", BRect(0, 0, 225, 175));
	if (result != B_OK)
		return 1;

	app.Run();
	return 0;
}

