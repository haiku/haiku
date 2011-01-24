/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H


#include "btrfs.h"

#include <util/SinglyLinkedList.h>

#include "Volume.h"


#define BPLUSTREE_NULL			-1LL
#define BPLUSTREE_FREE			-2LL


enum bplustree_traversing {
	BPLUSTREE_FORWARD = 1,
	BPLUSTREE_EXACT = 0,
	BPLUSTREE_BACKWARD = -1,

	BPLUSTREE_BEGIN = 0,
	BPLUSTREE_END = -1
};


//	#pragma mark - in-memory structures

template<class T> class Stack;
class TreeIterator;


// needed for searching (utilizing a stack)
struct node_and_key {
	off_t	nodeOffset;
	uint16	keyIndex;
};


class BPlusTree {
public:
								BPlusTree(Volume* volume,
									struct btrfs_stream *stream);
								BPlusTree(Volume* volume,
									fsblock_t rootBlock);
								~BPlusTree();
			status_t			FindExact(struct btrfs_key &key, void** value,
									size_t* size = NULL);
			status_t			FindNext(struct btrfs_key &key, void** value,
									size_t* size = NULL);
			status_t			FindPrevious(struct btrfs_key &key, void** value,
									size_t* size = NULL);

private:
								BPlusTree(const BPlusTree& other);
								BPlusTree& operator=(const BPlusTree& other);
									// no implementation

			int32				_CompareKeys(struct btrfs_key &key1,
									struct btrfs_key &key2);
			status_t			_Find(struct btrfs_key &key, void** value,
									size_t* size, bplustree_traversing type);
			void				_AddIterator(TreeIterator* iterator);
			void				_RemoveIterator(TreeIterator* iterator);
private:
			friend class TreeIterator;

			struct btrfs_stream* fStream;
			fsblock_t			fRootBlock;
			Volume*				fVolume;
			mutex				fIteratorLock;
			SinglyLinkedList<TreeIterator> fIterators;
};


class TreeIterator : public SinglyLinkedListLinkImpl<TreeIterator> {
public:
								TreeIterator(BPlusTree* tree, struct btrfs_key &key);
								~TreeIterator();

			status_t			Traverse(bplustree_traversing direction,
									struct btrfs_key &key, void** value,
									size_t *size = NULL);
			status_t			Find(struct btrfs_key &key);

			status_t			Rewind();
			status_t			GetNextEntry(struct btrfs_key &key, void** value,
									size_t *size = NULL);
			status_t			GetPreviousEntry(struct btrfs_key &key, void** value,
									size_t *size = NULL);

			BPlusTree*			Tree() const { return fTree; }

private:
			friend class BPlusTree;

			// called by BPlusTree
			void				Stop();

private:
			BPlusTree*			fTree;
			struct btrfs_key	fCurrentKey;
};


//	#pragma mark - TreeIterator inline functions


inline status_t
TreeIterator::Rewind()
{
	fCurrentKey.SetOffset(BPLUSTREE_BEGIN);
	return B_OK;
}

inline status_t
TreeIterator::GetNextEntry(struct btrfs_key &key, void** value, size_t *size)
{
	return Traverse(BPLUSTREE_FORWARD, key, value, size);
}

inline status_t
TreeIterator::GetPreviousEntry(struct btrfs_key &key, void** value,
	size_t *size)
{
	return Traverse(BPLUSTREE_BACKWARD, key, value, size);
}



#endif	// B_PLUS_TREE_H
