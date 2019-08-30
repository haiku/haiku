/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ENTRY_LISTENER_H
#define ENTRY_LISTENER_H

class Entry;

// listening flags
enum {
	ENTRY_LISTEN_ANY_ENTRY	= 0x01,
	ENTRY_LISTEN_ADDED		= 0x02,
	ENTRY_LISTEN_REMOVED	= 0x04,
	ENTRY_LISTEN_ALL		= ENTRY_LISTEN_ADDED | ENTRY_LISTEN_REMOVED,
};

class EntryListener {
public:
	EntryListener();
	virtual ~EntryListener();

	virtual void EntryAdded(Entry *entry);
	virtual void EntryRemoved(Entry *entry);
};

#endif	// ENTRY_LISTENER_H
