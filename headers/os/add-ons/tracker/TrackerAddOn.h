/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 */
#ifndef _TRACKER_ADDON_H
#define _TRACKER_ADDON_H

struct entry_ref;
class BMessage;
class BMenu;
class BHandler;

extern "C" {
	void process_refs(entry_ref directory, BMessage* refs, void* reserved);
	void populate_menu(BMessage* msg, BMenu* menu, BHandler* handler);
	void message_received(BMessage* msg);
}

#endif	// _TRACKER_ADDON_H
