/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Transaction.h"

#include <errno.h>

#include <algorithm>

#include <AutoDeleter.h>

#include "BlockAllocator.h"
#include "DebugSupport.h"
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
	fSHA256(NULL),
	fCheckSum(NULL),
	fID(-1)
{
}


Transaction::~Transaction()
{
	Abort();

	delete fCheckSum;
	delete fSHA256;
}


status_t
Transaction::Start()
{
	ASSERT(fID < 0);

	status_t error = fBlockInfos.Init();
	if (error != B_OK)
		return error;

	if (fSHA256 == NULL) {
		fSHA256 = new(std::nothrow) SHA256;
		if (fSHA256 == NULL)
			return B_NO_MEMORY;
	}

	if (fCheckSum == NULL) {
		fCheckSum = new(std::nothrow) checksum_device_ioctl_check_sum;
		if (fCheckSum == NULL)
			return B_NO_MEMORY;
	}

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
Transaction::StartAndAddNode(Node* node, uint32 flags)
{
	status_t error = Start();
	if (error != B_OK)
		return error;

	return AddNode(node, flags);
}


status_t
Transaction::Commit(const PostCommitNotification* notification1,
	const PostCommitNotification* notification2,
	const PostCommitNotification* notification3)
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

	// Make sure the previous transaction is on disk. This is not particularly
	// performance friendly, but prevents race conditions between us setting
	// the new block check sums and the block writer deciding to write back the
	// old block data.
	status_t error = block_cache_sync(fVolume->BlockCache());
	if (error != B_OK) {
		Abort();
		return error;
	}

	// compute the new block check sums
	error = _UpdateBlockCheckSums();
	if (error != B_OK) {
		Abort();
		return error;
	}

	// commit the cache transaction
	error = cache_end_transaction(fVolume->BlockCache(), fID, NULL, NULL);
	if (error != B_OK) {
		Abort();
		return error;
	}

	// send notifications
	if (notification1 != NULL)
		notification1->NotifyPostCommit();
	if (notification2 != NULL)
		notification2->NotifyPostCommit();
	if (notification3 != NULL)
		notification3->NotifyPostCommit();

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

	// revert the block check sums
	_RevertBlockCheckSums();

	// clean up

	// delete the node infos
	_DeleteNodeInfosAndUnlock(true);

	// delete the block infos
	BlockInfo* blockInfo = fBlockInfos.Clear(true);
	while (blockInfo != NULL) {
		BlockInfo* nextInfo = blockInfo->hashNext;
		block_cache_put(fVolume->BlockCache(),
			blockInfo->indexAndCheckSum.blockIndex);
		delete nextInfo;
		blockInfo = nextInfo;
	}

	fVolume->GetBlockAllocator()->ResetFreeBlocks(fOldFreeBlockCount);

	fVolume->TransactionFinished();
	fID = -1;
}


status_t
Transaction::AddNode(Node* node, uint32 flags)
{
	ASSERT(fID >= 0);

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
	ASSERT(fID >= 0);

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


bool
Transaction::RemoveNode(Node* node)
{
	ASSERT(fID >= 0);

	NodeInfo* info = _GetNodeInfo(node);
	if (info == NULL)
		return false;

	fNodeInfos.Remove(info);

	_DeleteNodeInfoAndUnlock(info, false);

	return true;
}


void
Transaction::UpdateNodeFlags(Node* node, uint32 flags)
{
	ASSERT(fID >= 0);

	NodeInfo* info = _GetNodeInfo(node);
	if (info == NULL)
		return;

	info->flags = flags;
}


void
Transaction::KeepNode(Node* node)
{
	ASSERT(fID >= 0);

	NodeInfo* info = _GetNodeInfo(node);
	if (info == NULL)
		return;

	info->flags &= ~(uint32)TRANSACTION_DELETE_NODE;
}


status_t
Transaction::RegisterBlock(uint64 blockIndex)
{
	ASSERT(fID >= 0);

	// look it up -- maybe it's already registered
	BlockInfo* info = fBlockInfos.Lookup(blockIndex);
	if (info != NULL) {
		info->refCount++;
		return B_OK;
	}

	// nope, create a new one
	info = new(std::nothrow) BlockInfo;
	if (info == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<BlockInfo> infoDeleter(info);

	info->indexAndCheckSum.blockIndex = blockIndex;
	info->refCount = 1;
	info->dirty = false;

	// get the old check sum
	if (ioctl(fVolume->FD(), CHECKSUM_DEVICE_IOCTL_GET_CHECK_SUM,
			&info->indexAndCheckSum, sizeof(info->indexAndCheckSum)) < 0) {
		RETURN_ERROR(errno);
	}

	// get the data (we're fine with read-only)
	info->data = block_cache_get(fVolume->BlockCache(), blockIndex);
	if (info->data == NULL) {
		delete info;
		RETURN_ERROR(B_ERROR);
	}

	fBlockInfos.Insert(infoDeleter.Detach());

	return B_OK;
}


void
Transaction::PutBlock(uint64 blockIndex, const void* data)
{
	ASSERT(fID >= 0);

	BlockInfo* info = fBlockInfos.Lookup(blockIndex);
	if (info == NULL) {
		panic("checksumfs: Transaction::PutBlock(): unknown block %" B_PRIu64,
			blockIndex);
		return;
	}

	if (info->refCount == 0) {
		panic("checksumfs: Unbalanced Transaction::PutBlock(): for block %"
			B_PRIu64, blockIndex);
		return;
	}

	info->dirty |= data != NULL;

	if (--info->refCount == 0 && !info->dirty) {
		// block wasn't got successfully -- remove the info
		fBlockInfos.Remove(info);
		block_cache_put(fVolume->BlockCache(),
			info->indexAndCheckSum.blockIndex);
		delete info;
	}
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
	while (NodeInfo* info = fNodeInfos.RemoveHead())
		_DeleteNodeInfoAndUnlock(info, failed);
}


void
Transaction::_DeleteNodeInfoAndUnlock(NodeInfo* info, bool failed)
{
	if (failed) {
		if ((info->flags & TRANSACTION_REMOVE_NODE_ON_ERROR) != 0)
			fVolume->RemoveNode(info->node);
		else if ((info->flags & TRANSACTION_UNREMOVE_NODE_ON_ERROR) != 0)
			fVolume->UnremoveNode(info->node);
	}

	if ((info->flags & TRANSACTION_DELETE_NODE) != 0)
		delete info->node;
	else if ((info->flags & TRANSACTION_KEEP_NODE_LOCKED) == 0)
		info->node->WriteUnlock();
	delete info;
}


status_t
Transaction::_UpdateBlockCheckSums()
{
	for (BlockInfoTable::Iterator it = fBlockInfos.GetIterator();
			BlockInfo* info = it.Next();) {
		if (info->refCount > 0) {
			panic("checksumfs: Transaction::Commit(): block %" B_PRIu64
				" still referenced", info->indexAndCheckSum.blockIndex);
		}

		if (!info->dirty)
			continue;

		// compute the check sum
		fSHA256->Init();
		fSHA256->Update(info->data, B_PAGE_SIZE);
		fCheckSum->blockIndex = info->indexAndCheckSum.blockIndex;
		fCheckSum->checkSum = fSHA256->Digest();

		// set it
		if (ioctl(fVolume->FD(), CHECKSUM_DEVICE_IOCTL_SET_CHECK_SUM, fCheckSum,
				sizeof(*fCheckSum)) < 0) {
			return errno;
		}
	}

	return B_OK;
}


status_t
Transaction::_RevertBlockCheckSums()
{
	for (BlockInfoTable::Iterator it = fBlockInfos.GetIterator();
			BlockInfo* info = it.Next();) {
		if (!info->dirty)
			continue;

		// set the old check sum
		if (ioctl(fVolume->FD(), CHECKSUM_DEVICE_IOCTL_SET_CHECK_SUM,
				&info->indexAndCheckSum, sizeof(info->indexAndCheckSum)) < 0) {
			return errno;
		}
	}

	return B_OK;
}


// #pragma mark - PostCommitNotification


PostCommitNotification::~PostCommitNotification()
{
}
