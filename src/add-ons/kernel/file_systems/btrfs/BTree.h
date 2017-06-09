/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H


#include "btrfs.h"
#include "Volume.h"


#define BTREE_NULL			-1LL
#define BTREE_FREE			-2LL


enum btree_traversing {
	BTREE_FORWARD = 1,
	BTREE_EXACT = 0,
	BTREE_BACKWARD = -1,

	BTREE_BEGIN = 0,
	BTREE_END = -1
};


//	#pragma mark - in-memory structures


template<class T> class Stack;
class TreeIterator;


// needed for searching (utilizing a stack)
struct node_and_key {
	off_t	nodeOffset;
	uint16	keyIndex;
};


class BTree {
public:
								BTree(Volume* volume,
									btrfs_stream* stream);
								BTree(Volume* volume,
									fsblock_t rootBlock);
								~BTree();
			status_t			FindExact(btrfs_key& key, void** value,
									size_t* size = NULL);
			status_t			FindNext(btrfs_key& key, void** value,
									size_t* size = NULL);
			status_t			FindPrevious(btrfs_key& key, void** value,
									size_t* size = NULL);

private:
								BTree(const BTree& other);
								BTree& operator=(const BTree& other);
									// no implementation

			int32				_CompareKeys(btrfs_key& key1,
									btrfs_key& key2);
			status_t			_Find(btrfs_key& key, void** value,
									size_t* size, btree_traversing type);
			void				_AddIterator(TreeIterator* iterator);
			void				_RemoveIterator(TreeIterator* iterator);
private:
			friend class TreeIterator;

			btrfs_stream* fStream;
			fsblock_t			fRootBlock;
			Volume*				fVolume;
			mutex				fIteratorLock;
			SinglyLinkedList<TreeIterator> fIterators;
};


class TreeIterator : public SinglyLinkedListLinkImpl<TreeIterator> {
public:
								TreeIterator(BTree* tree, btrfs_key& key);
								~TreeIterator();

			status_t			Traverse(btree_traversing direction,
									btrfs_key& key, void** value,
									size_t* size = NULL);
			status_t			Find(btrfs_key& key);

			status_t			Rewind();
			status_t			GetNextEntry(btrfs_key& key, void** value,
									size_t* size = NULL);
			status_t			GetPreviousEntry(btrfs_key& key, void** value,
									size_t* size = NULL);

			BTree*			Tree() const { return fTree; }

private:
			friend class BTree;

			// called by BTree
			void				Stop();

private:
			BTree*			fTree;
			btrfs_key	fCurrentKey;
};


//	#pragma mark - TreeIterator inline functions


inline status_t
TreeIterator::Rewind()
{
	fCurrentKey.SetOffset(BTREE_BEGIN);
	return B_OK;
}


inline status_t
TreeIterator::GetNextEntry(btrfs_key& key, void** value, size_t* size)
{
	return Traverse(BTREE_FORWARD, key, value, size);
}


inline status_t
TreeIterator::GetPreviousEntry(btrfs_key& key, void** value,
	size_t* size)
{
	return Traverse(BTREE_BACKWARD, key, value, size);
}


#endif	// B_PLUS_TREE_H
