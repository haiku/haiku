/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef INDEX_DIRECTORY_H
#define INDEX_DIRECTORY_H

#include "List.h"

class AttributeIndex;
class Index;
class LastModifiedIndex;
class NameIndex;
class SizeIndex;
class Volume;

class IndexDirectory {
public:
	IndexDirectory(Volume *volume);
	~IndexDirectory();

	status_t InitCheck() const;

	status_t CreateIndex(const char *name, uint32 type,
						 AttributeIndex **index = NULL);
	bool DeleteIndex(const char *name, uint32 type);
	bool DeleteIndex(Index *index);

	Index *FindIndex(const char *name);
	Index *FindIndex(const char *name, uint32 type);
	AttributeIndex *FindAttributeIndex(const char *name);
	AttributeIndex *FindAttributeIndex(const char *name, uint32 type);

	bool IsSpecialIndex(Index *index) const;
	NameIndex *GetNameIndex() const			{ return fNameIndex; }
	LastModifiedIndex *GetLastModifiedIndex() const
		{ return fLastModifiedIndex; }
	SizeIndex *GetSizeIndex() const			{ return fSizeIndex; }

	Index *IndexAt(int32 index) const	{ return fIndices.ItemAt(index); }

private:
	Volume				*fVolume;
	NameIndex			*fNameIndex;
	LastModifiedIndex	*fLastModifiedIndex;
	SizeIndex			*fSizeIndex;
	List<Index*>		fIndices;
};

#endif	// INDEX_DIRECTORY_H
