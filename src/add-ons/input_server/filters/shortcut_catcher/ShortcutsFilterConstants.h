/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#ifndef ShortcutsConstants_h
#define ShortcutsConstants_h

#define SHORTCUTS_SETTING_FILE_NAME		"shortcuts_settings"

#define SHORTCUTS_CATCHER_PORT_NAME		"ShortcutsCatcherPort" 

enum {
	EXECUTE_COMMAND = 'exec',	// Code: Execute "command" as given
	REPLENISH_MESSENGER,		// sent to tell us to write our port again
	NUM_ACTION_CODES
};

#endif

