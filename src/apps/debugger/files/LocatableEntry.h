/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOCATABLE_ENTRY_H
#define LOCATABLE_ENTRY_H

#include <String.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


enum locatable_entry_state {
	LOCATABLE_ENTRY_UNLOCATED,
	LOCATABLE_ENTRY_LOCATED_IMPLICITLY,
	LOCATABLE_ENTRY_LOCATED_EXPLICITLY
};


class LocatableDirectory;
class LocatableEntry;


class LocatableEntryOwner {
public:
	virtual						~LocatableEntryOwner();

	virtual	bool				Lock() = 0;
	virtual	void				Unlock() = 0;

	virtual	void				LocatableEntryUnused(LocatableEntry* entry) = 0;
};


class LocatableEntry : public BReferenceable,
	public DoublyLinkedListLinkImpl<LocatableEntry> {
public:
								LocatableEntry(LocatableEntryOwner* owner,
									LocatableDirectory* parent);
								~LocatableEntry();

			LocatableDirectory*	Parent() const	{ return fParent; }
	virtual	const char*			Name() const = 0;

			// mutable (requires locking)
			locatable_entry_state State() const	{ return fState; }
	virtual	bool				GetLocatedPath(BString& _path) const = 0;
	virtual	void				SetLocatedPath(const BString& path,
									bool implicit) = 0;

protected:
	virtual	void				LastReferenceReleased();

protected:
			LocatableEntryOwner* fOwner;
			LocatableDirectory*	fParent;
			locatable_entry_state fState;

public:
			LocatableEntry* fNext;
};


typedef DoublyLinkedList<LocatableEntry> LocatableEntryList;


#endif	// LOCATABLE_ENTRY_H
