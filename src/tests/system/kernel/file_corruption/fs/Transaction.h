/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TRANSACTION_H
#define TRANSACTION_H


#include <fs_cache.h>

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "CheckSum.h"
#include "driver/checksum_device.h"
#include "Node.h"


class PostCommitNotification;
class Volume;


enum {
	TRANSACTION_DELETE_NODE				= 0x01,
	TRANSACTION_NODE_ALREADY_LOCKED		= 0x02,
	TRANSACTION_KEEP_NODE_LOCKED		= 0x04,
	TRANSACTION_REMOVE_NODE_ON_ERROR	= 0x08,
	TRANSACTION_UNREMOVE_NODE_ON_ERROR	= 0x10
};


class Transaction {
public:
	explicit					Transaction(Volume* volume);
								~Transaction();

			int32				ID() const	{ return fID; }

			status_t			Start();
			status_t			StartAndAddNode(Node* node, uint32 flags = 0);
			status_t			Commit(
									const PostCommitNotification* notification1
										= NULL,
									const PostCommitNotification* notification2
										= NULL,
									const PostCommitNotification* notification3
										= NULL);
	inline	status_t			Commit(
									const PostCommitNotification& notification);
			void				Abort();

	inline	bool				IsActive() const	{ return fID >= 0; }

			status_t			AddNode(Node* node, uint32 flags = 0);
			status_t			AddNodes(Node* node1, Node* node2,
									Node* node3 = NULL);
			bool				RemoveNode(Node* node);
			void				UpdateNodeFlags(Node* node, uint32 flags);

			void				KeepNode(Node* node);

			bool				IsNodeLocked(Node* node) const
									{ return _GetNodeInfo(node) != NULL; }

			status_t			RegisterBlock(uint64 blockIndex);
			void				PutBlock(uint64 blockIndex, const void* data);

private:
			struct NodeInfo : DoublyLinkedListLinkImpl<NodeInfo> {
				Node*			node;
				checksumfs_node	oldNodeData;
				uint32			flags;
			};

			typedef DoublyLinkedList<NodeInfo> NodeInfoList;

			struct BlockInfo {
				checksum_device_ioctl_check_sum	indexAndCheckSum;
				BlockInfo*						hashNext;
				const void*						data;
				int32							refCount;
				bool							dirty;
			};

			struct BlockInfoHashDefinition {
				typedef uint64		KeyType;
				typedef	BlockInfo	ValueType;

				size_t HashKey(uint64 key) const
				{
					return (size_t)key;
				}

				size_t Hash(const BlockInfo* value) const
				{
					return HashKey(value->indexAndCheckSum.blockIndex);
				}

				bool Compare(uint64 key, const BlockInfo* value) const
				{
					return value->indexAndCheckSum.blockIndex == key;
				}

				BlockInfo*& GetLink(BlockInfo* value) const
				{
					return value->hashNext;
				}
			};

			typedef BOpenHashTable<BlockInfoHashDefinition> BlockInfoTable;

private:
			NodeInfo*			_GetNodeInfo(Node* node) const;
			void				_DeleteNodeInfosAndUnlock(bool failed);
			void				_DeleteNodeInfoAndUnlock(NodeInfo* info,
									bool failed);

			status_t			_UpdateBlockCheckSums();
			status_t			_RevertBlockCheckSums();

private:
			Volume*				fVolume;
			SHA256*				fSHA256;
			checksum_device_ioctl_check_sum* fCheckSum;
			int32				fID;
			NodeInfoList		fNodeInfos;
			BlockInfoTable		fBlockInfos;
			uint64				fOldFreeBlockCount;
};


status_t
Transaction::Commit(const PostCommitNotification& notification)
{
	return Commit(&notification);
}


// #pragma mark -


class PostCommitNotification {
public:
	virtual						~PostCommitNotification();

	virtual	void				NotifyPostCommit() const = 0;
};


#endif	// TRANSACTION_H
