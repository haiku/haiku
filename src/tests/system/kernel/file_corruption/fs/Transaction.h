/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TRANSACTION_H
#define TRANSACTION_H


#include <fs_cache.h>

#include <util/DoublyLinkedList.h>

#include "Node.h"


class PostCommitNotification;
class Volume;


enum {
	TRANSACTION_DELETE_NODE			= 0x1,
	TRANSACTION_NODE_ALREADY_LOCKED	= 0x2,
	TRANSACTION_KEEP_NODE_LOCKED	= 0x4
};


class Transaction {
public:
	explicit					Transaction(Volume* volume);
								~Transaction();

			int32				ID() const	{ return fID; }

			status_t			Start();
			status_t			StartAndAddNode(Node* node, uint32 flags = 0);
			status_t			Commit(
									const PostCommitNotification* notification
										= NULL);
	inline	status_t			Commit(
									const PostCommitNotification& notification);
			void				Abort();

			status_t			AddNode(Node* node, uint32 flags = 0);
			status_t			AddNodes(Node* node1, Node* node2,
									Node* node3 = NULL);

			void				KeepNode(Node* node);

private:
			struct NodeInfo : DoublyLinkedListLinkImpl<NodeInfo> {
				Node*			node;
				checksumfs_node	oldNodeData;
				uint32			flags;
			};

			typedef DoublyLinkedList<NodeInfo> NodeInfoList;

private:
			NodeInfo*			_GetNodeInfo(Node* node) const;
			void				_DeleteNodeInfosAndUnlock(bool failed);

private:
			Volume*				fVolume;
			int32				fID;
			NodeInfoList		fNodeInfos;
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
