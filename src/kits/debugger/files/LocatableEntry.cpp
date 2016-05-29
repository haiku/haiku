/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "LocatableEntry.h"

#include "AutoLocker.h"

#include "LocatableDirectory.h"


// #pragma mark - LocatableEntryOwner


LocatableEntryOwner::~LocatableEntryOwner()
{
}


// #pragma mark - LocatableEntry


LocatableEntry::LocatableEntry(LocatableEntryOwner* owner,
	LocatableDirectory* parent)
	:
	fOwner(owner),
	fParent(parent),
	fState(LOCATABLE_ENTRY_UNLOCATED)
{
	if (fParent != NULL)
		fParent->AcquireReference();
}


LocatableEntry::~LocatableEntry()
{
	if (fParent != NULL)
		fParent->ReleaseReference();
}


void
LocatableEntry::LastReferenceReleased()
{
	fOwner->LocatableEntryUnused(this);
}
