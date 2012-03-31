/*
 * Copyright 2001-2012, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H


#include "bfs.h"
#include "Journal.h"


//	#pragma mark - on-disk structures

struct bplustree_node;

#define BPLUSTREE_NULL			-1LL
#define BPLUSTREE_FREE			-2LL

struct bplustree_header {
	uint32		magic;
	uint32		node_size;
	uint32		max_number_of_levels;
	uint32		data_type;
	int64		root_node_pointer;
	int64		free_node_pointer;
	int64		maximum_size;

	uint32 Magic() const { return BFS_ENDIAN_TO_HOST_INT32(magic); }
	uint32 NodeSize() const { return BFS_ENDIAN_TO_HOST_INT32(node_size); }
	uint32 DataType() const { return BFS_ENDIAN_TO_HOST_INT32(data_type); }
	off_t RootNode() const
		{ return BFS_ENDIAN_TO_HOST_INT64(root_node_pointer); }
	off_t FreeNode() const
		{ return BFS_ENDIAN_TO_HOST_INT64(free_node_pointer); }
	off_t MaximumSize() const { return BFS_ENDIAN_TO_HOST_INT64(maximum_size); }
	uint32 MaxNumberOfLevels() const
		{ return BFS_ENDIAN_TO_HOST_INT32(max_number_of_levels); }

	inline bool CheckNode(bplustree_node* node) const;
	inline bool IsValidLink(off_t link) const;
	bool IsValid() const;
} _PACKED;

#define BPLUSTREE_MAGIC 			0x69f6c2e8
#define BPLUSTREE_NODE_SIZE 		1024
#define BPLUSTREE_MAX_KEY_LENGTH	256
#define BPLUSTREE_MIN_KEY_LENGTH	1

enum bplustree_types {
	BPLUSTREE_STRING_TYPE	= 0,
	BPLUSTREE_INT32_TYPE	= 1,
	BPLUSTREE_UINT32_TYPE	= 2,
	BPLUSTREE_INT64_TYPE	= 3,
	BPLUSTREE_UINT64_TYPE	= 4,
	BPLUSTREE_FLOAT_TYPE	= 5,
	BPLUSTREE_DOUBLE_TYPE	= 6
};


struct duplicate_array;


struct bplustree_node {
			int64				left_link;
			int64				right_link;
			int64				overflow_link;
			uint16				all_key_count;
			uint16				all_key_length;

			off_t				LeftLink() const
									{ return BFS_ENDIAN_TO_HOST_INT64(
										left_link); }
			off_t				RightLink() const
									{ return BFS_ENDIAN_TO_HOST_INT64(
										right_link); }
			off_t				OverflowLink() const
									{ return BFS_ENDIAN_TO_HOST_INT64(
										overflow_link); }
			uint16				NumKeys() const
									{ return BFS_ENDIAN_TO_HOST_INT16(
										all_key_count); }
			uint16				AllKeyLength() const
									{ return BFS_ENDIAN_TO_HOST_INT16(
										all_key_length); }

	inline	uint16*				KeyLengths() const;
	inline	off_t*				Values() const;
	inline	uint8*				Keys() const;
	inline	int32				Used() const;
			uint8*				KeyAt(int32 index, uint16* keyLength) const;

	inline	bool				IsLeaf() const;

			void				Initialize();
			uint8				CountDuplicates(off_t offset,
									bool isFragment) const;
			off_t				DuplicateAt(off_t offset, bool isFragment,
									int8 index) const;
			uint32				FragmentsUsed(uint32 nodeSize) const;
	inline	duplicate_array*	FragmentAt(int8 index) const;
	inline	duplicate_array*	DuplicateArray() const;

	static inline uint8			LinkType(off_t link);
	static inline off_t			MakeLink(uint8 type, off_t link,
									uint32 fragmentIndex = 0);
	static inline bool			IsDuplicate(off_t link);
	static inline off_t			FragmentOffset(off_t link);
	static inline uint32		FragmentIndex(off_t link);
	static inline uint32		MaxFragments(uint32 nodeSize);

			status_t			CheckIntegrity(uint32 nodeSize) const;
} _PACKED;

//#define BPLUSTREE_NODE 0
#define BPLUSTREE_DUPLICATE_NODE 2
#define BPLUSTREE_DUPLICATE_FRAGMENT 3

#define NUM_FRAGMENT_VALUES 7
#define NUM_DUPLICATE_VALUES 125

//**************************************

enum bplustree_traversing {
	BPLUSTREE_FORWARD = 1,
	BPLUSTREE_BACKWARD = -1,

	BPLUSTREE_BEGIN = 0,
	BPLUSTREE_END = 1
};


//	#pragma mark - in-memory structures

template<class T> class Stack;
class BPlusTree;
class TreeIterator;
class CachedNode;
class Inode;
struct TreeCheck;

// needed for searching (utilizing a stack)
struct node_and_key {
	off_t	nodeOffset;
	uint16	keyIndex;
};


class CachedNode {
public:
	CachedNode(BPlusTree* tree)
		:
		fTree(tree),
		fNode(NULL)
	{
	}

	CachedNode(BPlusTree* tree, off_t offset, bool check = true)
		:
		fTree(tree),
		fNode(NULL)
	{
		SetTo(offset, check);
	}

	~CachedNode()
	{
		Unset();
	}

			const bplustree_node* SetTo(off_t offset, bool check = true);
			bplustree_node*		SetToWritable(Transaction& transaction,
									off_t offset, bool check = true);
			bplustree_node*		MakeWritable(Transaction& transaction);
			const bplustree_header* SetToHeader();
			bplustree_header*	SetToWritableHeader(Transaction& transaction);

			void				UnsetUnchanged(Transaction& transaction);
			void				Unset();

			status_t			Free(Transaction& transaction, off_t offset);
			status_t			Allocate(Transaction& transaction,
									bplustree_node** _node, off_t* _offset);

			bool				IsWritable() const { return fWritable; }
			bplustree_node*		Node() const { return fNode; }

protected:
			bplustree_node*		InternalSetTo(Transaction* transaction,
									off_t offset);

			BPlusTree*			fTree;
			bplustree_node*		fNode;
			off_t				fOffset;
			off_t				fBlockNumber;
			bool				fWritable;
};


class BPlusTree : public TransactionListener {
public:
								BPlusTree(Transaction& transaction,
									Inode* stream,
									int32 nodeSize = BPLUSTREE_NODE_SIZE);
								BPlusTree(Inode* stream);
								BPlusTree();
								~BPlusTree();

			status_t			SetTo(Transaction& transaction, Inode* stream,
									int32 nodeSize = BPLUSTREE_NODE_SIZE);
			status_t			SetTo(Inode* stream);
			status_t			SetStream(Inode* stream);

			status_t			InitCheck();

			size_t				NodeSize() const { return fNodeSize; }
			Inode*				Stream() const { return fStream; }

			status_t			Validate(bool repair, bool& _errorsFound);
			status_t			MakeEmpty();

			status_t			Remove(Transaction& transaction,
									const uint8* key, uint16 keyLength,
									off_t value);
			status_t			Insert(Transaction& transaction,
									const uint8* key, uint16 keyLength,
									off_t value);

			status_t			Remove(Transaction& transaction, const char* key,
									off_t value);
			status_t			Insert(Transaction& transaction, const char* key,
									off_t value);
			status_t			Insert(Transaction& transaction, int32 key,
									off_t value);
			status_t			Insert(Transaction& transaction, uint32 key,
									off_t value);
			status_t			Insert(Transaction& transaction, int64 key,
									off_t value);
			status_t			Insert(Transaction& transaction, uint64 key,
									off_t value);
			status_t			Insert(Transaction& transaction, float key,
									off_t value);
			status_t			Insert(Transaction& transaction, double key,
									off_t value);

			status_t			Replace(Transaction& transaction,
									const uint8* key, uint16 keyLength,
									off_t value);
			status_t			Find(const uint8* key, uint16 keyLength,
									off_t* value);

	static	int32				TypeCodeToKeyType(type_code code);
	static	int32				ModeToKeyType(mode_t mode);

protected:
	virtual void				TransactionDone(bool success);
	virtual void				RemovedFromTransaction();

private:
								BPlusTree(const BPlusTree& other);
								BPlusTree& operator=(const BPlusTree& other);
									// no implementation

			int32				_CompareKeys(const void* key1, int keylength1,
									const void* key2, int keylength2);
			status_t			_FindKey(const bplustree_node* node,
									const uint8* key, uint16 keyLength,
									uint16* index = NULL, off_t* next = NULL);
			status_t			_SeekDown(Stack<node_and_key>& stack,
									const uint8* key, uint16 keyLength);

			status_t			_FindFreeDuplicateFragment(
									Transaction& transaction,
									const bplustree_node* node,
									CachedNode& cached, off_t* _offset,
									bplustree_node** _fragment, uint32* _index);
			status_t			_InsertDuplicate(Transaction& transaction,
									CachedNode& cached,
									const bplustree_node* node, uint16 index,
									off_t value);
			void				_InsertKey(bplustree_node* node, uint16 index,
									uint8* key, uint16 keyLength, off_t value);
			status_t			_SplitNode(bplustree_node* node,
									off_t nodeOffset, bplustree_node* other,
									off_t otherOffset, uint16* _keyIndex,
									uint8* key, uint16* _keyLength,
									off_t* _value);

			status_t			_RemoveDuplicate(Transaction& transaction,
									const bplustree_node* node,
									CachedNode& cached, uint16 keyIndex,
									off_t value);
			void				_RemoveKey(bplustree_node* node, uint16 index);

			void				_UpdateIterators(off_t offset, off_t nextOffset,
									uint16 keyIndex, uint16 splitAt,
									int8 change);
			void				_AddIterator(TreeIterator* iterator);
			void				_RemoveIterator(TreeIterator* iterator);

			status_t			_ValidateChildren(TreeCheck& check,
									uint32 level, off_t offset,
									const uint8* largestKey, uint16 keyLength,
									const bplustree_node* parent);
			status_t			_ValidateChild(TreeCheck& check,
									CachedNode& cached, uint32 level,
									off_t offset, off_t lastOffset,
									off_t nextOffset, const uint8* key,
									uint16 keyLength);

private:
			friend class TreeIterator;
			friend class CachedNode;
			friend class TreeCheck;

			Inode*				fStream;
			bplustree_header	fHeader;
			int32				fNodeSize;
			bool				fAllowDuplicates;
			bool				fInTransaction;
			status_t			fStatus;
			mutex				fIteratorLock;
			SinglyLinkedList<TreeIterator> fIterators;
};


//	#pragma mark - helper classes/functions


extern int32 compareKeys(type_code type, const void* key1, int keyLength1,
	const void* key2, int keyLength2);


class TreeIterator : public SinglyLinkedListLinkImpl<TreeIterator> {
public:
								TreeIterator(BPlusTree* tree);
								~TreeIterator();

			status_t			Goto(int8 to);
			status_t			Traverse(int8 direction, void* key,
									uint16* keyLength, uint16 maxLength,
									off_t* value, uint16* duplicate = NULL);
			status_t			Find(const uint8* key, uint16 keyLength);

			status_t			Rewind();
			status_t			GetNextEntry(void* key, uint16* keyLength,
									uint16 maxLength, off_t* value,
									uint16* duplicate = NULL);
			status_t			GetPreviousEntry(void* key, uint16* keyLength,
									uint16 maxLength, off_t* value,
									uint16* duplicate = NULL);
			void				SkipDuplicates();

			BPlusTree*			Tree() const { return fTree; }

#ifdef DEBUG
			void				Dump();
#endif

private:
			friend class BPlusTree;

			// called by BPlusTree
			void				Update(off_t offset, off_t nextOffset,
									uint16 keyIndex, uint16 splitAt,
									int8 change);
			void				Stop();

private:
			BPlusTree*			fTree;
			off_t				fCurrentNodeOffset;
									// traverse position
			int32				fCurrentKey;
			off_t				fDuplicateNode;
			uint16				fDuplicate;
			uint16				fNumDuplicates;
			bool				fIsFragment;
};


//	#pragma mark - BPlusTree's inline functions
//	(most of them may not be needed)


inline status_t
BPlusTree::Remove(Transaction& transaction, const char* key, off_t value)
{
	if (fHeader.DataType() != BPLUSTREE_STRING_TYPE)
		return B_BAD_TYPE;
	return Remove(transaction, (uint8*)key, strlen(key), value);
}


inline status_t
BPlusTree::Insert(Transaction& transaction, const char* key, off_t value)
{
	if (fHeader.DataType() != BPLUSTREE_STRING_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8*)key, strlen(key), value);
}


inline status_t
BPlusTree::Insert(Transaction& transaction, int32 key, off_t value)
{
	if (fHeader.DataType() != BPLUSTREE_INT32_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8*)&key, sizeof(key), value);
}


inline status_t
BPlusTree::Insert(Transaction& transaction, uint32 key, off_t value)
{
	if (fHeader.DataType() != BPLUSTREE_UINT32_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8*)&key, sizeof(key), value);
}


inline status_t
BPlusTree::Insert(Transaction& transaction, int64 key, off_t value)
{
	if (fHeader.DataType() != BPLUSTREE_INT64_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8*)&key, sizeof(key), value);
}


inline status_t
BPlusTree::Insert(Transaction& transaction, uint64 key, off_t value)
{
	if (fHeader.DataType() != BPLUSTREE_UINT64_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8*)&key, sizeof(key), value);
}


inline status_t
BPlusTree::Insert(Transaction& transaction, float key, off_t value)
{
	if (fHeader.DataType() != BPLUSTREE_FLOAT_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8*)&key, sizeof(key), value);
}


inline status_t
BPlusTree::Insert(Transaction& transaction, double key, off_t value)
{
	if (fHeader.DataType() != BPLUSTREE_DOUBLE_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8*)&key, sizeof(key), value);
}


//	#pragma mark - TreeIterator inline functions


inline status_t
TreeIterator::Rewind()
{
	return Goto(BPLUSTREE_BEGIN);
}


inline status_t
TreeIterator::GetNextEntry(void* key, uint16* keyLength, uint16 maxLength,
	off_t* value, uint16* duplicate)
{
	return Traverse(BPLUSTREE_FORWARD, key, keyLength, maxLength, value,
		duplicate);
}


inline status_t
TreeIterator::GetPreviousEntry(void* key, uint16* keyLength, uint16 maxLength,
	off_t* value, uint16* duplicate)
{
	return Traverse(BPLUSTREE_BACKWARD, key, keyLength, maxLength, value,
		duplicate);
}


//	#pragma mark - bplustree_header inline functions


inline bool
bplustree_header::CheckNode(bplustree_node* node) const
{
	// sanity checks (links, all_key_count)
	return IsValidLink(node->LeftLink())
		&& IsValidLink(node->RightLink())
		&& IsValidLink(node->OverflowLink())
		&& (int8*)node->Values() + node->NumKeys() * sizeof(off_t)
				<= (int8*)node + NodeSize();
}


inline bool
bplustree_header::IsValidLink(off_t link) const
{
	return link == BPLUSTREE_NULL
		|| (link > 0 && link <= MaximumSize() - NodeSize());
}


//	#pragma mark - bplustree_node inline functions


inline uint16*
bplustree_node::KeyLengths() const
{
	return (uint16*)(((char*)this) + key_align(sizeof(bplustree_node)
		+ AllKeyLength()));
}


inline off_t*
bplustree_node::Values() const
{
	return (off_t*)((char*)KeyLengths() + NumKeys() * sizeof(uint16));
}


inline uint8*
bplustree_node::Keys() const
{
	return (uint8*)this + sizeof(bplustree_node);
}


inline int32
bplustree_node::Used() const
{
	return key_align(sizeof(bplustree_node) + AllKeyLength()) + NumKeys()
		* (sizeof(uint16) + sizeof(off_t));
}


inline bool
bplustree_node::IsLeaf() const
{
	return OverflowLink() == BPLUSTREE_NULL;
}


inline duplicate_array*
bplustree_node::FragmentAt(int8 index) const
{
	return (duplicate_array*)((off_t*)this + index * (NUM_FRAGMENT_VALUES + 1));
}


inline duplicate_array*
bplustree_node::DuplicateArray() const
{
	return (duplicate_array*)&overflow_link;
}


inline uint8
bplustree_node::LinkType(off_t link)
{
	return *(uint64*)&link >> 62;
}


inline off_t
bplustree_node::MakeLink(uint8 type, off_t link, uint32 fragmentIndex)
{
	return ((off_t)type << 62) | (link & 0x3ffffffffffffc00LL)
		| (fragmentIndex & 0x3ff);
}


inline bool
bplustree_node::IsDuplicate(off_t link)
{
	return (LinkType(link)
		& (BPLUSTREE_DUPLICATE_NODE | BPLUSTREE_DUPLICATE_FRAGMENT)) > 0;
}


inline off_t
bplustree_node::FragmentOffset(off_t link)
{
	return link & 0x3ffffffffffffc00LL;
}


inline uint32
bplustree_node::FragmentIndex(off_t link)
{
	return (uint32)(link & 0x3ff);
}


inline uint32
bplustree_node::MaxFragments(uint32 nodeSize)
{
	return nodeSize / ((NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));
}


#endif	// B_PLUS_TREE_H
