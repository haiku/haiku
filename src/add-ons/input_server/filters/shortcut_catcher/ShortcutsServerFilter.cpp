/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
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
	return B_NO_ERROR;
}


filter_result
ShortcutsServerFilter::Filter(BMessage* msg, BList* outlist)
{
	filter_result ret = B_DISPATCH_MESSAGE;

	switch(msg->what)
	{
		case B_KEY_DOWN: 
		case B_KEY_UP: 
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
			ret = fMap->KeyEvent(msg, outlist, fMessenger);
		break;

		case B_MOUSE_MOVED:
		case B_MOUSE_UP:
		case B_MOUSE_DOWN:
			fMap->MouseMoved(msg);
		break;
	}
	fMap->DrainInjectedEvents(msg, outlist, fMessenger);
	return ret;
}
