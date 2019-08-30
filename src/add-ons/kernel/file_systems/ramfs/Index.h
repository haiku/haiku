/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef INDEX_H
#define INDEX_H

#include <SupportDefs.h>

#include "String.h"

class AbstractIndexEntryIterator;
class Entry;
class IndexEntryIterator;
class Node;
class Volume;

// Index
class Index {
public:
	Index(Volume *volume, const char *name, uint32 type,
		  bool fixedKeyLength, size_t keyLength = 0);
	virtual ~Index();

	status_t InitCheck() const;

	Volume *GetVolume() const		{ return fVolume; }
	void GetVolume(Volume *volume)	{ fVolume = volume; }

	const char *GetName() const		{ return fName.GetString(); }
	uint32 GetType() const			{ return fType; }
	bool HasFixedKeyLength() const	{ return fFixedKeyLength; }
	size_t GetKeyLength() const		{ return fKeyLength; }

	virtual int32 CountEntries() const = 0;

	bool GetIterator(IndexEntryIterator *iterator);
	bool Find(const uint8 *key, size_t length,
			  IndexEntryIterator *iterator);

	// debugging
	void Dump();

protected:
	virtual AbstractIndexEntryIterator *InternalGetIterator() = 0;
	virtual AbstractIndexEntryIterator *InternalFind(const uint8 *key,
													 size_t length) = 0;

protected:
	Volume		*fVolume;
	status_t	fInitStatus;
	String		fName;
	uint32		fType;
	size_t		fKeyLength;
	bool		fFixedKeyLength;
};

// IndexEntryIterator
class IndexEntryIterator {
public:
	IndexEntryIterator();
	IndexEntryIterator(Index *index);
	~IndexEntryIterator();

	Entry *GetCurrent();
	Entry *GetCurrent(uint8 *buffer, size_t *keyLength);
	Entry *GetPrevious();
	Entry *GetNext();
	Entry *GetNext(uint8 *buffer, size_t *keyLength);

	status_t Suspend();
	status_t Resume();

private:
	void SetIterator(AbstractIndexEntryIterator *iterator);

private:
	friend class Index;

	AbstractIndexEntryIterator	*fIterator;
};

#endif	// INDEX_H
