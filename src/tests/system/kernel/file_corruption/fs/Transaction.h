/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TRANSACTION_H
#define TRANSACTION_H


#include <fs_cache.h>

#include <util/DoublyLinkedList.h>

#include "Node.h"


class Volume;


enum {
	TRANSACTION_DELETE_NODE	= 0x1
};


class Transaction {
public:
	explicit					Transaction(Volume* volume);
								~Transaction();

			int32				ID() const	{ return fID; }

			status_t			Start();
			status_t			Commit();
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


#endif	// TRANSACTION_H
