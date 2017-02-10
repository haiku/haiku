#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H
/* BPlusTree - BFS B+Tree implementation
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** Roughly based on 'btlib' written by Marcus J. Ranum
** 
** Copyright (c) 2001-2004 pinc Software. All Rights Reserved.
** This file may be used under the terms of the MIT License.
*/


#include "bfs.h"
#include "Journal.h"
#include "Chain.h"

#include <string.h>


//****************** on-disk structures ********************

#define BPLUSTREE_NULL			-1LL
#define BPLUSTREE_FREE			-2LL

struct bplustree_header {
	uint32		magic;
	uint32		node_size;
	uint32		max_number_of_levels;
	uint32		data_type;
	off_t		root_node_pointer;
	off_t		free_node_pointer;
	off_t		maximum_size;

	uint32 Magic() const { return BFS_ENDIAN_TO_HOST_INT32(magic); }
	uint32 NodeSize() const { return BFS_ENDIAN_TO_HOST_INT32(node_size); }
	uint32 DataType() const { return BFS_ENDIAN_TO_HOST_INT32(data_type); }
	off_t RootNode() const { return BFS_ENDIAN_TO_HOST_INT64(root_node_pointer); }
	off_t FreeNode() const { return BFS_ENDIAN_TO_HOST_INT64(free_node_pointer); }
	off_t MaximumSize() const { return BFS_ENDIAN_TO_HOST_INT64(maximum_size); }
	uint32 MaxNumberOfLevels() const { return BFS_ENDIAN_TO_HOST_INT32(max_number_of_levels); }

	inline bool IsValidLink(off_t link);
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

struct sorted_array;
typedef sorted_array duplicate_array;

struct bplustree_node {
	off_t	left_link;
	off_t	right_link;
	off_t	overflow_link;
	uint16	all_key_count;
	uint16	all_key_length;

	off_t LeftLink() const { return BFS_ENDIAN_TO_HOST_INT64(left_link); }
	off_t RightLink() const { return BFS_ENDIAN_TO_HOST_INT64(right_link); }
	off_t OverflowLink() const { return BFS_ENDIAN_TO_HOST_INT64(overflow_link); }
	uint16 NumKeys() const { return BFS_ENDIAN_TO_HOST_INT16(all_key_count); }
	uint16 AllKeyLength() const { return BFS_ENDIAN_TO_HOST_INT16(all_key_length); }

	inline uint16 *KeyLengths() const;
	inline off_t *Values() const;
	inline uint8 *Keys() const;
	inline int32 Used() const;
	uint8 *KeyAt(int32 index, uint16 *keyLength) const;

	inline bool IsLeaf() const;

	void Initialize();
	uint8 CountDuplicates(off_t offset, bool isFragment) const;
	off_t DuplicateAt(off_t offset, bool isFragment, int8 index) const;
	int32 FragmentsUsed(uint32 nodeSize);
	inline duplicate_array *FragmentAt(int8 index);
	inline duplicate_array *DuplicateArray();

	static inline uint8 LinkType(off_t link);
	static inline off_t MakeLink(uint8 type, off_t link, uint32 fragmentIndex = 0);
	static inline bool IsDuplicate(off_t link);
	static inline off_t FragmentOffset(off_t link);
	static inline uint32 FragmentIndex(off_t link);

#ifdef DEBUG
	void CheckIntegrity(uint32 nodeSize);
#endif
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


//****************** in-memory structures ********************

template<class T> class Stack;
class BPlusTree;
class TreeIterator;
class CachedNode;
class Inode;

// needed for searching (utilizing a stack)
struct node_and_key {
	off_t	nodeOffset;
	uint16	keyIndex;
};


//***** Cache handling *****

class CachedNode {
	public:
		CachedNode(BPlusTree *tree)
			:
			fTree(tree),
			fNode(NULL),
			fBlock(NULL)
		{
		}

		CachedNode(BPlusTree *tree, off_t offset, bool check = true)
			:
			fTree(tree),
			fNode(NULL),
			fBlock(NULL)
		{
			SetTo(offset, check);
		}

		~CachedNode()
		{
			Unset();
		}

		bplustree_node *SetTo(off_t offset, bool check = true);
		bplustree_header *SetToHeader();
		void Unset();

		status_t Free(Transaction *transaction, off_t offset);
		status_t Allocate(Transaction *transaction, bplustree_node **node, off_t *offset);
		status_t WriteBack(Transaction *transaction);

		bplustree_node *Node() const { return fNode; }

	protected:
		bplustree_node	*InternalSetTo(off_t offset);

		BPlusTree		*fTree;
		bplustree_node	*fNode;
		uint8			*fBlock;
		off_t			fBlockNumber;
};


//******** B+tree class *********

class BPlusTree {
	public:
		BPlusTree(Transaction *transaction, Inode *stream, int32 nodeSize = BPLUSTREE_NODE_SIZE);
		BPlusTree(Inode *stream);
		BPlusTree();
		~BPlusTree();

		status_t	SetTo(Transaction *transaction, Inode *stream, int32 nodeSize = BPLUSTREE_NODE_SIZE);
		status_t	SetTo(Inode *stream);
		status_t	SetStream(Inode *stream);

		status_t	InitCheck();
		status_t	Validate();

		status_t	Remove(Transaction *transaction, const uint8 *key, uint16 keyLength, off_t value);
		status_t	Insert(Transaction *transaction, const uint8 *key, uint16 keyLength, off_t value);

		status_t	Remove(Transaction *transaction, const char *key, off_t value);
		status_t	Insert(Transaction *transaction, const char *key, off_t value);
		status_t	Insert(Transaction *transaction, int32 key, off_t value);
		status_t	Insert(Transaction *transaction, uint32 key, off_t value);
		status_t	Insert(Transaction *transaction, int64 key, off_t value);
		status_t	Insert(Transaction *transaction, uint64 key, off_t value);
		status_t	Insert(Transaction *transaction, float key, off_t value);
		status_t	Insert(Transaction *transaction, double key, off_t value);

		status_t	Replace(Transaction *transaction, const uint8 *key, uint16 keyLength, off_t value);
		status_t	Find(const uint8 *key, uint16 keyLength, off_t *value);

		static int32 TypeCodeToKeyType(type_code code);
		static int32 ModeToKeyType(mode_t mode);

	private:
		BPlusTree(const BPlusTree &);
		BPlusTree &operator=(const BPlusTree &);
			// no implementation

		int32		CompareKeys(const void *key1, int keylength1, const void *key2, int keylength2);
		status_t	FindKey(bplustree_node *node, const uint8 *key, uint16 keyLength,
						uint16 *index = NULL, off_t *next = NULL);
		status_t	SeekDown(Stack<node_and_key> &stack, const uint8 *key, uint16 keyLength);

		status_t	FindFreeDuplicateFragment(bplustree_node *node, CachedNode *cached,
						off_t *_offset, bplustree_node **_fragment, uint32 *_index);
		status_t	InsertDuplicate(Transaction *transaction, CachedNode *cached,
						bplustree_node *node, uint16 index, off_t value);
		void		InsertKey(bplustree_node *node, uint16 index, uint8 *key, uint16 keyLength,
						off_t value);
		status_t	SplitNode(bplustree_node *node, off_t nodeOffset, bplustree_node *other,
						off_t otherOffset, uint16 *_keyIndex, uint8 *key, uint16 *_keyLength,
						off_t *_value);

		status_t	RemoveDuplicate(Transaction *transaction, bplustree_node *node,
						CachedNode *cached, uint16 keyIndex, off_t value);
		void		RemoveKey(bplustree_node *node, uint16 index);

		void		UpdateIterators(off_t offset, off_t nextOffset, uint16 keyIndex,
						uint16 splitAt, int8 change);
		void		AddIterator(TreeIterator *iterator);
		void		RemoveIterator(TreeIterator *iterator);

	private:
		friend struct TreeIterator;
		friend class CachedNode;

		Inode		*fStream;
		bplustree_header *fHeader;
		CachedNode	fCachedHeader;
		int32		fNodeSize;
		bool		fAllowDuplicates;
		status_t	fStatus;
		SimpleLock	fIteratorLock;
		Chain<TreeIterator> fIterators;
};


//***** helper classes/functions *****

extern int32 compareKeys(type_code type, const void *key1, int keyLength1,
				const void *key2, int keyLength2);

class TreeIterator {
	public:
		TreeIterator(BPlusTree *tree);
		~TreeIterator();

		status_t	Goto(int8 to);
		status_t	Traverse(int8 direction, void *key, uint16 *keyLength, uint16 maxLength,
						off_t *value, uint16 *duplicate = NULL);
		status_t	Find(const uint8 *key, uint16 keyLength);

		status_t	Rewind();
		status_t	GetNextEntry(void *key, uint16 *keyLength, uint16 maxLength,
						off_t *value, uint16 *duplicate = NULL);
		status_t	GetPreviousEntry(void *key, uint16 *keyLength, uint16 maxLength,
						off_t *value, uint16 *duplicate = NULL);
		void		SkipDuplicates();

#ifdef DEBUG
		void		Dump();
#endif

	private:
		BPlusTree	*fTree;

		off_t		fCurrentNodeOffset;	// traverse position
		int32		fCurrentKey;
		off_t		fDuplicateNode;
		uint16		fDuplicate, fNumDuplicates;
		bool		fIsFragment;

	private:
		friend class Chain<TreeIterator>;
		friend class BPlusTree;

		void Update(off_t offset, off_t nextOffset, uint16 keyIndex, uint16 splitAt, int8 change);
		void Stop();
		TreeIterator *fNext;
};

// BPlusTree's inline functions (most of them may not be needed)

inline status_t
BPlusTree::Remove(Transaction *transaction, const char *key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_STRING_TYPE)
		return B_BAD_TYPE;
	return Remove(transaction, (uint8 *)key, strlen(key), value);
}

inline status_t
BPlusTree::Insert(Transaction *transaction, const char *key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_STRING_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8 *)key, strlen(key), value);
}

inline status_t
BPlusTree::Insert(Transaction *transaction, int32 key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_INT32_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8 *)&key, sizeof(key), value);
}

inline status_t
BPlusTree::Insert(Transaction *transaction, uint32 key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_UINT32_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8 *)&key, sizeof(key), value);
}

inline status_t
BPlusTree::Insert(Transaction *transaction, int64 key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_INT64_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8 *)&key, sizeof(key), value);
}

inline status_t
BPlusTree::Insert(Transaction *transaction, uint64 key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_UINT64_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8 *)&key, sizeof(key), value);
}

inline status_t
BPlusTree::Insert(Transaction *transaction, float key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_FLOAT_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8 *)&key, sizeof(key), value);
}

inline status_t
BPlusTree::Insert(Transaction *transaction, double key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_DOUBLE_TYPE)
		return B_BAD_TYPE;
	return Insert(transaction, (uint8 *)&key, sizeof(key), value);
}


/************************ TreeIterator inline functions ************************/
//	#pragma mark -

inline status_t
TreeIterator::Rewind()
{
	return Goto(BPLUSTREE_BEGIN);
}

inline status_t
TreeIterator::GetNextEntry(void *key, uint16 *keyLength, uint16 maxLength,
	off_t *value, uint16 *duplicate)
{
	return Traverse(BPLUSTREE_FORWARD, key, keyLength, maxLength, value, duplicate);
}

inline status_t
TreeIterator::GetPreviousEntry(void *key, uint16 *keyLength, uint16 maxLength,
	off_t *value, uint16 *duplicate)
{
	return Traverse(BPLUSTREE_BACKWARD, key, keyLength, maxLength, value, duplicate);
}

/************************ bplustree_header inline functions ************************/
//	#pragma mark -


inline bool
bplustree_header::IsValidLink(off_t link)
{
	return link == BPLUSTREE_NULL || (link > 0 && link <= MaximumSize() - NodeSize());
}


/************************ bplustree_node inline functions ************************/
//	#pragma mark -


inline uint16 *
bplustree_node::KeyLengths() const
{
	return (uint16 *)(((char *)this) + round_up(sizeof(bplustree_node) + AllKeyLength()));
}


inline off_t *
bplustree_node::Values() const
{
	return (off_t *)((char *)KeyLengths() + NumKeys() * sizeof(uint16));
}


inline uint8 *
bplustree_node::Keys() const
{
	return (uint8 *)this + sizeof(bplustree_node);
}


inline int32
bplustree_node::Used() const
{
	return round_up(sizeof(bplustree_node) + AllKeyLength()) + NumKeys() * (sizeof(uint16) + sizeof(off_t));
}


inline bool 
bplustree_node::IsLeaf() const
{
	return OverflowLink() == BPLUSTREE_NULL;
}


inline duplicate_array *
bplustree_node::FragmentAt(int8 index)
{
	return (duplicate_array *)((off_t *)this + index * (NUM_FRAGMENT_VALUES + 1));
}


inline duplicate_array *
bplustree_node::DuplicateArray()
{
	return (duplicate_array *)&this->overflow_link;
}


inline uint8
bplustree_node::LinkType(off_t link)
{
	return *(uint64 *)&link >> 62;
}


inline off_t
bplustree_node::MakeLink(uint8 type, off_t link, uint32 fragmentIndex)
{
	return ((off_t)type << 62) | (link & 0x3ffffffffffffc00LL) | (fragmentIndex & 0x3ff);
}


inline bool 
bplustree_node::IsDuplicate(off_t link)
{
	return (LinkType(link) & (BPLUSTREE_DUPLICATE_NODE | BPLUSTREE_DUPLICATE_FRAGMENT)) > 0;
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

#endif	/* B_PLUS_TREE_H */
