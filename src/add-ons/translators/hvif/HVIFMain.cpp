/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */

#include <Application.h>
#include <Catalog.h>
#include "HVIFTranslator.h"
#include "TranslatorWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HVIFMain"

int
main(int argc, char *argv[])
{
	BApplication application("application/x-vnd.Haiku.HVIFTranslator");
	if (LaunchTranslatorWindow(new HVIFTranslator, 
		B_TRANSLATE("HVIF Settings"), BRect(0, 0, 250, 150)) != B_OK)
		return 1;

	application.Run();
	return 0;
}
