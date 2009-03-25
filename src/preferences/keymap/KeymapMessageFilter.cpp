/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "KeymapMessageFilter.h"

#include <AppDefs.h>
#include <Message.h>

#include "Keymap.h"


KeymapMessageFilter::KeymapMessageFilter(message_delivery delivery,
		message_source source, Keymap* keymap)
	: BMessageFilter(delivery, source),
	fKeymap(keymap)
{
}


KeymapMessageFilter::~KeymapMessageFilter()
{
}


void
KeymapMessageFilter::SetKeymap(Keymap* keymap)
{
	fKeymap = keymap;
}


filter_result
KeymapMessageFilter::Filter(BMessage* message, BHandler** /*_target*/)
{
	if (fKeymap == NULL || message->what != B_KEY_DOWN)
		return B_DISPATCH_MESSAGE;

	// TODO: add dead key handling!

	int32 modifiers;
	int32 key;
	if (message->FindInt32("modifiers", &modifiers) == B_OK
		&& message->FindInt32("key", &key) == B_OK) {
		// replace "bytes", and "raw_char"/"byte"
		char* string;
		int32 numBytes;
		fKeymap->GetChars(key, modifiers, 0, &string, &numBytes);
		if (string != NULL) {
			message->ReplaceString("bytes", string);
			delete[] string;
		}

		fKeymap->GetChars(key, 0, 0, &string, &numBytes);
		if (string != NULL) {
			message->ReplaceInt32("raw_char", string[0]);
			message->ReplaceInt8("byte", string[0]);
			delete[] string;
		}
	}
	
	return B_DISPATCH_MESSAGE;
}
