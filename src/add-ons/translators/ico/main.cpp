/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ICOTranslator.h"
#include "ICO.h"

#include "TranslatorWindow.h"
#include <Application.h>
/*
#include <TranslatorRoster.h>

#include <stdio.h>
#include <string.h>
*/

int
main(int /*argc*/, char **/*argv*/)
{
	BApplication app("application/x-vnd.haiku-ico-translator");

	status_t result;
	result = LaunchTranslatorWindow(new ICOTranslator, "ICO Settings", BRect(0, 0, 225, 175));
	if (result != B_OK)
		return 1;

	app.Run();
	return 0;
}

