/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "CharacterMap.h"

#include <stdlib.h>

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
	BAlert *alert = new BAlert("about", B_TRANSLATE(
		"CharacterMap\n"
		"\twritten by Axel Dörfler\n"
		"\tCopyright 2009, Haiku, Inc.\n"), B_TRANSLATE("OK"));
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, 12, &font);

	alert->Go();
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	CharacterMap app;
	app.Run();

	return 0;
}
