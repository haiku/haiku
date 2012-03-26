/* BPlusTree - BFS B+Tree implementation
**
** Copyright 2001-2002 pinc Software. All Rights Reserved.
*/


#include "BPlusTree.h"
#include "Inode.h"
#include "Stack.h"
#include "dump.h"

#include <Debug.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define MAX_NODES_IN_CACHE 20

class CacheableNode : public NodeCache::Cacheable
{
	public:
		CacheableNode(off_t offset,bplustree_node *node)
			:
			fOffset(offset),
			fNode(node)
		{
		}

		virtual ~CacheableNode()
		{
			if (fNode)
				free(fNode);
		}

		virtual bool Equals(off_t offset)
		{
			return offset == fOffset;
		}

		off_t			fOffset;
		bplustree_node	*fNode;
};


NodeCache::NodeCache(BPlusTree *tree)
	:	Cache<off_t>(),
	fTree(tree)
{
}


Cache<off_t>::Cacheable *
NodeCache::NewCacheable(off_t offset)
{
	return new CacheableNode(offset,fTree->Node(offset,false));
}


bplustree_node *
NodeCache::Get(off_t offset)
{
	CacheableNode *node = (CacheableNode *)Cache<off_t>::Get(offset);
	return node->fNode;
}


//	#pragma mark -


BPlusTree::BPlusTree(int32 keyType,int32 nodeSize,bool allowDuplicates)
	:
	fStream(NULL),
	fHeader(NULL),
	fCache(this),
	fCurrentNodeOffset(BPLUSTREE_NULL)
{
	SetTo(keyType,nodeSize,allowDuplicates);
}


BPlusTree::BPlusTree(BPositionIO *stream,bool allowDuplicates)
	:
	fStream(NULL),
	fHeader(NULL),
	fCache(this),
	fCurrentNodeOffset(BPLUSTREE_NULL)
{
	SetTo(stream,allowDuplicates);
}


BPlusTree::BPlusTree()
	:
	fStream(NULL),
	fHeader(NULL),
	fNodeSize(BPLUSTREE_NODE_SIZE),
	fAllowDuplicates(true),
	fStatus(B_NO_INIT),
	fCache(this),
	fCurrentNodeOffset(BPLUSTREE_NULL)
{
}


BPlusTree::~BPlusTree()
{
	fCache.Clear();
}


void BPlusTree::Initialize(int32 nodeSize)
{
	// free old data
	fCache.Clear(0,true);

	if (fHeader)
		free(fHeader);

	fStream = NULL;

	fNodeSize = nodeSize;
	fHeader = (bplustree_header *)malloc(fNodeSize);
 	memset(fHeader,0,fNodeSize);
 	
 	fCurrentNodeOffset = BPLUSTREE_NULL;
}


status_t BPlusTree::SetTo(int32 keyType,int32 nodeSize,bool allowDuplicates)
{
	// initializes in-memory B+Tree

	Initialize(nodeSize);

	fAllowDuplicates = allowDuplicates;
	fCache.SetHoldChanges(true);

	// initialize b+tree header
 	fHeader->magic = BPLUSTREE_MAGIC;
 	fHeader->node_size = fNodeSize;
 	fHeader->max_number_of_levels = 1;
 	fHeader->data_type = keyType;
 	fHeader->root_node_pointer = fNodeSize;
 	fHeader->free_node_pointer = BPLUSTREE_NULL;
 	fHeader->maximum_size = fNodeSize * 2;
 	
	return fStatus = B_OK;
}


status_t BPlusTree::SetTo(BPositionIO *stream,bool allowDuplicates)
{
	// initializes on-disk B+Tree

	bplustree_header header;
	ssize_t read = stream->ReadAt(0,&header,sizeof(bplustree_header));
	if (read < 0)
		return fStatus = read;

	// is header valid?
	
	stream->Seek(0,SEEK_END);
	off_t size = stream->Position();
	stream->Seek(0,SEEK_SET);

	//dump_bplustree_header(&header);

	if (header.magic != BPLUSTREE_MAGIC
		|| header.maximum_size != size
		|| (header.root_node_pointer % header.node_size) != 0
		|| !header.IsValidLink(header.root_node_pointer)
		|| !header.IsValidLink(header.free_node_pointer))
		return fStatus = B_BAD_DATA;

	fAllowDuplicates = allowDuplicates;

	if (DataStream *dataStream = dynamic_cast<DataStream *>(stream))
	{
		uint32 toMode[] = {S_STR_INDEX, S_INT_INDEX, S_UINT_INDEX, S_LONG_LONG_INDEX,
						   S_ULONG_LONG_INDEX, S_FLOAT_INDEX, S_DOUBLE_INDEX};
		uint32 mode = dataStream->Mode() & (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX | S_LONG_LONG_INDEX
						   | S_ULONG_LONG_INDEX | S_FLOAT_INDEX | S_DOUBLE_INDEX);
	
		if (header.data_type > BPLUSTREE_DOUBLE_TYPE
			|| (dataStream->Mode() & S_INDEX_DIR) && toMode[header.data_type] != mode
			|| !dataStream->IsDirectory())
			return fStatus = B_BAD_TYPE;

		 // although it's in stat.h, the S_ALLOW_DUPS flag is obviously unused
		fAllowDuplicates = (dataStream->Mode() & (S_INDEX_DIR | 0777)) == S_INDEX_DIR;

		//printf("allows duplicates? %s\n",fAllowDuplicates ? "yes" : "no");
	}

	Initialize(header.node_size);

	memcpy(fHeader,&header,sizeof(bplustree_header));

	fStream = stream;

	bplustree_node *node = fCache.Get(header.root_node_pointer);
	//if (node)
	//	dump_bplustree_node(node);

	return fStatus = node && CheckNode(node) ? B_OK : B_BAD_DATA;
}


status_t BPlusTree::InitCheck()
{
	return fStatus;
}


struct validate_info {
	off_t	offset;
	off_t	from;
	bool	free;
};


status_t
BPlusTree::Validate(bool verbose)
{
	// validates the on-disk b+tree
	// (for now only the node integrity is checked)

	if (!fStream)
		return B_OK;

	struct validate_info info;

	Stack<validate_info> stack;
	info.offset = fHeader->root_node_pointer;
	info.from = BPLUSTREE_NULL;
	info.free = false;
	stack.Push(info);

	if (fHeader->free_node_pointer != BPLUSTREE_NULL) {
		info.offset = fHeader->free_node_pointer;
		info.free = true;
		stack.Push(info);
	}

	char escape[3] = {0x1b, '[', 0};
	int32 count = 0,freeCount = 0;

	while (true) {
		if (!stack.Pop(&info)) {
			if (verbose) {
				printf("  %ld B+tree node(s) processed (%ld free node(s)).\n",
					count, freeCount);
			}
			return B_OK;
		}

		if (!info.free && verbose && ++count % 10 == 0)
			printf("  %ld%s1A\n", count, escape);
		else if (info.free)
			freeCount++;

		bplustree_node *node;
		if ((node = fCache.Get(info.offset)) == NULL
			|| !info.free && !CheckNode(node)) {
			if (verbose) {
				fprintf(stderr,"  B+Tree: Could not get data at position %Ld (referenced by node at %Ld)\n",info.offset,info.from);
				if ((node = fCache.Get(info.from)) != NULL)
					dump_bplustree_node(node,fHeader);
			}
			return B_BAD_DATA;
		}

		info.from = info.offset;

		if (node->overflow_link != BPLUSTREE_NULL && !info.free) {
			info.offset = node->overflow_link;
			stack.Push(info);

			//dump_bplustree_node(node,fHeader);
			off_t *values = node->Values();
			for (int32 i = 0;i < node->all_key_count;i++) {
				info.offset = values[i];
				stack.Push(info);
			}
		} else if (node->left_link != BPLUSTREE_NULL && info.free) {
			info.offset = node->left_link;
			stack.Push(info);
		}
	}
}


status_t
BPlusTree::WriteTo(BPositionIO *stream)
{
	ssize_t written = stream->WriteAt(0,fHeader,fNodeSize);
	if (written < fNodeSize)
		return written < B_OK ? written : B_IO_ERROR;

	for (off_t offset = fNodeSize;offset < fHeader->maximum_size;offset += fNodeSize)
	{
		bplustree_node *node = fCache.Get(offset);
		if (node == NULL)
			return B_ERROR;

		written = stream->WriteAt(offset,node,fNodeSize);
		if (written < fNodeSize)
			return written < B_OK ? written : B_IO_ERROR;
	}
	return stream->SetSize(fHeader->maximum_size);
}


//	#pragma mark -


void BPlusTree::SetCurrentNode(bplustree_node *node,off_t offset,int8 to)
{
	fCurrentNodeOffset = offset;
	fCurrentKey = to == BPLUSTREE_BEGIN ? -1 : node->all_key_count;
	fDuplicateNode = BPLUSTREE_NULL;
}


status_t BPlusTree::Goto(int8 to)
{
	if (fHeader == NULL)
		return B_BAD_VALUE;

	Stack<off_t> stack;
	if (stack.Push(fHeader->root_node_pointer) < B_OK)
		return B_NO_MEMORY;

	bplustree_node *node;
	off_t pos;
	while (stack.Pop(&pos) && (node = fCache.Get(pos)) != NULL && CheckNode(node))
	{
		// is the node a leaf node?
		if (node->overflow_link == BPLUSTREE_NULL)
		{
			SetCurrentNode(node,pos,to);

			return B_OK;
		}

		if (to == BPLUSTREE_END || node->all_key_count == 0)
			pos = node->overflow_link;
		else
		{
			if (node->all_key_length > fNodeSize
				|| (uint32)node->Values() > (uint32)node + fNodeSize - 8 * node->all_key_count)
				return B_ERROR;
			
			pos = *node->Values();
		}

		if (stack.Push(pos) < B_OK)
			break;
	}
	return B_ERROR;
}


status_t BPlusTree::Traverse(int8 direction,void *key,uint16 *keyLength,uint16 maxLength,off_t *value)
{
	if (fCurrentNodeOffset == BPLUSTREE_NULL
		&& Goto(direction == BPLUSTREE_FORWARD ? BPLUSTREE_BEGIN : BPLUSTREE_END) < B_OK) 
		return B_ERROR;

	bplustree_node *node;

	if (fDuplicateNode != BPLUSTREE_NULL)
	{
		// regardless of traverse direction the duplicates are always presented in
		// the same order; since they are all considered as equal, this shouldn't
		// cause any problems

		if (!fIsFragment || fDuplicate < fNumDuplicates)
			node = fCache.Get(bplustree_node::FragmentOffset(fDuplicateNode));
		else
			node = NULL;

		if (node != NULL)
		{
			if (!fIsFragment && fDuplicate >= fNumDuplicates)
			{
				// if the node is out of duplicates, we go directly to the next one
				fDuplicateNode = node->right_link;
				if (fDuplicateNode != BPLUSTREE_NULL
					&& (node = fCache.Get(fDuplicateNode)) != NULL)
				{
					fNumDuplicates = node->CountDuplicates(fDuplicateNode,false);
					fDuplicate = 0;
				}
			}
			if (fDuplicate < fNumDuplicates)
			{
				*value = node->DuplicateAt(fDuplicateNode,fIsFragment,fDuplicate++);
				return B_OK;
			}
		}
		fDuplicateNode = BPLUSTREE_NULL;
	}

	off_t savedNodeOffset = fCurrentNodeOffset;
	if ((node = fCache.Get(fCurrentNodeOffset)) == NULL || !CheckNode(node))
		return B_ERROR;

	fCurrentKey += direction;
	
	// is the current key in the current node?
	while ((direction == BPLUSTREE_FORWARD && fCurrentKey >= node->all_key_count)
		   || (direction == BPLUSTREE_BACKWARD && fCurrentKey < 0))
	{
		fCurrentNodeOffset = direction == BPLUSTREE_FORWARD ? node->right_link : node->left_link;

		// are there any more nodes?
		if (fCurrentNodeOffset != BPLUSTREE_NULL)
		{
			node = fCache.Get(fCurrentNodeOffset);
			if (!node || !CheckNode(node))
				return B_ERROR;

			// reset current key
			fCurrentKey = direction == BPLUSTREE_FORWARD ? 0 : node->all_key_count;
		}
		else
		{
			// there are no nodes left, so turn back to the last key
			fCurrentNodeOffset = savedNodeOffset;
			fCurrentKey = direction == BPLUSTREE_FORWARD ? node->all_key_count : -1;

			return B_ENTRY_NOT_FOUND;
		}
	}

	if (node->all_key_count == 0)
		return B_ERROR; //B_ENTRY_NOT_FOUND;

	uint16 length;
	uint8 *keyStart = node->KeyAt(fCurrentKey,&length);

	length = min_c(length,maxLength);
	memcpy(key,keyStart,length);
	
	if (fHeader->data_type == BPLUSTREE_STRING_TYPE)	// terminate string type
	{
		if (length == maxLength)
			length--;
		((char *)key)[length] = '\0';
	}
	*keyLength = length;

	off_t offset = node->Values()[fCurrentKey];
	
	// duplicate fragments?
	uint8 type = bplustree_node::LinkType(offset);
	if (type == BPLUSTREE_DUPLICATE_FRAGMENT || type == BPLUSTREE_DUPLICATE_NODE)
	{
		fDuplicateNode = offset;

		node = fCache.Get(bplustree_node::FragmentOffset(fDuplicateNode));
		if (node == NULL)
			return B_ERROR;

		fIsFragment = type == BPLUSTREE_DUPLICATE_FRAGMENT;

		fNumDuplicates = node->CountDuplicates(offset,fIsFragment);
		if (fNumDuplicates)
		{
			// give back first duplicate
			fDuplicate = 1;
			offset = node->DuplicateAt(offset,fIsFragment,0);
		}
		else
		{
			// shouldn't happen, but we're dealing here with corrupt disks...
			fDuplicateNode = BPLUSTREE_NULL;
			offset = 0;
		}
	}
	*value = offset;

	return B_OK;
}


int32 BPlusTree::CompareKeys(const void *key1, int keyLength1, const void *key2, int keyLength2)
{
	switch (fHeader->data_type)
	{
	    case BPLUSTREE_STRING_TYPE:
    	{
			int len = min_c(keyLength1,keyLength2);
			int result = strncmp((const char *)key1,(const char *)key2,len);
			
			if (result == 0)
				result = keyLength1 - keyLength2;

			return result;
		}

		case BPLUSTREE_INT32_TYPE:
			return *(int32 *)key1 - *(int32 *)key2;
			
		case BPLUSTREE_UINT32_TYPE:
		{
			if (*(uint32 *)key1 == *(uint32 *)key2)
				return 0;
			else if (*(uint32 *)key1 > *(uint32 *)key2)
				return 1;

			return -1;
		}
			
		case BPLUSTREE_INT64_TYPE:
		{
			if (*(int64 *)key1 == *(int64 *)key2)
				return 0;
			else if (*(int64 *)key1 > *(int64 *)key2)
				return 1;

			return -1;
		}

		case BPLUSTREE_UINT64_TYPE:
		{
			if (*(uint64 *)key1 == *(uint64 *)key2)
				return 0;
			else if (*(uint64 *)key1 > *(uint64 *)key2)
				return 1;

			return -1;
		}

		case BPLUSTREE_FLOAT_TYPE:
		{
			float result = *(float *)key1 - *(float *)key2;
			if (result == 0.0f)
				return 0;

			return (result < 0.0f) ? -1 : 1;
		}

		case BPLUSTREE_DOUBLE_TYPE:
		{
			double result = *(double *)key1 - *(double *)key2;
			if (result == 0.0)
				return 0;

			return (result < 0.0) ? -1 : 1;
		}
	}
	return 0;
}


status_t BPlusTree::FindKey(bplustree_node *node,uint8 *key,uint16 keyLength,uint16 *index,off_t *next)
{
	if (node->all_key_count == 0)
	{
		if (index)
			*index = 0;
		if (next)
			*next = node->overflow_link;
		return B_ENTRY_NOT_FOUND;
	}

	off_t *values = node->Values();
	int16 saveIndex = 0;

	// binary search in the key array
	for (int16 first = 0,last = node->all_key_count - 1;first <= last;)
	{
		uint16 i = (first + last) >> 1;

		uint16 searchLength;
		uint8 *searchKey = node->KeyAt(i,&searchLength);
		
		int32 cmp = CompareKeys(key,keyLength,searchKey,searchLength);
		if (cmp < 0)
		{
			last = i - 1;
			saveIndex = i;
		}
		else if (cmp > 0)
		{
			saveIndex = first = i + 1;
		}
		else
		{
			if (index)
				*index = i;
			if (next)
				*next = values[i];
			return B_OK;
		}
	}

	if (index)
		*index = saveIndex;
	if (next)
	{
		if (saveIndex == node->all_key_count)
			*next = node->overflow_link;
		else
			*next = values[saveIndex];
	}
	return B_ENTRY_NOT_FOUND;
}


status_t BPlusTree::SeekDown(Stack<node_and_key> &stack,uint8 *key,uint16 keyLength)
{
	node_and_key nodeAndKey;
	nodeAndKey.nodeOffset = fHeader->root_node_pointer;
	nodeAndKey.keyIndex = 0;

	bplustree_node *node;
	while ((node = fCache.Get(nodeAndKey.nodeOffset)) != NULL && CheckNode(node)) {
		// if we are already on leaf level, we're done
		if (node->overflow_link == BPLUSTREE_NULL) {
			// put the node on the stack
			// node that the keyIndex is not properly set here!
			nodeAndKey.keyIndex = 0;
			stack.Push(nodeAndKey);
			return B_OK;
		}

		off_t nextOffset;
		status_t status = FindKey(node,key,keyLength,&nodeAndKey.keyIndex,&nextOffset);

		if (status == B_ENTRY_NOT_FOUND && nextOffset == nodeAndKey.nodeOffset)
			return B_ERROR;

		// put the node & the correct keyIndex on the stack
		stack.Push(nodeAndKey);

		nodeAndKey.nodeOffset = nextOffset;
	}
	return B_ERROR;
}


void BPlusTree::InsertKey(bplustree_node *node,uint8 *key,uint16 keyLength,off_t value,uint16 index)
{
	// should never happen, but who knows?
	if (index > node->all_key_count)
		return;

	off_t *values = node->Values();
	uint16 *keyLengths = node->KeyLengths();
	uint8 *keys = node->Keys();

	node->all_key_count++;
	node->all_key_length += keyLength;

	off_t *newValues = node->Values();
	uint16 *newKeyLengths = node->KeyLengths();

	// move values and copy new value into them
	memmove(newValues + index + 1,values + index,sizeof(off_t) * (node->all_key_count - 1 - index));
	memmove(newValues,values,sizeof(off_t) * index);

	newValues[index] = value;

	// move and update key length index
	for (uint16 i = node->all_key_count;i-- > index + 1;)
		newKeyLengths[i] = keyLengths[i - 1] + keyLength;
	memmove(newKeyLengths,keyLengths,sizeof(uint16) * index);

	int32 keyStart;
	newKeyLengths[index] = keyLength + (keyStart = index > 0 ? newKeyLengths[index - 1] : 0);

	// move keys and copy new key into them
	int32 size = node->all_key_length - newKeyLengths[index];
	if (size > 0)
		memmove(keys + newKeyLengths[index],keys + newKeyLengths[index] - keyLength,size);

	memcpy(keys + keyStart,key,keyLength);
}


status_t BPlusTree::InsertDuplicate(bplustree_node */*node*/,uint16 /*index*/)
{
	printf("DUPLICATE ENTRY!!\n");

//		/* handle dup keys */
//		if (dupflg == 0)
//		{
//			bt_errno(b) = BT_DUPKEY;
//			goto bombout;
//		}
//		else
//		{
//			/* paste new data ptr in page */
//			/* and write it out again. */
//			off_t	*p;
//			p = (off_t *)KEYCLD(op->p);
//			*(p + keyat) = rrn;
//			op->flags = BT_CHE_DIRTY;
//			if(bt_wpage(b,op) == BT_ERR ||
//				bt_wpage(b,kp) == BT_ERR)
//				return(BT_ERR);
//
//			/* mark all as well with tree */
//			bt_clearerr(b);
//			return(BT_OK);
//		}

	return B_OK;
}


status_t BPlusTree::SplitNode(bplustree_node *node,off_t nodeOffset,uint16 *_keyIndex,uint8 *key,uint16 *_keyLength,off_t *_value)
{
	if (*_keyIndex > node->all_key_count + 1)
		return B_BAD_VALUE;

	//printf("before (insert \"%s\" (%d bytes) at %d):\n\n",key,*_keyLength,*_keyIndex);
	//dump_bplustree_node(node,fHeader);
	//hexdump(node,fNodeSize);

	off_t otherOffset;
	bplustree_node *other = fCache.Get(otherOffset = fHeader->maximum_size); //Node(otherOffset = fHeader->maximum_size/*,false*/);
	if (other == NULL)
		return B_NO_MEMORY;

	uint16 *inKeyLengths = node->KeyLengths();
	off_t *inKeyValues = node->Values();
	uint8 *inKeys = node->Keys();
	uint8 *outKeys = other->Keys();
	uint16 keyIndex = *_keyIndex;

	// how many keys will fit in one (half) page?

	// "bytes" is the number of bytes written for the new key,
	// "bytesBefore" are the bytes before that key
	// "bytesAfter" are the bytes after the new key, if any
	int32 bytes = 0,bytesBefore = 0,bytesAfter = 0;

	size_t size = fNodeSize >> 1;
	int32 out,in;
	for (in = out = 0;in < node->all_key_count + 1;) {
		if (!bytes)
			bytesBefore = in > 0 ? inKeyLengths[in - 1] : 0;
		if (in == keyIndex && !bytes) {
			bytes = *_keyLength;
		} else {
			if (keyIndex < out) {
				bytesAfter = inKeyLengths[in] - bytesBefore;
				// fix the key lengths for the new node
				inKeyLengths[in] = bytesAfter + bytesBefore + bytes;
			} //else
			in++;
		}
		out++;

		if (round_up(sizeof(bplustree_node) + bytesBefore + bytesAfter + bytes) +
						out * (sizeof(uint16) + sizeof(off_t)) >= size) {
			// we have found the number of keys in the new node!
			break;
		}
	}

	// if the new key was not inserted, set the length of the keys
	// that can be copied directly
	if (keyIndex >= out && in > 0)
		bytesBefore = inKeyLengths[in - 1];

	if (bytesBefore < 0 || bytesAfter < 0)
		return B_BAD_DATA;

	//printf("put %ld keys in the other node (%ld, %ld, %ld) (in = %ld)\n",out,bytesBefore,bytes,bytesAfter,in);

	other->left_link = node->left_link;
	other->right_link = nodeOffset;
	other->all_key_length = bytes + bytesBefore + bytesAfter;
	other->all_key_count = out;
	//printf("-> used = %ld (bplustree_node = %ld bytes)\n",other->Used(),sizeof(bplustree_node));

	uint16 *outKeyLengths = other->KeyLengths();
	off_t *outKeyValues = other->Values();
	int32 keys = out > keyIndex ? keyIndex : out;

	if (bytesBefore) {
		// copy the keys
		memcpy(outKeys,inKeys,bytesBefore);
		memcpy(outKeyLengths,inKeyLengths,keys * sizeof(uint16));
		memcpy(outKeyValues,inKeyValues,keys * sizeof(off_t));
	}
	if (bytes) {
		// copy the newly inserted key
		memcpy(outKeys + bytesBefore,key,bytes);
		outKeyLengths[keyIndex] = bytes + bytesBefore;
		outKeyValues[keyIndex] = *_value;

		if (bytesAfter) {
			// copy the keys after the new key
			memcpy(outKeys + bytesBefore + bytes,inKeys + bytesBefore,bytesAfter);
			keys = out - keyIndex - 1;
			memcpy(outKeyLengths + keyIndex + 1,inKeyLengths + keyIndex,keys * sizeof(uint16));
			memcpy(outKeyValues + keyIndex + 1,inKeyValues + keyIndex,keys * sizeof(off_t));
		}
	}

	// if the new key was already inserted, we shouldn't use it again
	if (in != out)
		keyIndex--;

	int32 total = bytesBefore + bytesAfter;

	// if we have split an index node, we have to drop the first key
	// of the next node (which can also be the new key to insert)
	if (node->overflow_link != BPLUSTREE_NULL) {
		if (in == keyIndex) {
			other->overflow_link = *_value;
			keyIndex--;
		} else {
			other->overflow_link = inKeyValues[in];
			total = inKeyLengths[in++];
		}
	}

	// and now the same game for the other page and the rest of the keys
	// (but with memmove() instead of memcpy(), because they may overlap)

	bytesBefore = bytesAfter = bytes = 0;
	out = 0;
	int32 skip = in;
	while (in < node->all_key_count + 1) {
		//printf("in = %ld, keyIndex = %d, bytes = %ld\n",in,keyIndex,bytes);
		if (in == keyIndex && !bytes) {
			bytesBefore = in > skip ? inKeyLengths[in - 1] : 0;
			//printf("bytesBefore = %ld\n",bytesBefore);
			bytes = *_keyLength;
		} else if (in < node->all_key_count) {
			//printf("1.inKeyLength[%ld] = %u\n",in,inKeyLengths[in]);
			inKeyLengths[in] -= total;
			if (bytes) {
				inKeyLengths[in] += bytes;
				bytesAfter = inKeyLengths[in] - bytesBefore - bytes;
				//printf("2.inKeyLength[%ld] = %u, bytesAfter = %ld, bytesBefore = %ld\n",in,inKeyLengths[in],bytesAfter,bytesBefore);
			}
			
			in++;
		} else
			in++;

		out++;

		//printf("-> out = %ld, keylen = %ld, %ld bytes total\n",out,bytesBefore,round_up(sizeof(bplustree_node) + bytesBefore + bytesAfter + bytes) +
		//				out * (sizeof(uint16) + sizeof(off_t)));
		if (in > node->all_key_count && keyIndex < in)
			break;
	}

	//printf("bytes = (%ld, %ld, %ld), in = %ld, total = %ld\n",bytesBefore,bytes,bytesAfter,in,total);

	if (keyIndex >= in && keyIndex - skip < out) {
		bytesAfter = inKeyLengths[in] - bytesBefore - total;
	} else if (keyIndex < skip)
		bytesBefore = node->all_key_length - total;

	//printf("bytes = (%ld, %ld, %ld), in = %ld, total = %ld\n",bytesBefore,bytes,bytesAfter,in,total);

	if (bytesBefore < 0 || bytesAfter < 0)
		return B_BAD_DATA;

	node->left_link = otherOffset;
		// right link, and overflow link can stay the same
	node->all_key_length = bytes + bytesBefore + bytesAfter;
	node->all_key_count = out - 1;

	// array positions have changed
	outKeyLengths = node->KeyLengths();
	outKeyValues = node->Values();

	//printf("put %ld keys in the first node (%ld, %ld, %ld) (in = %ld)\n",out,bytesBefore,bytes,bytesAfter,in);

	// move the keys in the old node: the order is important here,
	// because we don't want to overwrite any contents

	keys = keyIndex <= skip ? out : keyIndex - skip;
	keyIndex -= skip;

	if (bytesBefore)
		memmove(inKeys,inKeys + total,bytesBefore);
	if (bytesAfter)
		memmove(inKeys + bytesBefore + bytes,inKeys + total + bytesBefore,bytesAfter);

	if (bytesBefore)
		memmove(outKeyLengths,inKeyLengths + skip,keys * sizeof(uint16));
	in = out - keyIndex - 1;
	if (bytesAfter)
		memmove(outKeyLengths + keyIndex + 1,inKeyLengths + skip + keyIndex,in * sizeof(uint16));

	if (bytesBefore)
		memmove(outKeyValues,inKeyValues + skip,keys * sizeof(off_t));
	if (bytesAfter)
		memmove(outKeyValues + keyIndex + 1,inKeyValues + skip + keyIndex,in * sizeof(off_t));

	if (bytes) {
		// finally, copy the newly inserted key (don't overwrite anything)
		memcpy(inKeys + bytesBefore,key,bytes);
		outKeyLengths[keyIndex] = bytes + bytesBefore;
		outKeyValues[keyIndex] = *_value;
	}

	//puts("\n!!!!!!!!!!!!!!! after: !!!!!!!!!!!!!!!\n");
	//dump_bplustree_node(other,fHeader);
	//hexdump(other,fNodeSize);
	//puts("\n");
	//dump_bplustree_node(node,fHeader);
	//hexdump(node,fNodeSize);

	// write the updated nodes back
	
	fCache.SetDirty(otherOffset,true);
	fCache.SetDirty(nodeOffset,true);

	// update the right link of the node in the left of the new node
	if (other->left_link != BPLUSTREE_NULL) {
		bplustree_node *left = fCache.Get(other->left_link);
		if (left != NULL) {
			left->right_link = otherOffset;
			fCache.SetDirty(other->left_link,true);
		}
	}

	// prepare key to insert in the parent node
	
	//printf("skip: %ld, count-1: %u\n",skip,other->all_key_count - 1);
	uint16 length;
	uint8 *lastKey = other->KeyAt(other->all_key_count - 1,&length);
	memcpy(key,lastKey,length);
	*_keyLength = length;
	*_value = otherOffset;

	return B_OK;
}


status_t BPlusTree::Insert(uint8 *key,uint16 keyLength,off_t value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH || keyLength > BPLUSTREE_MAX_KEY_LENGTH)
		return B_BAD_VALUE;

	Stack<node_and_key> stack;
	if (SeekDown(stack,key,keyLength) != B_OK)
		return B_ERROR;

	uint8 keyBuffer[BPLUSTREE_MAX_KEY_LENGTH + 1];

	memcpy(keyBuffer,key,keyLength);
	keyBuffer[keyLength] = 0;

	off_t valueToInsert = value;

	fCurrentNodeOffset = BPLUSTREE_NULL;

	node_and_key nodeAndKey;
	bplustree_node *node;
	uint32 count = 0;

	while (stack.Pop(&nodeAndKey) && (node = fCache.Get(nodeAndKey.nodeOffset)) != NULL && CheckNode(node))
	{
		if (count++ == 0)	// first round, check for duplicate entries
		{
			status_t status = FindKey(node,key,keyLength,&nodeAndKey.keyIndex);
			if (status == B_ERROR)
				return B_ERROR;

			// is this a duplicate entry?
			if (status == B_OK && node->overflow_link == BPLUSTREE_NULL)
			{
				if (fAllowDuplicates)
					return InsertDuplicate(node,nodeAndKey.keyIndex);
				else
					return B_NAME_IN_USE;
			}
		}

		// is the node big enough to hold the pair?
		if (int32(round_up(sizeof(bplustree_node) + node->all_key_length + keyLength)
			+ (node->all_key_count + 1) * (sizeof(uint16) + sizeof(off_t))) < fNodeSize)
		{
			InsertKey(node,keyBuffer,keyLength,valueToInsert,nodeAndKey.keyIndex);
			fCache.SetDirty(nodeAndKey.nodeOffset,true);

			return B_OK;
		}
		else
		{
			// do we need to allocate a new root node? if so, then do
			// it now
			bplustree_node *rootNode = NULL;
			off_t newRoot = BPLUSTREE_NULL;
			if (nodeAndKey.nodeOffset == fHeader->root_node_pointer) {
				rootNode = fCache.Get(newRoot = fHeader->maximum_size);
				if (rootNode == NULL) {
					// the tree is most likely dead
					return B_NO_MEMORY;
				}
			}

			if (SplitNode(node,nodeAndKey.nodeOffset,&nodeAndKey.keyIndex,keyBuffer,&keyLength,&valueToInsert) < B_OK) {
				if (newRoot != BPLUSTREE_NULL) {
					// free root node
				}
				return B_ERROR;
			}

			if (newRoot != BPLUSTREE_NULL) {
				rootNode->overflow_link = nodeAndKey.nodeOffset;
				
				InsertKey(rootNode,keyBuffer,keyLength,node->left_link,0);

				fHeader->root_node_pointer = newRoot;
				fHeader->max_number_of_levels++;
				// write header
				
				fCache.SetDirty(newRoot,true);

				return B_OK;
			}
		}
	}

	return B_ERROR;
}


status_t BPlusTree::Find(uint8 *key,uint16 keyLength,off_t *value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH || keyLength > BPLUSTREE_MAX_KEY_LENGTH)
		return B_BAD_VALUE;

	Stack<node_and_key> stack;
	if (SeekDown(stack,key,keyLength) != B_OK)
		return B_ERROR;

	fCurrentNodeOffset = BPLUSTREE_NULL;

	node_and_key nodeAndKey;
	bplustree_node *node;

	if (stack.Pop(&nodeAndKey) && (node = fCache.Get(nodeAndKey.nodeOffset)) != NULL && CheckNode(node))
	{
		status_t status = FindKey(node,key,keyLength,&nodeAndKey.keyIndex);
		if (status == B_ERROR)
			return B_ERROR;
		
		SetCurrentNode(node,nodeAndKey.nodeOffset);

		if (status == B_OK && node->overflow_link == BPLUSTREE_NULL)
		{
			*value = node->Values()[nodeAndKey.keyIndex];
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


//	#pragma mark -


bool
BPlusTree::CheckNode(bplustree_node *node)
{
	if (!fHeader->IsValidLink(node->left_link)
		|| !fHeader->IsValidLink(node->right_link)
		|| !fHeader->IsValidLink(node->overflow_link)
		|| (int8 *)node->Values() + node->all_key_count * sizeof(off_t) > (int8 *)node + fNodeSize)
		return false;

	return true;
}


bplustree_node *BPlusTree::Node(off_t nodeOffset,bool check)
{
/*printf("1: %d,%d,%d\n",
	nodeOffset > fHeader->maximum_size - fNodeSize,
	nodeOffset < 0 && nodeOffset != BPLUSTREE_NULL,
	(nodeOffset % fNodeSize) != 0);
*/
	// the super node is always in memory, and shouldn't
	// never be taken out of the cache
	if (nodeOffset > fHeader->maximum_size /*- fNodeSize*/
		|| nodeOffset <= 0 && nodeOffset != BPLUSTREE_NULL
		|| (nodeOffset % fNodeSize) != 0)
		return NULL;

	bplustree_node *node = (bplustree_node *)malloc(fNodeSize);
	if (node == NULL)
		return NULL;

	if (nodeOffset == BPLUSTREE_NULL || !fStream)
	{
	 	node->left_link = BPLUSTREE_NULL;
	 	node->right_link = BPLUSTREE_NULL;
	 	node->overflow_link = BPLUSTREE_NULL;
	 	node->all_key_count = 0;
	 	node->all_key_length = 0;
	}
	else if (fStream && fStream->ReadAt(nodeOffset,node,fNodeSize) < fNodeSize)
	{
		free(node);
		return NULL;
	}

	if (check && node != NULL)
	{
		// sanity checks (links, all_key_count)
		if (!fHeader->IsValidLink(node->left_link)
			|| !fHeader->IsValidLink(node->right_link)
			|| !fHeader->IsValidLink(node->overflow_link)
			|| (int8 *)node->Values() + node->all_key_count * sizeof(off_t) > (int8 *)node + fNodeSize)
			return NULL;
	}

	if (!fStream && nodeOffset > fHeader->maximum_size - fNodeSize) {
		// do some hacks here
		fHeader->maximum_size += fNodeSize;
	}

	return node;
}


void BPlusTree::SetHoldChanges(bool hold)
{
	fCache.SetHoldChanges(hold);
}


//	#pragma mark -


uint8 *bplustree_node::KeyAt(int32 index,uint16 *keyLength) const
{
	if (index < 0 || index > all_key_count)
		return NULL;

	uint8 *keyStart = Keys();
	uint16 *keyLengths = KeyLengths();

	*keyLength = keyLengths[index] - (index != 0 ? keyLengths[index - 1] : 0);
	if (index > 0)
		keyStart += keyLengths[index - 1];
	
	return keyStart;
}


uint8 bplustree_node::CountDuplicates(off_t offset,bool isFragment) const
{
	// the duplicate fragment handling is currently hard-coded to a node size
	// of 1024 bytes - with future versions of BFS, this may be a problem

	if (isFragment)
	{
		uint32 fragment = 8 * ((uint64)offset & 0x3ff);
	
		return ((off_t *)this)[fragment];
	}
	return overflow_link;
}


off_t bplustree_node::DuplicateAt(off_t offset,bool isFragment,int8 index) const
{
	uint32 start;
	if (isFragment)
		start = 8 * ((uint64)offset & 0x3ff);
	else
		start = 2;

	return ((off_t *)this)[start + 1 + index];
}

