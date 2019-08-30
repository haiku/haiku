/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <TypeConstants.h>

#include "AttributeIndexImpl.h"
#include "DebugSupport.h"
#include "IndexDirectory.h"
#include "LastModifiedIndex.h"
#include "NameIndex.h"
#include "SizeIndex.h"

// constructor
IndexDirectory::IndexDirectory(Volume *volume)
	: fVolume(volume),
	  fNameIndex(NULL),
	  fLastModifiedIndex(NULL),
	  fSizeIndex(NULL),
	  fIndices()
{
	fNameIndex = new(nothrow) NameIndex(volume);
	fLastModifiedIndex = new(nothrow) LastModifiedIndex(volume);
	fSizeIndex = new(nothrow) SizeIndex(volume);
	if (fNameIndex && fLastModifiedIndex && fSizeIndex) {
		if (!fIndices.AddItem(fNameIndex)
			|| !fIndices.AddItem(fLastModifiedIndex)
			|| !fIndices.AddItem(fSizeIndex)) {
			fIndices.MakeEmpty();
			delete fNameIndex;
			delete fLastModifiedIndex;
			delete fSizeIndex;
			fNameIndex = NULL;
			fLastModifiedIndex = NULL;
			fSizeIndex = NULL;
		}
	}
}

// destructor
IndexDirectory::~IndexDirectory()
{
	// delete the default indices
	if (fNameIndex) {
		fIndices.RemoveItem(fNameIndex);
		delete fNameIndex;
	}
	if (fLastModifiedIndex) {
		fIndices.RemoveItem(fLastModifiedIndex);
		delete fLastModifiedIndex;
	}
	if (fSizeIndex) {
		fIndices.RemoveItem(fSizeIndex);
		delete fSizeIndex;
	}
	// delete the attribute indices
	int32 count = fIndices.CountItems();
	for (int i = 0; i < count; i++)
		delete fIndices.ItemAt(i);
}

// InitCheck
status_t
IndexDirectory::InitCheck() const
{
	return (fNameIndex && fLastModifiedIndex && fSizeIndex ? B_OK
														   : B_NO_MEMORY);
}

// CreateIndex
status_t
IndexDirectory::CreateIndex(const char *name, uint32 type,
							AttributeIndex **_index)
{
	status_t error = (name ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!FindIndex(name)) {
			// create the index
			AttributeIndex *index = NULL;
			switch (type) {
				case B_INT32_TYPE:
					index = new(nothrow) AttributeIndexImpl(fVolume,
						name, type, sizeof(int32));
					break;
				case B_UINT32_TYPE:
					index = new(nothrow) AttributeIndexImpl(fVolume,
						name, type, sizeof(uint32));
					break;
				case B_INT64_TYPE:
					index = new(nothrow) AttributeIndexImpl(fVolume,
						name, type, sizeof(int64));
					break;
				case B_UINT64_TYPE:
					index = new(nothrow) AttributeIndexImpl(fVolume,
						name, type, sizeof(uint64));
					break;
				case B_FLOAT_TYPE:
					index = new(nothrow) AttributeIndexImpl(fVolume,
						name, type, sizeof(float));
					break;
				case B_DOUBLE_TYPE:
					index = new(nothrow) AttributeIndexImpl(fVolume,
						name, type, sizeof(double));
					break;
				case B_STRING_TYPE:
					index = new(nothrow) AttributeIndexImpl(fVolume,
						name, type, 0);
					break;
				default:
					error = B_BAD_VALUE;
					break;
			}
			if (error == B_OK && !index)
				error = B_NO_MEMORY;
			// add the index
			if (error == B_OK) {
				if (fIndices.AddItem(index)) {
					if (_index)
						*_index = index;
				} else {
					delete index;
					error = B_NO_MEMORY;
				}
			}
		} else
			error = B_FILE_EXISTS;
	}
	return error;
}

// DeleteIndex
bool
IndexDirectory::DeleteIndex(const char *name, uint32 type)
{
	return DeleteIndex(FindIndex(name, type));
}

// DeleteIndex
bool
IndexDirectory::DeleteIndex(Index *index)
{
	bool result = false;
	if (index && !IsSpecialIndex(index)) {
		int32 i = fIndices.IndexOf(index);
		if (i >= 0) {
			fIndices.RemoveItem(i);
			delete index;
			result = true;
		}
	}
	return result;
}

// FindIndex
Index *
IndexDirectory::FindIndex(const char *name)
{
	if (name) {
		int32 count = fIndices.CountItems();
		for (int32 i = 0; i < count; i++) {
			Index *index = fIndices.ItemAt(i);
			if (!strcmp(index->GetName(), name))
				return index;
		}
	}
	return NULL;
}

// FindIndex
Index *
IndexDirectory::FindIndex(const char *name, uint32 type)
{
	Index *index = FindIndex(name);
	if (index && index->GetType() != type)
		index = NULL;
	return index;
}

// FindAttributeIndex
AttributeIndex *
IndexDirectory::FindAttributeIndex(const char *name)
{
	AttributeIndex *attrIndex = NULL;
	if (Index *index = FindIndex(name))
		attrIndex = dynamic_cast<AttributeIndex*>(index);
	return attrIndex;
}

// FindAttributeIndex
AttributeIndex *
IndexDirectory::FindAttributeIndex(const char *name, uint32 type)
{
	AttributeIndex *attrIndex = NULL;
	if (Index *index = FindIndex(name, type))
		attrIndex = dynamic_cast<AttributeIndex*>(index);
	return attrIndex;
}

// IsSpecialIndex
bool
IndexDirectory::IsSpecialIndex(Index *index) const
{
	return (index == fNameIndex || index == fLastModifiedIndex
			|| index == fSizeIndex);
}

