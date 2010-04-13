/*
 * Copyright 2001-2010, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ENTRY_LIST_H
#define _ENTRY_LIST_H


#include <dirent.h>

#include <SupportDefs.h>


class BEntry;
struct entry_ref;


/*! Interface for iterating through a list of filesystem entries
	Defines a general interface for iterating through a list of entries (i.e.
	files in a folder
*/
class BEntryList {
public:
								BEntryList();
	virtual						~BEntryList();
	
	virtual status_t			GetNextEntry(BEntry* entry,
									bool traverse = false) = 0;
	virtual status_t			GetNextRef(entry_ref* ref) = 0;
	virtual int32				GetNextDirents(struct dirent* direntBuffer,
									size_t bufferSize,
									int32 maxEntries = INT_MAX) = 0;
	virtual status_t			Rewind() = 0;
	virtual int32				CountEntries() = 0;

private:
	virtual	void				_ReservedEntryList1();
	virtual	void				_ReservedEntryList2();
	virtual	void				_ReservedEntryList3();
	virtual	void				_ReservedEntryList4();
	virtual	void				_ReservedEntryList5();
	virtual	void				_ReservedEntryList6();
	virtual	void				_ReservedEntryList7();
	virtual	void				_ReservedEntryList8();
};


#endif	// _ENTRY_LIST_H


