/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 */
#ifndef _TRACKER_ADDON_H
#define _TRACKER_ADDON_H

struct entry_ref;
class BMessage;

extern "C" void process_refs(entry_ref directory, BMessage* refs,
	void* reserved);

#endif	// _TRACKER_ADDON_H
