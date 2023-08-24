/* Query - query parsing and evaluation
 *
 * Copyright 2001-2004, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 *
 * Adjusted by Ingo Weinhold <bonefish@cs.tu-berlin.de> for usage in RAM FS.
 */
#ifndef QUERY_H
#define QUERY_H


#include <OS.h>
#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>

#include "Index.h"
#include "ramfs.h"


namespace QueryParser {
	template<typename QueryPolicy> class Query;
};

class Node;
class Volume;


#define B_QUERY_NON_INDEXED	0x00000002


class Query : public DoublyLinkedListLinkImpl<Query> {
public:
							~Query();

	static	status_t		Create(Volume* volume, const char* queryString,
								uint32 flags, port_id port, uint32 token,
								Query*& _query);

			status_t		Rewind();
			status_t		GetNextEntry(struct dirent* entry, size_t size);

			void			LiveUpdate(Entry* entry, Node* node,
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


#endif	/* QUERY_H */
