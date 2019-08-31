/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef NODE_TABLE_H
#define NODE_TABLE_H

#include <util/OpenHashTable.h>

#include "AllocationInfo.h"
#include "Node.h"

// NodeHash
struct NodeHash {
	typedef ino_t		KeyType;
	typedef	Node		ValueType;

	size_t HashKey(KeyType key) const
	{
		return uint32(key & 0xffffffff);
	}

	size_t Hash(ValueType* value) const
	{
		return HashKey(value->GetID());
	}

	bool Compare(KeyType key, ValueType* value) const
	{
		return value->GetID() == key;
	}

	ValueType*& GetLink(ValueType* value) const
	{
		return value->HashLink();
	}
};

// NodeTable
class NodeTable {
public:
	NodeTable();
	~NodeTable();

	status_t InitCheck() const;

	status_t AddNode(Node *node);
	status_t RemoveNode(Node *node);
	status_t RemoveNode(ino_t id);
	Node *GetNode(ino_t id);

	// debugging
	void GetAllocationInfo(AllocationInfo &info);

private:
	BOpenHashTable<NodeHash> fNodes;
	status_t fInitStatus;
};

#endif	// NODE_TABLE_H
