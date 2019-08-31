/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DebugSupport.h"
#include "Directory.h"
#include "Entry.h"
#include "Index.h"
#include "IndexImpl.h"

// Index

// constructor
Index::Index(Volume *volume, const char *name, uint32 type,
			 bool fixedKeyLength, size_t keyLength)
	: fVolume(volume),
	  fInitStatus(B_OK),
	  fName(name),
	  fType(type),
	  fKeyLength(keyLength),
	  fFixedKeyLength(fixedKeyLength)
{
	if (!fVolume)
		fInitStatus = B_BAD_VALUE;
	else if (!fName.GetString())
		fInitStatus = B_NO_MEMORY;
}

// destructor
Index::~Index()
{
}

// InitCheck
status_t
Index::InitCheck() const
{
	return fInitStatus;
}

// GetIterator
bool
Index::GetIterator(IndexEntryIterator *iterator)
{
	bool result = false;
	if (iterator) {
		AbstractIndexEntryIterator *actualIterator = InternalGetIterator();
		if (actualIterator) {
			iterator->SetIterator(actualIterator);
			result = true;
		}
	}
	return result;
}

// Find
bool
Index::Find(const uint8 *key, size_t length, IndexEntryIterator *iterator)
{
	bool result = false;
	if (key && iterator) {
		AbstractIndexEntryIterator *actualIterator
			= InternalFind(key, length);
		if (actualIterator) {
			iterator->SetIterator(actualIterator);
			result = true;
		}
	}
	return result;
}

// Dump
void
Index::Dump()
{
	D(
		PRINT("Index: `%s', type: %lx\n", GetName(), GetType());
		for (IndexEntryIterator it(this); it.GetCurrent(); it.GetNext()) {
			Entry *entry = it.GetCurrent();
			PRINT("  entry: `%s', dir: %Ld\n", entry->GetName(),
												entry->GetParent()->GetID());
		}
	)
}


// IndexEntryIterator

// constructor
IndexEntryIterator::IndexEntryIterator()
	: fIterator(NULL)
{
}

// constructor
IndexEntryIterator::IndexEntryIterator(Index *index)
	: fIterator(NULL)
{
	if (index)
		index->GetIterator(this);
}

// destructor
IndexEntryIterator::~IndexEntryIterator()
{
	SetIterator(NULL);
}

// GetCurrent
Entry *
IndexEntryIterator::GetCurrent()
{
	return (fIterator ? fIterator->GetCurrent() : NULL);
}

// GetCurrent
Entry *
IndexEntryIterator::GetCurrent(uint8 *buffer, size_t *keyLength)
{
	return (fIterator ? fIterator->GetCurrent(buffer, keyLength) : NULL);
}

// GetPrevious
Entry *
IndexEntryIterator::GetPrevious()
{
	return (fIterator ? fIterator->GetPrevious() : NULL);
}

// GetNext
Entry *
IndexEntryIterator::GetNext()
{
	return (fIterator ? fIterator->GetNext() : NULL);
}

// GetNext
Entry *
IndexEntryIterator::GetNext(uint8 *buffer, size_t *keyLength)
{
	Entry *entry = NULL;
	if (fIterator && fIterator->GetNext())
		entry = GetCurrent(buffer, keyLength);
	return entry;
}

// Suspend
status_t
IndexEntryIterator::Suspend()
{
	return (fIterator ? fIterator->Suspend() : B_BAD_VALUE);
}

// Resume
status_t
IndexEntryIterator::Resume()
{
	return (fIterator ? fIterator->Resume() : B_BAD_VALUE);
}

// SetIterator
void
IndexEntryIterator::SetIterator(AbstractIndexEntryIterator *iterator)
{
	if (fIterator)
		delete fIterator;
	fIterator = iterator;
}


// AbstractIndexEntryIterator

// constructor
AbstractIndexEntryIterator::AbstractIndexEntryIterator()
{
}

// destructor
AbstractIndexEntryIterator::~AbstractIndexEntryIterator()
{
}

// Suspend
status_t
AbstractIndexEntryIterator::Suspend()
{
	return B_OK;
}

// Resume
status_t
AbstractIndexEntryIterator::Resume()
{
	return B_OK;
}

