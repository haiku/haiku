/*
 * Copyright 2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include <Application.h>
#include "WEBPTranslator.h"
#include "TranslatorWindow.h"

int
main()
{
	BApplication app("application/x-vnd.Haiku-WEBPTranslator");
	if (LaunchTranslatorWindow(new WEBPTranslator,
		"WEBP Settings") != B_OK)
		return 1;

	app.Run();
	return 0;
}
