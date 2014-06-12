/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
#ifndef _SHORTCUTS_CONSTANTS_H
#define _SHORTCUTS_CONSTANTS_H


#define SHORTCUTS_SETTING_FILE_NAME		"shortcuts_settings"
#define SHORTCUTS_CATCHER_PORT_NAME		"ShortcutsCatcherPort"


enum {
	EXECUTE_COMMAND = 'exec',	// Code: Execute "command" as given
	REPLENISH_MESSENGER,		// sent to tell us to write our port again
	NUM_ACTION_CODES
};


#endif	// _SHORTCUTS_CONSTANTS_H
