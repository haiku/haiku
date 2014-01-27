/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ralf Schülke, ralf.schuelke@googlemail.com
 *		John Scipione, jscipione@gmail.com
 */


#include "Pairs.h"

#include <stdio.h>
	// for snprintf()
#include <stdlib.h>

#include <Alert.h>
#include <Catalog.h>
#include <Message.h>
#include <MimeType.h>
#include <String.h>

#include "PairsWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Pairs"


const char* kSignature = "application/x-vnd.Haiku-Pairs";

static const size_t kMinIconCount = 64;
static const size_t kMaxIconCount = 384;


//	#pragma mark - Pairs


Pairs::Pairs()
	:
	BApplication(kSignature),
	fWindow(NULL)
{
	_GetVectorIcons();
}


Pairs::~Pairs()
{
}


void
Pairs::ReadyToRun()
{
	fWindow = new PairsWindow();
	fWindow->Show();
}


void
Pairs::RefsReceived(BMessage* message)
{
	fWindow->PostMessage(message);
}


void
Pairs::MessageReceived(BMessage* message)
{
	BApplication::MessageReceived(message);
}


bool
Pairs::QuitRequested()
{
	// delete vector icons
	for (IconMap::iterator iter = fIconMap.begin(); iter != fIconMap.end();
			++iter) {
		delete fIconMap[iter->first];
	}

	return true;
}


//	#pragma mark - Pairs private methods


void
Pairs::_GetVectorIcons()
{
	// Load vector icons from the MIME type database and add a pointer to them
	// into a std::map keyed by a generated hash.

	BMessage types;
	if (BMimeType::GetInstalledTypes("application", &types) != B_OK)
		return;

	const char* type;
	for (int32 i = 0; types.FindString("types", i, &type) == B_OK; i++) {
		BMimeType mimeType(type);
		if (mimeType.InitCheck() != B_OK)
			continue;

		uint8* data;
		size_t size;

		if (mimeType.GetIcon(&data, &size) != B_OK) {
			// didn't find an icon
			continue;
		}

		size_t hash = 0xdeadbeef;
		for (size_t i = 0; i < size; i++)
			hash = 31 * hash + data[i];

		if (fIconMap.find(hash) != fIconMap.end()) {
			// key has already been added to the map
			delete[] data;
			continue;
		}

		vector_icon* icon = (vector_icon*)malloc(sizeof(vector_icon));
		if (icon == NULL) {
			delete[] data;
			free(icon);
			continue;
		}

		icon->data = data;
		icon->size = size;

		// found a vector icon, add it to the list
		fIconMap[hash] = icon;
		if (fIconMap.size() >= kMaxIconCount) {
			// this is enough to choose from, stop eating memory...
			return;
		}
	}

	if (fIconMap.size() < kMinIconCount) {
		char buffer[512];
		snprintf(buffer, sizeof(buffer),
			B_TRANSLATE_COMMENT("Pairs did not find enough vector icons "
			"to start; it needs at least %zu, found %zu.\n",
			"Don't translate \"%zu\", but make sure to keep them."),
			kMinIconCount, fIconMap.size());
		BString messageString(buffer);
		BAlert* alert = new BAlert("Fatal", messageString.String(),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_FROM_WIDEST,
			B_STOP_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		exit(1);
	}
}


//	#pragma mark - main


int
main(void)
{
	Pairs pairs;
	pairs.Run();

	return 0;
}
