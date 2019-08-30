/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Directory.h"
#include "Entry.h"
#include "EntryIterator.h"
#include "Volume.h"

// constructor
EntryIterator::EntryIterator(Directory *directory)
	: fDirectory(directory),
	  fEntry(NULL),
	  fSuspended(false),
	  fIsNext(false),
	  fDone(false)
{
}

// destructor
EntryIterator::~EntryIterator()
{
	Unset();
}

// SetTo
status_t
EntryIterator::SetTo(Directory *directory)
{
	Unset();
	status_t error = (directory ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fDirectory = directory;
		fEntry = NULL;
		fSuspended = false;
		fIsNext = false;
		fDone = false;
	}
	return error;
}

// Unset
void
EntryIterator::Unset()
{
	if (fDirectory && fSuspended)
		Resume();
	fDirectory = NULL;
	fEntry = NULL;
	fSuspended = false;
	fIsNext = false;
	fDone = false;
}

// Suspend
status_t
EntryIterator::Suspend()
{
	status_t error = (fDirectory ? B_OK : B_ERROR);
	if (error == B_OK) {
		if (fDirectory->GetVolume()->IteratorLock()) {
			if (!fSuspended) {
				if (fEntry)
					fEntry->AttachEntryIterator(this);
				fDirectory->GetVolume()->IteratorUnlock();
				fSuspended = true;
			} else
				error = B_ERROR;
		} else
			error = B_ERROR;
	}
	return error;
}

// Resume
status_t
EntryIterator::Resume()
{
	status_t error = (fDirectory ? B_OK : B_ERROR);
	if (error == B_OK) {
		if (fDirectory->GetVolume()->IteratorLock()) {
			if (fSuspended) {
				if (fEntry)
					fEntry->DetachEntryIterator(this);
				fSuspended = false;
			}
			fDirectory->GetVolume()->IteratorUnlock();
		} else
			error = B_ERROR;
	}
	return error;
}

// GetNext
status_t
EntryIterator::GetNext(Entry **entry)
{
	status_t error = B_ENTRY_NOT_FOUND;
	if (!fDone && fDirectory && entry) {
		if (fIsNext) {
			fIsNext = false;
			if (fEntry)
				error = B_OK;
		} else
			error = fDirectory->GetNextEntry(&fEntry);
		*entry = fEntry;
	}
	fDone = (error != B_OK);
	return error;
}

// Rewind
status_t
EntryIterator::Rewind()
{
	status_t error = (fDirectory ? B_OK : B_ERROR);
	if (error == B_OK) {
		if (fDirectory->GetVolume()->IteratorLock()) {
			if (fSuspended && fEntry)
				fEntry->DetachEntryIterator(this);
			fEntry = NULL;
			fIsNext = false;
			fDone = false;
			fDirectory->GetVolume()->IteratorUnlock();
		} else
			error = B_ERROR;
	}
	return error;
}

// SetCurrent
void
EntryIterator::SetCurrent(Entry *entry, bool isNext)
{
	fIsNext = isNext;
	fEntry = entry;
	fDone = !fEntry;
}

