/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef QUERY_H
#define QUERY_H


#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>


struct dirent;

namespace QueryParser {
	template<typename QueryPolicy> class Query;
};

class Node;
class Volume;


class Query : public DoublyLinkedListLinkImpl<Query> {
public:
							~Query();

	static	status_t		Create(Volume* volume, const char* queryString,
								uint32 flags, port_id port, uint32 token,
								Query*& _query);

			status_t		Rewind();
			status_t		GetNextEntry(struct dirent* entry, size_t size);

			void			LiveUpdate(Node* node,
								const char* attribute, int32 type,
								const void* oldKey, size_t oldLength,
								const void* newKey, size_t newLength);

private:
			struct QueryPolicy;
			friend struct QueryPolicy;
			typedef QueryParser::Query<QueryPolicy> QueryImpl;

private:
							Query(Volume* volume);

			status_t		_Init(const char* queryString, uint32 flags,
								port_id port, uint32 token);

private:
			Volume*			fVolume;
			QueryImpl*		fImpl;
};


typedef DoublyLinkedList<Query> QueryList;


#endif	// QUERY_H
