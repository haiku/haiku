/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Query.h"

#include <file_systems/QueryParser.h>

#include "AttributeCookie.h"
#include "Directory.h"
#include "Index.h"
#include "Node.h"
#include "Volume.h"


// #pragma mark - QueryPolicy


struct Query::QueryPolicy {
	typedef Query Context;
	typedef ::Node Entry;
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
		::Index*			index;

		IndexIterator(::Index* index)
			:
			index(index)
		{
		}
	};

	static const int32 kMaxFileNameLength = B_FILE_NAME_LENGTH;

	// Entry interface

	static ino_t EntryGetParentID(Entry* entry)
	{
		return entry->Parent()->ID();
	}

	static Node* EntryGetNode(Entry* entry)
	{
		return entry;
	}

	static ino_t EntryGetNodeID(Entry* entry)
	{
		return entry->ID();
	}

	static ssize_t EntryGetName(Entry* entry, void* buffer, size_t bufferSize)
	{
		const char* name = entry->Name();
		size_t nameLength = strlen(name);
		if (nameLength >= bufferSize)
			return B_BUFFER_OVERFLOW;

		memcpy(buffer, name, nameLength + 1);
		return nameLength + 1;
	}

	static const char* EntryGetNameNoCopy(Entry* entry, void* buffer,
		size_t bufferSize)
	{
		return entry->Name();
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
		static const int32 maxFactor = 1024 * 1024;
		return score * (maxFactor
			/ std::min(maxFactor,
				std::max((int32)1, index.index->CountEntries())));
	}

	static type_code IndexGetType(Index& index)
	{
		return index.index->Type();
	}

	static int32 IndexGetKeySize(Index& index)
	{
		return index.index->KeyLength();
	}

	static IndexIterator* IndexCreateIterator(Index& index)
	{
		IndexIterator* iterator = new(std::nothrow) IndexIterator(index.index);
		if (iterator == NULL)
			return NULL;

		if (!index.index->GetIterator(iterator)) {
			delete iterator;
			return NULL;
		}

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
		if (!indexIterator->index->Find(value, size, indexIterator))
			return B_ENTRY_NOT_FOUND;

		return B_OK;
	}

	static status_t IndexIteratorGetNextEntry(IndexIterator* indexIterator,
		void* value, size_t* _valueLength, size_t bufferSize, Entry** _entry)
	{
		Node* node = indexIterator->Next(value, _valueLength);
		if (node == NULL)
			return B_ENTRY_NOT_FOUND;

		*_entry = node;
		return B_OK;
	}

	static void IndexIteratorSuspend(IndexIterator* indexIterator)
	{
		indexIterator->Suspend();
	}

	static void IndexIteratorResume(IndexIterator* indexIterator)
	{
		indexIterator->Resume();
	}

	// Node interface

	static const off_t NodeGetSize(Node* node)
	{
		return node->FileSize();
	}

	static time_t NodeGetLastModifiedTime(Node* node)
	{
		return node->ModifiedTime().tv_sec;
	}

	static status_t NodeGetAttribute(Node* node, const char* attribute,
		void* buffer, size_t* _size, int32* _type)
	{
		// TODO: Creating a cookie is quite a bit of overhead.
		AttributeCookie* cookie;
		status_t error = node->OpenAttribute(attribute, O_RDONLY, cookie);
		if (error != B_OK)
			return error;

		error = cookie->ReadAttribute(0, buffer, _size);

		cookie->Close();
		delete cookie;

		return error;
	}

	static Entry* NodeGetFirstReferrer(Node* node)
	{
		return node;
	}

	static Entry* NodeGetNextReferrer(Node* node, Entry* entry)
	{
		return NULL;
	}

	// Volume interface

	static dev_t ContextGetVolumeID(Context* context)
	{
		return context->fVolume->ID();
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
Query::LiveUpdate(Node* node, const char* attribute, int32 type,
	const void* oldKey, size_t oldLength, const void* newKey, size_t newLength)
{
	fImpl->LiveUpdate(node, node, attribute, type, (const uint8*)oldKey,
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
