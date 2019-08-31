/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DebugSupport.h"
#include "NodeTable.h"

// constructor
NodeTable::NodeTable()
{
	fInitStatus = fNodes.Init(1000);
}

// destructor
NodeTable::~NodeTable()
{
}

// InitCheck
status_t
NodeTable::InitCheck() const
{
	RETURN_ERROR(fInitStatus);
}

// AddNode
status_t
NodeTable::AddNode(Node *node)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (fNodes.Lookup(node->GetID()) != NULL)
			fNodes.Remove(node);
		SET_ERROR(error, fNodes.Insert(node));
	}
	return error;
}

// RemoveNode
status_t
NodeTable::RemoveNode(Node *node)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = RemoveNode(node->GetID());
	return error;
}

// RemoveNode
status_t
NodeTable::RemoveNode(ino_t id)
{
	status_t error = B_OK;
	if (Node *element = fNodes.Lookup(id))
		fNodes.Remove(element);
	else
		error = B_ERROR;
	return error;
}

// GetNode
Node *
NodeTable::GetNode(ino_t id)
{
	Node *node = fNodes.Lookup(id);
	return node;
}

// GetAllocationInfo
void
NodeTable::GetAllocationInfo(AllocationInfo &info)
{
	info.AddNodeTableAllocation(0, fNodes.TableSize(),
		sizeof(Node*), fNodes.CountElements());
}
