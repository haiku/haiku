/*
 * Copyright 2008, Jérôme Duval, korli@users.berlios.de. All rights reserved.
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "PCXTranslator.h"
#include "PCX.h"

#include "TranslatorWindow.h"
#include <Application.h>
#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "main"


int
main(int /*argc*/, char **/*argv*/)
{
	BApplication app("application/x-vnd.Haiku-PCXTranslator");

	status_t result;
	result = LaunchTranslatorWindow(new PCXTranslator, 
		B_TRANSLATE("PCX Settings"), BRect(0, 0, 225, 175));
	if (result != B_OK)
		return 1;

	app.Run();
	return 0;
}

