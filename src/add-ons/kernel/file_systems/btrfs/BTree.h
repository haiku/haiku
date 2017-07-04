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

#define BTRFS_MAX_TREE_DEPTH		8


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
								BTree(Volume* volume);
								BTree(Volume* volume,
									btrfs_stream* stream);
								BTree(Volume* volume,
									fsblock_t rootBlock);
								~BTree();
			status_t			FindExact(btrfs_key& key, void** value,
									size_t* size = NULL, bool read = true);
			status_t			FindNext(btrfs_key& key, void** value,
									size_t* size = NULL, bool read = true);
			status_t			FindPrevious(btrfs_key& key, void** value,
									size_t* size = NULL, bool read = true);

			Volume*				SystemVolume() const { return fVolume; }

			status_t			SetRoot(off_t logical, fsblock_t* block);
			fsblock_t			RootBlock() const { return fRootBlock; }
			off_t				LogicalRoot() const { return fLogicalRoot; }

private:
								BTree(const BTree& other);
								BTree& operator=(const BTree& other);
									// no implementation

			status_t			_Find(btrfs_key& key, void** value, size_t* size,
									bool read, btree_traversing type);
			void				_AddIterator(TreeIterator* iterator);
			void				_RemoveIterator(TreeIterator* iterator);
private:
			friend class TreeIterator;

			fsblock_t			fRootBlock;
			off_t				fLogicalRoot;
			Volume*				fVolume;
			mutex				fIteratorLock;
			SinglyLinkedList<TreeIterator> fIterators;

public:
	class Node {
	public:
		Node(void* cache);
		Node(void* cache, off_t block);
		~Node();

			// just return from Header
		uint64	LogicalAddress() const
			{ return fNode->header.LogicalAddress(); }
		uint64	Flags() const
			{ return fNode->header.Flags(); }
		uint64	Generation() const
			{ return fNode->header.Generation(); }
		uint64	Owner() const
			{ return fNode->header.Owner(); }
		uint32	ItemCount() const
			{ return fNode->header.ItemCount(); }
		uint8	Level() const
			{ return fNode->header.Level(); }

		btrfs_index*	Index(uint32 i) const
			{ return &fNode->index[i]; }

		btrfs_entry*	Item(uint32 i) const
			{ return &fNode->entries[i]; }
		uint8*	ItemData(uint32 i) const
			{ return (uint8*)Item(0) + Item(i)->Offset(); }

		void	Keep();
		void	Unset();

		void	SetTo(off_t block);
		void	SetToWritable(off_t block,
				int32 transactionId, bool empty);

		off_t	BlockNum() const { return fBlockNumber;}
		bool	IsWritable() const { return fWritable; }

		int32	SearchSlot(const btrfs_key& key, int* slot,
					btree_traversing type) const;
		private:
		Node(const Node&);
		Node& operator=(const Node&);
			//no implementation

		btrfs_stream* 		fNode;
		void* 				fCache;
		off_t				fBlockNumber;
		uint32 				fCurrentSlot;
		bool				fWritable;
	};


	class Path {
	public:
		Path();
	private:
		Path(const Path&);
		Path operator=(const Path&);
		Node*	nodes[BTRFS_MAX_TREE_DEPTH];
	};

};	// class BTree


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
