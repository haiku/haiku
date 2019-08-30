/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ENTRY_ITERATOR_H
#define ENTRY_ITERATOR_H

#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>

class Directory;
class Entry;

class EntryIterator : public DoublyLinkedListLinkImpl<EntryIterator> {
public:
	EntryIterator(Directory *directory = NULL);
	~EntryIterator();

	status_t SetTo(Directory *directory);
	void Unset();

	Directory *GetDirectory() const { return fDirectory; }

	status_t Suspend();
	status_t Resume();
	bool IsSuspended() const { return fSuspended; }

	status_t GetNext(Entry **entry);
	Entry *GetCurrent() const { return fEntry; }

	status_t Rewind();

private:
	void SetCurrent(Entry *entry, bool isNext);

private:
	friend class Directory;

private:
	Directory					*fDirectory;
	Entry						*fEntry;
	bool						fSuspended;
	bool						fIsNext;
	bool						fDone;
};

#endif	// ENTRY_ITERATOR_H
