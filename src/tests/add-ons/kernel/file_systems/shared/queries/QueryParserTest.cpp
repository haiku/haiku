/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>

#define DEBUG_QUERY
#define PRINT(expr) printf expr
#define __out(message...) PRINT((message))
#define QUERY_INFORM __out
#define QUERY_FATAL(message...) { fprintf(stderr, message); abort(); }
#define QUERY_D(block) block
#include <file_systems/QueryParser.h>


class Entry;


class Query {
public:
	static	status_t		Create(void* volume, const char* queryString,
								uint32 flags, port_id port, uint32 token,
								Query*& _query);

private:
	struct QueryPolicy;
	friend struct QueryPolicy;
	typedef QueryParser::Query<QueryPolicy> QueryImpl;

private:
							Query();

			status_t		_Init(const char* queryString, uint32 flags,
								port_id port, uint32 token);

private:
			QueryImpl*		fImpl;
};


struct Query::QueryPolicy {
	typedef Query Context;
	typedef ::Entry Entry;
	typedef ::Entry Node;
	typedef void* NodeHolder;

	struct Index {
		Query*		query;

		Index(Context* context)
			:
			query(context)
		{
		}
	};

	struct IndexIterator {
		Entry* entry;
	};

	static const int32 kMaxFileNameLength = B_FILE_NAME_LENGTH;

	// Entry interface

	static ino_t EntryGetParentID(Entry* entry)
	{
		return -1;
	}

	static Node* EntryGetNode(Entry* entry)
	{
		return entry;
	}

	static ino_t EntryGetNodeID(Entry* entry)
	{
		return -1;
	}

	static ssize_t EntryGetName(Entry* entry, void* buffer, size_t bufferSize)
	{
		return B_ERROR;
	}

	static const char* EntryGetNameNoCopy(NodeHolder& holder, Entry* entry)
	{
		return NULL;
	}

	// Index interface

	static status_t IndexSetTo(Index& index, const char* attribute)
	{
		return B_ERROR;
	}

	static void IndexUnset(Index& index)
	{
	}

	static int32 IndexGetSize(Index& index)
	{
		return 0;
	}

	static type_code IndexGetType(Index& index)
	{
		return 0;
	}

	static int32 IndexGetKeySize(Index& index)
	{
		return 0;
	}

	static IndexIterator* IndexCreateIterator(Index& index)
	{
		return NULL;
	}

	// IndexIterator interface

	static void IndexIteratorDelete(IndexIterator* indexIterator)
	{
		delete indexIterator;
	}

	static status_t IndexIteratorFind(IndexIterator* indexIterator,
		const void* value, size_t size)
	{
		return B_ERROR;
	}

	static status_t IndexIteratorFetchNextEntry(IndexIterator* indexIterator,
		void* value, size_t* _valueLength, size_t bufferSize, size_t* duplicate)
	{
		return B_ERROR;
	}

	static status_t IndexIteratorGetEntry(Context* context, IndexIterator* indexIterator,
		NodeHolder& holder, Entry** _entry)
	{
		*_entry = indexIterator->entry;
		return B_OK;
	}

	static void IndexIteratorSkipDuplicates(IndexIterator* indexIterator)
	{
	}

	static void IndexIteratorSuspend(IndexIterator* indexIterator)
	{
	}

	static void IndexIteratorResume(IndexIterator* indexIterator)
	{
	}

	// Node interface

	static const off_t NodeGetSize(Node* node)
	{
		return 0;
	}

	static time_t NodeGetLastModifiedTime(Node* node)
	{
		return 0;
	}

	static status_t NodeGetAttribute(NodeHolder& nodeHolder, Node* node,
		const char* attribute, void* buffer, size_t* _size, int32* _type)
	{
		return B_ERROR;
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
		return 0;
	}
};


/*static*/ status_t
Query::Create(void* volume, const char* queryString, uint32 flags,
	port_id port, uint32 token, Query*& _query)
{
	Query* query = new(std::nothrow) Query();
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


Query::Query()
	:
	fImpl(NULL)
{
}


status_t
Query::_Init(const char* queryString, uint32 flags, port_id port, uint32 token)
{
	status_t error = QueryImpl::Create(this, queryString, flags, port, token,
		fImpl);
	if (error != B_OK)
		return error;

	return B_OK;
}


int
main(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++) {
		Query* query;
		status_t error = Query::Create(NULL, argv[i], 0, 0, 0, query);
		if (error != B_OK) {
			fprintf(stderr, "Error creating query %d: %s\n", i - 1, strerror(error));
			continue;
		}
		delete query;
	}

	return 0;
}
