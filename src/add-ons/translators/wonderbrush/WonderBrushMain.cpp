/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <Application.h>
#include <Catalog.h>

#include "WonderBrushTranslator.h"
#include "TranslatorWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WonderBrushMain"


int
main()
{
	BApplication app("application/x-vnd.Haiku-WonderBrushTranslator");
	status_t result;
	result = LaunchTranslatorWindow(new WonderBrushTranslator,
		B_TRANSLATE("WBI Settings"), BRect(0, 0, 225, 175));
	if (result == B_OK) {
		app.Run();
		return 0;
	} else
		return 1;
}
