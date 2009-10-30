/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
 
 
#ifndef ShortcutsServerFilter_h
#define ShortcutsServerFilter_h


#include <stdio.h>


#include <Messenger.h>
#include <Message.h>
#include <List.h>
#include <InputServerFilter.h>

#ifdef __POWERPC__
#pragma export on
#endif

// export this for the input_server
extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

class KeyCommandMap;
class CommandExecutor;

class ShortcutsServerFilter : public BInputServerFilter {
public:
								ShortcutsServerFilter();
	virtual 					~ShortcutsServerFilter();
	virtual status_t 			InitCheck();
	virtual filter_result 		Filter(BMessage* message, BList* outlist);
private:
			// Tells us what command goes with a given key
			KeyCommandMap*		fMap;		
	
			// Executes the given commands
			CommandExecutor*	fExecutor;
	
			// Points to fExecutor:declaration order is important!
			BMessenger			fMessenger;
};

#ifdef __POWERPC__
#pragma export reset
#endif

#endif
