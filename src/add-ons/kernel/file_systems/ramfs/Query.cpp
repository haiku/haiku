/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "Query.h"

#include "Directory.h"
#include "Entry.h"
#include "Node.h"
#include "Volume.h"
#include "Index.h"

#include <file_systems/QueryParser.h>


// #pragma mark - IndexIterator


class IndexIterator {
public:
	IndexIterator(Index *index);

	status_t Find(const uint8 *const key, size_t keyLength);
	status_t Rewind();
	status_t GetNextEntry(uint8 *buffer, size_t *keyLength, Entry **entry);

	Index* GetIndex() const { return fIndex; }

private:
	Index				*fIndex;
	IndexEntryIterator	fIterator;
	bool				fInitialized;
};


IndexIterator::IndexIterator(Index *index)
	: fIndex(index),
	  fIterator(),
	  fInitialized(false)
{
}


status_t
IndexIterator::Find(const uint8 *const key, size_t keyLength)
{
	status_t error = B_ENTRY_NOT_FOUND;
	if (fIndex) {
		// TODO: We actually don't want an exact Find() here, but rather a
		// FindClose().
		fInitialized = fIndex->Find(key, keyLength, &fIterator);
		if (fInitialized)
			error = B_OK;
	}
	return error;
}


status_t
IndexIterator::Rewind()
{
	status_t error = B_ENTRY_NOT_FOUND;
	if (fIndex) {
		fInitialized = fIndex->GetIterator(&fIterator);
		if (fInitialized)
			error = B_OK;
	}
	return error;
}


status_t
IndexIterator::GetNextEntry(uint8 *buffer, size_t *_keyLength, Entry **_entry)
{
	status_t error = B_ENTRY_NOT_FOUND;
	if (fIndex) {
		// init iterator, if not done yet
		if (!fInitialized) {
			fIndex->GetIterator(&fIterator);
			fInitialized = true;
		}

		// get key
		size_t keyLength;
		if (Entry *entry = fIterator.GetCurrent(buffer, &keyLength)) {
			*_keyLength = keyLength;
			*_entry = entry;
			error = B_OK;
		}

		// get next entry
		fIterator.GetNext();
	}
	return error;
}


// #pragma mark - QueryPolicy


struct Query::QueryPolicy {
	typedef Query Context;
	typedef ::Entry Entry;
	typedef ::Node Node;

	struct Index {
		Query*		query;
		::Index*	index;

		Index(Context* context)
			:
			query(context)
		{
		}
	};

	struct IndexIterator : ::IndexIterator {
		IndexIterator(::Index* index)
			:
			::IndexIterator(index)
		{
		}
	};

	static const int32 kMaxFileNameLength = B_FILE_NAME_LENGTH;

	// Entry interface

	static ino_t EntryGetParentID(Entry* entry)
	{
		return entry->GetParent()->GetID();
	}

	static Node* EntryGetNode(Entry* entry)
	{
		return entry->GetNode();
	}

	static ino_t EntryGetNodeID(Entry* entry)
	{
		return entry->GetNode()->GetID();
	}

	static ssize_t EntryGetName(Entry* entry, void* buffer, size_t bufferSize)
	{
		const char* name = entry->GetName();
		size_t nameLength = strlen(name);
		if (nameLength >= bufferSize)
			return B_BUFFER_OVERFLOW;

		memcpy(buffer, name, nameLength + 1);
		return nameLength + 1;
	}

	static const char* EntryGetNameNoCopy(Entry* entry, void* buffer,
		size_t bufferSize)
	{
		return entry->GetName();
	}

	// Index interface

	static status_t IndexSetTo(Index& index, const char* attribute)
	{
		index.index = index.query->fVolume->FindIndex(attribute);
		return index.index != NULL ? B_OK : B_ENTRY_NOT_FOUND;
	}

	static void IndexUnset(Index& index)
	{
		index.index = NULL;
	}

	static int32 IndexGetWeightedScore(Index& index, int32 score)
	{
		// should be inversely proportional to the index size; max input score
		// is 2048
		static const int32 maxFactor = (1024 * 1024) - 1;
		return score * (maxFactor /
			std::min(maxFactor, std::max((int32)1, index.index->CountEntries())));
	}

	static type_code IndexGetType(Index& index)
	{
		return index.index->GetType();
	}

	static int32 IndexGetKeySize(Index& index)
	{
		return index.index->GetKeyLength();
	}

	static IndexIterator* IndexCreateIterator(Index& index)
	{
		IndexIterator* iterator = new(std::nothrow) IndexIterator(index.index);
		if (iterator == NULL)
			return NULL;

		return iterator;
	}

	// IndexIterator interface

	static void IndexIteratorDelete(IndexIterator* indexIterator)
	{
		delete indexIterator;
	}

	static status_t IndexIteratorFind(IndexIterator* indexIterator,
		const void* value, size_t size)
	{
		return indexIterator->Find((const uint8*)value, size);
	}

	static status_t IndexIteratorGetNextEntry(IndexIterator* indexIterator,
		void* value, size_t* _valueLength, size_t bufferSize, Entry** _entry)
	{
		return indexIterator->GetNextEntry((uint8*)value, _valueLength, _entry);
	}

	static void IndexIteratorSuspend(IndexIterator* indexIterator)
	{
		// Nothing to do.
	}

	static void IndexIteratorResume(IndexIterator* indexIterator)
	{
		// Nothing to do.
	}

	// Node interface

	static const off_t NodeGetSize(Node* node)
	{
		return node->GetSize();
	}

	static time_t NodeGetLastModifiedTime(Node* node)
	{
		return node->GetMTime();
	}

	static status_t NodeGetAttribute(Node* node, const char* attribute,
		void* buffer, size_t* _size, int32* _type)
	{
		Attribute* attr = NULL;
		status_t error = node->FindAttribute(attribute, &attr);
		if (error != B_OK)
			return error;

		*_type = attr->GetType();
		error = attr->ReadAt(0, buffer, *_size, _size);

		return error;
	}

	static Entry* NodeGetFirstReferrer(Node* node)
	{
		return node->GetFirstReferrer();
	}

	static Entry* NodeGetNextReferrer(Node* node, Entry* entry)
	{
		return node->GetNextReferrer(entry);
	}

	// Volume interface

	static dev_t ContextGetVolumeID(Context* context)
	{
		return context->fVolume->GetID();
	}
};


// #pragma mark - Query


Query::Query(Volume* volume)
	:
	fVolume(volume),
	fImpl(NULL)
{
}


Query::~Query()
{
	if (fImpl != NULL) {
		if ((fImpl->Flags() & B_LIVE_QUERY) != 0)
			fVolume->RemoveQuery(this);

		delete fImpl;
	}
}


/*static*/ status_t
Query::Create(Volume* volume, const char* queryString, uint32 flags,
	port_id port, uint32 token, Query*& _query)
{
	Query* query = new(std::nothrow) Query(volume);
	if (query == NULL)
		return B_NO_MEMORY;

	status_t error = query->_Init(queryString, flags, port, token);
	if (error != B_OK) {
		delete query;
		return error;
	}

	_query = query;
	return B_OK;
}


status_t
Query::Rewind()
{
	return fImpl->Rewind();
}


status_t
Query::GetNextEntry(struct dirent* entry, size_t size)
{
	return fImpl->GetNextEntry(entry, size);
}


void
Query::LiveUpdate(Entry* entry, Node* node, const char* attribute, int32 type,
	const void* oldKey, size_t oldLength, const void* newKey, size_t newLength)
{
	fImpl->LiveUpdate(entry, node, attribute, type, (const uint8*)oldKey,
		oldLength, (const uint8*)newKey, newLength);
}


status_t
Query::_Init(const char* queryString, uint32 flags, port_id port, uint32 token)
{
	status_t error = QueryImpl::Create(this, queryString, flags, port, token,
		fImpl);
	if (error != B_OK)
		return error;

	if ((fImpl->Flags() & B_LIVE_QUERY) != 0)
		fVolume->AddQuery(this);

	return B_OK;
}
