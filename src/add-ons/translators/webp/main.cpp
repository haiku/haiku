/*
 * Copyright 2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include <Application.h>

#include "WebPTranslator.h"
#include "TranslatorWindow.h"

int
main()
{
	BApplication app("application/x-vnd.Haiku-WebPTranslator");
	if (LaunchTranslatorWindow(new WebPTranslator,
		"WebP Settings") != B_OK)
		return 1;

	app.Run();
	return 0;
}
