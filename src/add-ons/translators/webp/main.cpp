/*
 * Copyright 2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include <Application.h>
#include <Catalog.h>

#include "TranslatorWindow.h"
#include "WebPTranslator.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "main"


int
main()
{
	BApplication app("application/x-vnd.Haiku-WebPTranslator");
	if (LaunchTranslatorWindow(new WebPTranslator,
		B_TRANSLATE("WebP Settings")) != B_OK)
		return 1;

	app.Run();
	return 0;
}
