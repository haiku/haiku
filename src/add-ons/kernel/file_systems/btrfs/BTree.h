/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef B_TREE_H
#define B_TREE_H


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
	class Path;

public:
								BTree(Volume* volume);
								BTree(Volume* volume,
									btrfs_stream* stream);
								BTree(Volume* volume,
									fsblock_t rootBlock);
								~BTree();

			status_t			FindExact(Path* path, btrfs_key& key,
									void** _value, uint32* _size = NULL,
									uint32* _offset = NULL) const;
			status_t			FindNext(Path* path, btrfs_key& key,
									void** _value, uint32* _size = NULL,
									uint32* _offset = NULL) const;
			status_t			FindPrevious(Path* path, btrfs_key& key,
									void** _value, uint32* _size = NULL,
									uint32* _offset = NULL) const;

			status_t			Traverse(btree_traversing type, Path* path,
									const btrfs_key& key) const;

			status_t			PreviousLeaf(Path* path) const;
			status_t			NextLeaf(Path* path) const;

			Volume*				SystemVolume() const { return fVolume; }
			status_t			SetRoot(off_t logical, fsblock_t* block);
			fsblock_t			RootBlock() const { return fRootBlock; }
			off_t				LogicalRoot() const { return fLogicalRoot; }

private:
								BTree(const BTree& other);
								BTree& operator=(const BTree& other);
									// no implementation

			status_t			_Find(Path* path, btrfs_key& key,
									void** _value, uint32* _size,
									uint32* _offset, btree_traversing type)
									const;
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
		Node(Volume* volume);
		Node(Volume* volume, off_t block);
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
		void	SetToWritable(off_t block, int32 transactionId, bool empty);

		off_t	BlockNum() const { return fBlockNumber;}
		bool	IsWritable() const { return fWritable; }

		status_t	SearchSlot(const btrfs_key& key, int* slot,
						btree_traversing type) const;
		private:
		Node(const Node&);
		Node& operator=(const Node&);
			//no implementation

		btrfs_stream* 		fNode;
		Volume*				fVolume;
		off_t				fBlockNumber;
		bool				fWritable;
	};


	class Path {
	public:
		Path(BTree* tree);
		~Path();

		Node*		GetNode(int level, int* _slot = NULL) const;
		Node*		SetNode(off_t block, int slot);
		Node*		SetNode(const Node* node, int slot);
		status_t	GetCurrentEntry(btrfs_key* _key, void** _value,
						uint32* _size = NULL, uint32* _offset = NULL);
		status_t	GetEntry(int slot, btrfs_key* _key, void** _value,
						uint32* _size = NULL, uint32* _offset = NULL);

		int			Move(int level, int step);

		BTree*		Tree() const { return fTree; }
	private:
		Path(const Path&);
		Path operator=(const Path&);
	private:
		Node*	fNodes[BTRFS_MAX_TREE_DEPTH];
		int		fSlots[BTRFS_MAX_TREE_DEPTH];
		BTree*	fTree;
	};

};	// class BTree


class TreeIterator : public SinglyLinkedListLinkImpl<TreeIterator> {
public:
								TreeIterator(BTree* tree, btrfs_key& key);
								~TreeIterator();

			status_t			Traverse(btree_traversing direction,
									btrfs_key& key, void** value,
									uint32* size = NULL);
			status_t			Find(btrfs_key& key);

			status_t			Rewind();
			status_t			GetNextEntry(btrfs_key& key, void** value,
									uint32* size = NULL);
			status_t			GetPreviousEntry(btrfs_key& key, void** value,
									uint32* size = NULL);

			BTree*			Tree() const { return fTree; }

private:
			friend class BTree;

			// called by BTree
			void				Stop();

private:
			BTree*			fTree;
			btrfs_key	fCurrentKey;
};


//	#pragma mark - BTree::Path inline functions


inline status_t
BTree::Path::GetCurrentEntry(btrfs_key* _key, void** _value, uint32* _size,
	uint32* _offset)
{
	return GetEntry(fSlots[0], _key, _value, _size, _offset);
}


//	#pragma mark - TreeIterator inline functions


inline status_t
TreeIterator::Rewind()
{
	fCurrentKey.SetOffset(BTREE_BEGIN);
	return B_OK;
}


inline status_t
TreeIterator::GetNextEntry(btrfs_key& key, void** value, uint32* size)
{
	return Traverse(BTREE_FORWARD, key, value, size);
}


inline status_t
TreeIterator::GetPreviousEntry(btrfs_key& key, void** value,
	uint32* size)
{
	return Traverse(BTREE_BACKWARD, key, value, size);
}


#endif	// B_TREE_H
