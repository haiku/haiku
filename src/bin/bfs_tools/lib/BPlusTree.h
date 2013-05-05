#ifndef B_PLUS_TREE_H
#define B_PLUS_TREE_H
/* BPlusTree - BFS B+Tree implementation
**
** Copyright 2001 pinc Software. All Rights Reserved.
*/

#include <List.h>

#include <string.h>

#include "Cache.h"
#include "bfs.h"


class BPositionIO;


//****************** on-disk structures ********************

#define BPLUSTREE_NULL			-1LL
#define BPLUSTREE_FREE			-2LL

struct bplustree_header 
{
	uint32		magic;
	uint32		node_size;
	uint32		max_number_of_levels;
	uint32		data_type;
	off_t		root_node_pointer;
	off_t		free_node_pointer;
	off_t		maximum_size;
	
	inline bool IsValidLink(off_t link);
};

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

struct bplustree_node {
	off_t	left_link;
	off_t	right_link;
	off_t	overflow_link;
	uint16	all_key_count;
	uint16	all_key_length;
	
	inline uint16 *KeyLengths() const;
	inline off_t *Values() const;
	inline uint8 *Keys() const;
	inline int32 Used() const;
	uint8 *KeyAt(int32 index,uint16 *keyLength) const;
	
	uint8 CountDuplicates(off_t offset,bool isFragment) const;
	off_t DuplicateAt(off_t offset,bool isFragment,int8 index) const;

	static inline uint8 LinkType(off_t link);
	static inline off_t FragmentOffset(off_t link);
};

#define BPLUSTREE_NODE 0
#define BPLUSTREE_DUPLICATE_NODE 2
#define BPLUSTREE_DUPLICATE_FRAGMENT 3

//**************************************

enum bplustree_traversing {
	BPLUSTREE_FORWARD = 1,
	BPLUSTREE_BACKWARD = -1,
	
	BPLUSTREE_BEGIN = 0,
	BPLUSTREE_END = 1
};

template<class T> class Stack;
class BPlusTree;

class NodeCache : public Cache<off_t> {
	public:
		NodeCache(BPlusTree *);
		
		virtual Cacheable *NewCacheable(off_t offset);
		bplustree_node *Get(off_t offset);
//		void SetOffset(bplustree_node *,off_t offset);

	protected:
		BPlusTree	*fTree;
};


class BPlusTree {
	public:
		BPlusTree(int32 keyType, int32 nodeSize = BPLUSTREE_NODE_SIZE,
			bool allowDuplicates = true);
		BPlusTree(BPositionIO *stream, bool allowDuplicates = true);
		BPlusTree();
		~BPlusTree();

		status_t	SetTo(int32 keyType,int32 nodeSize = BPLUSTREE_NODE_SIZE,bool allowDuplicates = true);
		status_t	SetTo(BPositionIO *stream,bool allowDuplicates = true);

		status_t	InitCheck();
		status_t	Validate(bool verbose = false);

		int32		Type() const { return fHeader->data_type; }

		status_t	Rewind();
		status_t	Goto(int8 to);
		status_t	GetNextEntry(void *key,uint16 *keyLength,uint16 maxLength,off_t *value);
		status_t	GetPreviousEntry(void *key,uint16 *keyLength,uint16 maxLength,off_t *value);

		status_t	Insert(uint8 *key, uint16 keyLength, off_t value);
		
		status_t	Insert(const char *key, off_t value);
		status_t	Insert(int32 key, off_t value);
		status_t	Insert(uint32 key, off_t value);
		status_t	Insert(int64 key, off_t value);
		status_t	Insert(uint64 key, off_t value);
		status_t	Insert(float key, off_t value);
		status_t	Insert(double key, off_t value);

		status_t	Find(uint8 *key, uint16 keyLength, off_t *value);

		status_t	WriteTo(BPositionIO *stream);

		void 		SetHoldChanges(bool hold);

	private:
		// needed for searching
		struct node_and_key {
			off_t	nodeOffset;
			uint16	keyIndex;
		};

		void		Initialize(int32 nodeSize);

		void		SetCurrentNode(bplustree_node *node,off_t offset,int8 to = BPLUSTREE_BEGIN);
		status_t	Traverse(int8 direction,void *key,uint16 *keyLength,uint16 maxLength,off_t *value);

		int32		CompareKeys(const void *key1, int keylength1, const void *key2, int keylength2);
		status_t	FindKey(bplustree_node *node, uint8 *key, uint16 keyLength, uint16 *index = NULL, off_t *next = NULL);
		status_t	SeekDown(Stack<node_and_key> &stack, uint8 *key, uint16 keyLength);

		status_t	SplitNode(bplustree_node *node, off_t nodeOffset, uint16 *_keyIndex, uint8 *key, uint16 *_keyLength, off_t *_value);

		void		InsertKey(bplustree_node *node, uint8 *key, uint16 keyLength, off_t value, uint16 index);
		status_t	InsertDuplicate(bplustree_node *node,uint16 index);

		bool		CheckNode(bplustree_node *node);

	private:
		friend class NodeCache;
		bplustree_node *Node(off_t nodeoffset,bool check = true);

	private:
		BPositionIO	*fStream;
		bplustree_header *fHeader;
		int32		fNodeSize;
		bool		fAllowDuplicates;
		status_t	fStatus;

		NodeCache	fCache;

		off_t		fCurrentNodeOffset;	// traverse position
		int32		fCurrentKey;
		off_t		fDuplicateNode;
		uint16		fDuplicate, fNumDuplicates;
		bool		fIsFragment;
};

// inline functions

inline status_t BPlusTree::Rewind()
{
	return Goto(BPLUSTREE_BEGIN);
}

inline status_t BPlusTree::GetNextEntry(void *key,uint16 *keyLength,uint16 maxLength,off_t *value)
{
	return Traverse(BPLUSTREE_FORWARD,key,keyLength,maxLength,value);
}

inline status_t BPlusTree::GetPreviousEntry(void *key,uint16 *keyLength,uint16 maxLength,off_t *value)
{
	return Traverse(BPLUSTREE_BACKWARD,key,keyLength,maxLength,value);
}

inline status_t BPlusTree::Insert(const char *key,off_t value)
{
	if (fHeader->data_type != BPLUSTREE_STRING_TYPE)
		return B_BAD_TYPE;
	return Insert((uint8 *)key, strlen(key), value);
}

inline status_t BPlusTree::Insert(int32 key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_INT32_TYPE)
		return B_BAD_TYPE;
	return Insert((uint8 *)&key, sizeof(key), value);
}

inline status_t BPlusTree::Insert(uint32 key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_UINT32_TYPE)
		return B_BAD_TYPE;
	return Insert((uint8 *)&key, sizeof(key), value);
}

inline status_t BPlusTree::Insert(int64 key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_INT64_TYPE)
		return B_BAD_TYPE;
	return Insert((uint8 *)&key, sizeof(key), value);
}

inline status_t BPlusTree::Insert(uint64 key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_UINT64_TYPE)
		return B_BAD_TYPE;
	return Insert((uint8 *)&key, sizeof(key), value);
}

inline status_t BPlusTree::Insert(float key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_FLOAT_TYPE)
		return B_BAD_TYPE;
	return Insert((uint8 *)&key, sizeof(key), value);
}

inline status_t BPlusTree::Insert(double key, off_t value)
{
	if (fHeader->data_type != BPLUSTREE_DOUBLE_TYPE)
		return B_BAD_TYPE;
	return Insert((uint8 *)&key, sizeof(key), value);
}


/************************ bplustree_header inline functions ************************/
//	#pragma mark -


inline bool bplustree_header::IsValidLink(off_t link)
{
	return link == BPLUSTREE_NULL || (link > 0 && link <= maximum_size - node_size);
}


/************************ bplustree_node inline functions ************************/
//	#pragma mark -


inline uint16 *bplustree_node::KeyLengths() const
{
	return (uint16 *)(((char *)this) + round_up(sizeof(bplustree_node) + all_key_length));
}

inline off_t *bplustree_node::Values() const
{
	return (off_t *)((char *)KeyLengths() + all_key_count * sizeof(uint16));
}

inline uint8 *bplustree_node::Keys() const
{
	return (uint8 *)this + sizeof(bplustree_node);
}

inline int32 bplustree_node::Used() const
{
	return round_up(sizeof(bplustree_node) + all_key_length) + all_key_count * (sizeof(uint16) + sizeof(off_t));
}

inline uint8 bplustree_node::LinkType(off_t link)
{
	return *(uint64 *)&link >> 62;
}

inline off_t bplustree_node::FragmentOffset(off_t link)
{
	return link & 0x3ffffffffffffc00LL;
}

#endif	/* B_PLUS_TREE_H */
