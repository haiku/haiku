/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Transaction.h"

#include <algorithm>

#include "BlockAllocator.h"
#include "Volume.h"


static inline bool
swap_if_greater(Node*& a, Node*& b)
{
	if (a->BlockIndex() <= b->BlockIndex())
		return false;

	std::swap(a, b);
	return true;
}


// #pragma mark - Transaction


Transaction::Transaction(Volume* volume)
	:
	fVolume(volume),
	fID(-1)
{
}


Transaction::~Transaction()
{
	Abort();
}


status_t
Transaction::Start()
{
	ASSERT(fID < 0);

	fVolume->TransactionStarted();

	fID = cache_start_transaction(fVolume->BlockCache());
	if (fID < 0) {
		fVolume->TransactionFinished();
		return fID;
	}

	fOldFreeBlockCount = fVolume->GetBlockAllocator()->FreeBlocks();

	return B_OK;
}


status_t
Transaction::StartAndAddNode(Node* node, uint32 flags = 0)
{
	status_t error = Start();
	if (error != B_OK)
		return error;

	return AddNode(node, flags);
}


status_t
Transaction::Commit(const PostCommitNotification* notification)
{
	ASSERT(fID >= 0);

	// flush the nodes
	for (NodeInfoList::Iterator it = fNodeInfos.GetIterator();
			NodeInfo* info = it.Next();) {
		status_t error = info->node->Flush(*this);
		if (error != B_OK) {
			Abort();
			return error;
		}
	}

	// commit the cache transaction
	status_t error = cache_end_transaction(fVolume->BlockCache(), fID, NULL,
		NULL);
	if (error != B_OK) {
		Abort();
		return error;
	}

	// send notifications
	if (notification != NULL)
		notification->NotifyPostCommit();

	// clean up
	_DeleteNodeInfosAndUnlock(false);

	fVolume->TransactionFinished();
	fID = -1;

	return B_OK;
}


void
Transaction::Abort()
{
	if (fID < 0)
		return;

	// abort the cache transaction
	cache_abort_transaction(fVolume->BlockCache(), fID);

	// revert the nodes
	for (NodeInfoList::Iterator it = fNodeInfos.GetIterator();
			NodeInfo* info = it.Next();) {
		info->node->RevertNodeData(info->oldNodeData);
	}

	// clean up
	_DeleteNodeInfosAndUnlock(true);

	fVolume->GetBlockAllocator()->ResetFreeBlocks(fOldFreeBlockCount);

	fVolume->TransactionFinished();
	fID = -1;
}


status_t
Transaction::AddNode(Node* node, uint32 flags)
{
	NodeInfo* info = _GetNodeInfo(node);
	if (info != NULL)
		return B_OK;

	info = new(std::nothrow) NodeInfo;
	if (info == NULL)
		return B_NO_MEMORY;

	if ((flags & TRANSACTION_NODE_ALREADY_LOCKED) == 0)
		node->WriteLock();

	info->node = node;
	info->oldNodeData = node->NodeData();
	info->flags = flags;

	fNodeInfos.Add(info);

	return B_OK;
}


status_t
Transaction::AddNodes(Node* node1, Node* node2, Node* node3)
{
	// sort the nodes
	swap_if_greater(node1, node2);
	if (node3 != NULL && swap_if_greater(node2, node3))
		swap_if_greater(node1, node2);

	// add them
	status_t error = AddNode(node1);
	if (error == B_OK)
		error = AddNode(node2);
	if (error == B_OK && node3 != NULL)
		AddNode(node3);

	return error;
}


void
Transaction::KeepNode(Node* node)
{
	NodeInfo* info = _GetNodeInfo(node);
	if (info == NULL)
		return;

	info->flags &= ~(uint32)TRANSACTION_DELETE_NODE;
}


Transaction::NodeInfo*
Transaction::_GetNodeInfo(Node* node) const
{
	for (NodeInfoList::ConstIterator it = fNodeInfos.GetIterator();
			NodeInfo* info = it.Next();) {
		if (node == info->node)
			return info;
	}

	return NULL;
}


void
Transaction::_DeleteNodeInfosAndUnlock(bool failed)
{
	while (NodeInfo* info = fNodeInfos.RemoveHead()) {
		if ((info->flags & TRANSACTION_DELETE_NODE) != 0)
			delete info->node;
		else if ((info->flags & TRANSACTION_KEEP_NODE_LOCKED) == 0)
			info->node->WriteUnlock();
		delete info;
	}
}



// #pragma mark - PostCommitNotification


PostCommitNotification::~PostCommitNotification()
{
}
