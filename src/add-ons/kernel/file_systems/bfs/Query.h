/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef QUERY_H
#define QUERY_H

#include "system_dependencies.h"

#include "Index.h"


namespace QueryParser {
	template<typename QueryPolicy> class Query;
};

class Volume;


class Query : public DoublyLinkedListLinkImpl<Query> {
public:
							~Query();

	static	status_t		Create(Volume* volume, const char* queryString,
								uint32 flags, port_id port, uint32 token,
								Query*& _query);

			status_t		Rewind();
			status_t		GetNextEntry(struct dirent* entry, size_t size);

			void			LiveUpdate(Inode* inode,
								const char* attribute, int32 type,
								const void* oldKey, size_t oldLength,
								const void* newKey, size_t newLength);
			void			LiveUpdateRenameMove(Inode* node,
								ino_t oldDirectoryID, const char* oldName,
								size_t oldLength, ino_t newDirectoryID,
								const char* newName, size_t newLength);
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


#endif	// QUERY_H
