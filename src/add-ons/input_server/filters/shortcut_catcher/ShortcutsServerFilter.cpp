/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#include "ShortcutsServerFilter.h"

#include <Path.h>
#include <FindDirectory.h>

#include "KeyCommandMap.h"
#include "KeyInfos.h"
#include "CommandExecutor.h"
#include "ShortcutsFilterConstants.h"


class BInputServerFilter;


// Called by input_server on startup. Must be exported as a "C" function!
BInputServerFilter* instantiate_input_filter()
{
	return new ShortcutsServerFilter;
}


ShortcutsServerFilter::ShortcutsServerFilter()
	:
	fExecutor(new CommandExecutor)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK)
		path.Append(SHORTCUTS_SETTING_FILE_NAME);

	fMap = new KeyCommandMap(path.Path());

	InitKeyIndices();
	fMessenger = BMessenger(fExecutor);
	fMap->Run();
	fExecutor->Run();
}


ShortcutsServerFilter::~ShortcutsServerFilter()
{
	if (fMap->Lock())
		fMap->Quit();

	if (fExecutor->Lock())
		fExecutor->Quit();
}


status_t
ShortcutsServerFilter::InitCheck()
{
	return B_OK;
}


filter_result
ShortcutsServerFilter::Filter(BMessage* message, BList* outList)
{
	filter_result result = B_DISPATCH_MESSAGE;

	switch(message->what) {
		case B_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
			result = fMap->KeyEvent(message, outList, fMessenger);
			break;

		case B_MOUSE_MOVED:
		case B_MOUSE_UP:
		case B_MOUSE_DOWN:
			fMap->MouseMessageReceived(message);
			break;
	}
	fMap->DrainInjectedEvents(message, outList, fMessenger);

	return result;
}
