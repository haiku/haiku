/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "CharacterMap.h"

#include <stdlib.h>

#include <Application.h>
#include <Catalog.h>

#include "CharacterWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CharacterMap"

const char* kSignature = "application/x-vnd.Haiku-CharacterMap";


CharacterMap::CharacterMap()
	: BApplication(kSignature)
{
}


CharacterMap::~CharacterMap()
{
}


void
CharacterMap::ReadyToRun()
{
	fWindow = new CharacterWindow();
	fWindow->Show();
}


void
CharacterMap::RefsReceived(BMessage* message)
{
	fWindow->PostMessage(message);
}


void
CharacterMap::MessageReceived(BMessage* message)
{
	BApplication::MessageReceived(message);
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	CharacterMap app;
	app.Run();

	return 0;
}
