/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include "Node.h"


class Directory : public Node {
public:
								Directory(Volume* volume, uint64 blockIndex,
									const checksumfs_node& nodeData);
								Directory(Volume* volume, mode_t mode);
	virtual						~Directory();

	virtual	void				DeletingNode();

			status_t			LookupEntry(const char* name,
									uint64& _blockIndex);
			status_t			LookupNextEntry(const char* name,
									char* foundName, size_t& _foundNameLength,
									uint64& _blockIndex);

			status_t			InsertEntry(const char* name, uint64 blockIndex,
									Transaction& transaction);
			status_t			RemoveEntry(const char* name,
									Transaction& transaction,
									bool* _lastEntryRemoved = NULL);
};


#endif	// DIRECTORY_H
