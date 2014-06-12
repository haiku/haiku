/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
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
	:
	BLooper("Shortcuts commands executor")
{
}


CommandExecutor::~CommandExecutor()
{
}


// Returns true if it is returning valid results into (*setBegin) and
// (*setEnd). If returning true, (*setBegin) now points to the first char in a
// new word, and (*setEnd) now points to the char after the last char in the
// word, which has been set to a NUL byte.
bool
CommandExecutor::GetNextWord(char** setBegin, char** setEnd) const
{
	char* next = *setEnd;
		// we'll start one after the end of the last one...

	while (next++) {
		if (*next == '\0')
			return false; // no words left!
		else if (*next <= ' ')
			*next = '\0';
		else
			break; // found a non-whitespace char!
	}

	*setBegin = next;
		// we found the first char!

	while (next++) {
		if (*next <= ' ') {
			*next = '\0'; // terminate the word
			*setEnd = next;
			return true;
		}
	}

	return false;
		// should never get here, actuatorually
}


void
CommandExecutor::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_UNMAPPED_KEY_DOWN:
		case B_KEY_DOWN:
		{
			BMessage actuatorMessage;
			void* asyncData;
			if ((message->FindMessage("act", &actuatorMessage) == B_OK)
				&& (message->FindPointer("adata", &asyncData) == B_OK)) {
				BArchivable* archivedObject
					= instantiate_object(&actuatorMessage);
				if (archivedObject != NULL) {
					CommandActuator* actuator
						= dynamic_cast<CommandActuator*>(archivedObject);

					if (actuator)
						actuator->KeyEventAsync(message, asyncData);
					delete archivedObject;
				}
			}
			break;
		}
	}
}
