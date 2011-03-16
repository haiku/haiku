/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "CharacterMap.h"

#include <stdlib.h>

#include <AboutWindow.h>
#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <TextView.h>

#include "CharacterWindow.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "CharacterMap"

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


void
CharacterMap::AboutRequested()
{
	const char* authors[] = {
		"Axel Dörfler",
		NULL
	};
	
	BAboutWindow about(B_TRANSLATE_APP_NAME("CharacterMap"), 2009, authors);
	about.Show();
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	CharacterMap app;
	app.Run();

	return 0;
}
