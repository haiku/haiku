/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#include "CommandExecutor.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <image.h>


#include "ShortcutsFilterConstants.h"
#include "CommandActuators.h"

CommandExecutor::CommandExecutor()
	: BLooper("Shortcuts commands executor")
{
	// empty
}


CommandExecutor::~CommandExecutor()
{
	// empty
}


// Returns true if it is returning valid results into (*setBegin) and
// (*setEnd). If returning true, (*setBegin) now points to the first char in a
// new word, and (*setEnd) now points to the char after the last char in the
// word, which has been set to a NUL byte.
bool
CommandExecutor::GetNextWord(char** setBegin, char** setEnd) const
{
	char* next = *setEnd; // we'll start one after the end of the last one...

	while (next++) {
		if (*next == '\0')
			return false; // no words left!
		else if (*next <= ' ')
			*next = '\0';
		else
			break;	// found a non-whitespace char!
	}

	*setBegin = next;	// we found the first char!

	while (next++) {
		if (*next <= ' ') {
			*next = '\0';	// terminate the word
			*setEnd = next;
			return true;
		}
	}
	return false;	// should never get here, actually
}


void
CommandExecutor::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case B_UNMAPPED_KEY_DOWN:
		case B_KEY_DOWN:
		{
			BMessage actMessage;
			void* asyncData;
			if ((msg->FindMessage("act", &actMessage) == B_NO_ERROR)
				&& (msg->FindPointer("adata", &asyncData) == B_NO_ERROR)) {
				BArchivable* arcObj = instantiate_object(&actMessage);
				if (arcObj) {
					CommandActuator* act = dynamic_cast<CommandActuator*>(arcObj);

					if (act)
						act->KeyEventAsync(msg, asyncData);
					delete arcObj;
				}
			}
			break;
		}
	}
}
