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


class Transaction;


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
	class Node;

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
			status_t			MakeEntries(Transaction& transaction,
									Path* path, const btrfs_key& startKey,
									int num, int length);
			status_t			InsertEntries(Transaction& transaction,
									Path* path, btrfs_entry* entries,
									void** data, int num);

			Volume*				SystemVolume() const { return fVolume; }
			status_t			SetRoot(off_t logical, fsblock_t* block);
			void				SetRoot(Node* root);
			fsblock_t			RootBlock() const { return fRootBlock; }
			off_t				LogicalRoot() const { return fLogicalRoot; }
			uint8				RootLevel() const { return fRootLevel; }

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
			uint8				fRootLevel;
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
		void	SetLogicalAddress(uint64 address)
			{ fNode->header.SetLogicalAddress(address); }
		void	SetGeneration(uint64 generation)
			{ fNode->header.SetGeneration(generation); }
		void	SetItemCount(uint32 itemCount)
			{ fNode->header.SetItemCount(itemCount); }

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
		int		SpaceUsed() const;
		int		SpaceLeft() const;

		off_t	BlockNum() const { return fBlockNumber;}
		bool	IsWritable() const { return fWritable; }
		status_t	Copy(const Node* origin, uint32 start, uint32 end,
						int length) const;
		status_t	MoveEntries(uint32 start, uint32 end, int length) const;

		status_t	SearchSlot(const btrfs_key& key, int* slot,
						btree_traversing type) const;
	private:
		Node(const Node&);
		Node& operator=(const Node&);
			//no implementation

		void	_Copy(const Node* origin, uint32 at, uint32 from, uint32 to,
					int length) const;
		status_t	_SpaceCheck(int length) const;
		int		_CalculateSpace(uint32 from, uint32 to, uint8 type = 1) const;

		btrfs_stream*		fNode;
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
		status_t	SetEntry(int slot, const btrfs_entry& entry, void* value);

		int			Move(int level, int step);

		status_t	CopyOnWrite(Transaction& transaction, int level,
						uint32 start, int num, int length);
		status_t	InternalCopy(Transaction& transaction, int level);

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
								TreeIterator(BTree* tree, const btrfs_key& key);
								~TreeIterator();

			void				Rewind(bool inverse = false);
			status_t			Find(const btrfs_key& key);
			status_t			GetNextEntry(void** _value,
									uint32* _size = NULL,
									uint32* _offset = NULL);
			status_t			GetPreviousEntry(void** _value,
									uint32* _size = NULL,
									uint32* _offset = NULL);

			BTree*				Tree() const { return fTree; }
			btrfs_key			Key() const { return fKey; }

private:
			friend class BTree;

			status_t			_Traverse(btree_traversing direction);
			status_t			_Find(btree_traversing type, btrfs_key& key,
									void** _value);
			status_t			_GetEntry(btree_traversing type, void** _value,
									uint32* _size, uint32* _offset);
			// called by BTree
			void				Stop();

private:
			BTree*			fTree;
			BTree::Path*	fPath;
			btrfs_key		fKey;
			status_t		fIteratorStatus;
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
TreeIterator::GetNextEntry(void** _value, uint32* _size, uint32* _offset)
{
	return _GetEntry(BTREE_FORWARD, _value, _size, _offset);
}


inline status_t
TreeIterator::GetPreviousEntry(void** _value, uint32* _size, uint32* _offset)
{
	return _GetEntry(BTREE_BACKWARD, _value, _size, _offset);
}


#endif	// B_TREE_H
