/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Emmanuel Gil Peyrot
 */


#include <Application.h>
#include <Catalog.h>

#include "TranslatorWindow.h"
#include "AVIFTranslator.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "main"


int
main()
{
	BApplication app("application/x-vnd.Haiku-AVIFTranslator");
	if (LaunchTranslatorWindow(new AVIFTranslator,
		B_TRANSLATE("AVIF Settings")) != B_OK)
		return 1;

	app.Run();
	return 0;
}
