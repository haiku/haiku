/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <Application.h>

#include "WonderBrushTranslator.h"
#include "TranslatorWindow.h"

int
main()
{
	BApplication app("application/x-vnd.haiku-wbi-translator");
	status_t result;
	result = LaunchTranslatorWindow(new WonderBrushTranslator,
		"WBI Settings", BRect(0, 0, 225, 175));
	if (result == B_OK) {
		app.Run();
		return 0;
	} else
		return 1;
}
