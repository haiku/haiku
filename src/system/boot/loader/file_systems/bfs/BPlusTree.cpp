/*
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 *
 * Roughly based on 'btlib' written by Marcus J. Ranum - it shares
 * no code but achieves binary compatibility with the on disk format.
 */


#include "BPlusTree.h"

#include <boot/platform.h>
#include <util/kernel_cpp.h>
#include <util/Stack.h>

#include <TypeConstants.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace BFS;


// Node Caching for the BPlusTree class
//
// With write support, there is the need for a function that allocates new
// nodes by either returning empty nodes, or by growing the file's data stream
//
// !! The CachedNode class assumes that you have properly locked the stream
// !! before asking for nodes.
//
// Note: This code will fail if the block size is smaller than the node size!
// Since BFS supports block sizes of 1024 bytes or greater, and the node size
// is hard-coded to 1024 bytes, that's not an issue now.


void 
CachedNode::Unset()
{
	if (fTree == NULL || fTree->fStream == NULL)
		return;

	if (fBlock != NULL)
		fNode = NULL;
}


bplustree_node *
CachedNode::SetTo(off_t offset, bool check)
{
	if (fTree == NULL || fTree->fStream == NULL)
		return NULL;

	Unset();

	// You can only ask for nodes at valid positions - you can't
	// even access the b+tree header with this method (use SetToHeader()
	// instead)
	if (offset > fTree->fHeader->MaximumSize() - fTree->fNodeSize
		|| offset <= 0
		|| (offset % fTree->fNodeSize) != 0)
		return NULL;

	if (InternalSetTo(offset) != NULL && check) {
		// sanity checks (links, all_key_count)
		bplustree_header *header = fTree->fHeader;
		if (!header->IsValidLink(fNode->LeftLink())
			|| !header->IsValidLink(fNode->RightLink())
			|| !header->IsValidLink(fNode->OverflowLink())
			|| (int8 *)fNode->Values() + fNode->NumKeys() * sizeof(off_t)
					> (int8 *)fNode + fTree->fNodeSize) {
			dprintf("invalid node read from offset %Ld, inode at %Ld\n",
					offset, fTree->fStream->ID());
			return NULL;
		}
	}

	return fNode;
}


bplustree_header *
CachedNode::SetToHeader()
{
	if (fTree == NULL || fTree->fStream == NULL)
		return NULL;

	Unset();

	InternalSetTo(0LL);
	return (bplustree_header *)fNode;
}


bplustree_node *
CachedNode::InternalSetTo(off_t offset)
{
	fNode = NULL;

	off_t fileOffset;
	block_run run;
	if (offset < fTree->fStream->Size()
		&& fTree->fStream->FindBlockRun(offset, run, fileOffset) == B_OK) {
		Volume &volume = fTree->fStream->GetVolume();

		int32 blockOffset = (offset - fileOffset) / volume.BlockSize();
		fBlockNumber = volume.ToBlock(run) + blockOffset;

		if (fBlock == NULL) {
			fBlock = (uint8 *)malloc(volume.BlockSize());
			if (fBlock == NULL)
				return NULL;
		}
		if (read_pos(volume.Device(), fBlockNumber << volume.BlockShift(),
				fBlock, volume.BlockSize()) < (ssize_t)volume.BlockSize())
			return NULL;

		// the node is somewhere in that block... (confusing offset calculation)
		fNode = (bplustree_node *)(fBlock + offset -
					(fileOffset + (blockOffset << volume.BlockShift())));
	}
	return fNode;
}


//	#pragma mark -


BPlusTree::BPlusTree(Stream *stream)
	:
	fStream(NULL),
	fHeader(NULL),
	fCachedHeader(this)
{
	SetTo(stream);
}


BPlusTree::~BPlusTree()
{
}


status_t
BPlusTree::SetTo(Stream *stream)
{
	if (stream == NULL || stream->InitCheck() != B_OK)
		return fStatus = B_BAD_VALUE;

	// get on-disk B+Tree header

	fCachedHeader.Unset();
	fStream = stream;

	fHeader = fCachedHeader.SetToHeader();
	if (fHeader == NULL)
		return fStatus = B_NO_INIT;

	// is header valid?

	if (fHeader->Magic() != BPLUSTREE_MAGIC
		|| fHeader->MaximumSize() != stream->Size()
		|| (fHeader->RootNode() % fHeader->NodeSize()) != 0
		|| !fHeader->IsValidLink(fHeader->RootNode()))
		return fStatus = B_BAD_DATA;

	fNodeSize = fHeader->NodeSize();

	{
		uint32 toMode[] = {S_STR_INDEX, S_INT_INDEX, S_UINT_INDEX,
			S_LONG_LONG_INDEX, S_ULONG_LONG_INDEX, S_FLOAT_INDEX,
			S_DOUBLE_INDEX};
		uint32 mode = stream->Mode() & (S_STR_INDEX | S_INT_INDEX
			| S_UINT_INDEX | S_LONG_LONG_INDEX | S_ULONG_LONG_INDEX
			| S_FLOAT_INDEX | S_DOUBLE_INDEX);

		if (fHeader->DataType() > BPLUSTREE_DOUBLE_TYPE
			|| ((stream->Mode() & S_INDEX_DIR) != 0
				&& toMode[fHeader->DataType()] != mode)
			|| !stream->IsContainer()) {
			return fStatus = B_BAD_TYPE;
		}

#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
		 // although it's in stat.h, the S_ALLOW_DUPS flag is obviously unused
		 // in the original BFS code - we will honour it nevertheless
		fAllowDuplicates = ((stream->Mode() & S_INDEX_DIR) == S_INDEX_DIR
				&& stream->BlockRun() != stream->Parent())
			|| (stream->Mode() & S_ALLOW_DUPS) != 0;
#endif
	}

	CachedNode cached(this, fHeader->RootNode());

	return fStatus = cached.Node() ? B_OK : B_BAD_DATA;
}


status_t
BPlusTree::InitCheck()
{
	return fStatus;
}


int32 
BPlusTree::TypeCodeToKeyType(type_code code)
{
	switch (code) {
		case B_STRING_TYPE:
			return BPLUSTREE_STRING_TYPE;
		case B_INT32_TYPE:
		case B_SSIZE_T_TYPE:
			return BPLUSTREE_INT32_TYPE;
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
			return BPLUSTREE_UINT32_TYPE;
		case B_OFF_T_TYPE:
		case B_INT64_TYPE:
			return BPLUSTREE_INT64_TYPE;
		case B_UINT64_TYPE:
			return BPLUSTREE_UINT64_TYPE;
		case B_FLOAT_TYPE:
			return BPLUSTREE_FLOAT_TYPE;
		case B_DOUBLE_TYPE:
			return BPLUSTREE_DOUBLE_TYPE;
	}
	return -1;
}


int32 
BPlusTree::ModeToKeyType(mode_t mode)
{
	switch (mode & (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX | S_LONG_LONG_INDEX
				   | S_ULONG_LONG_INDEX | S_FLOAT_INDEX | S_DOUBLE_INDEX)) {
		case S_INT_INDEX:
			return BPLUSTREE_INT32_TYPE;
		case S_UINT_INDEX:
			return BPLUSTREE_UINT32_TYPE;
		case S_LONG_LONG_INDEX:
			return BPLUSTREE_INT64_TYPE;
		case S_ULONG_LONG_INDEX:
			return BPLUSTREE_UINT64_TYPE;
		case S_FLOAT_INDEX:
			return BPLUSTREE_FLOAT_TYPE;
		case S_DOUBLE_INDEX:
			return BPLUSTREE_DOUBLE_TYPE;
		case S_STR_INDEX:
		default:
			// default is for standard directories
			return BPLUSTREE_STRING_TYPE;
	}
}


int32
BPlusTree::CompareKeys(const void *key1, int keyLength1, const void *key2, int keyLength2)
{
	type_code type = 0;
	switch (fHeader->DataType())
	{
	    case BPLUSTREE_STRING_TYPE:
	    	type = B_STRING_TYPE;
	    	break;
		case BPLUSTREE_INT32_TYPE:
	    	type = B_INT32_TYPE;
	    	break;
		case BPLUSTREE_UINT32_TYPE:
	    	type = B_UINT32_TYPE;
	    	break;
		case BPLUSTREE_INT64_TYPE:
	    	type = B_INT64_TYPE;
	    	break;
		case BPLUSTREE_UINT64_TYPE:
	    	type = B_UINT64_TYPE;
	    	break;
		case BPLUSTREE_FLOAT_TYPE:
	    	type = B_FLOAT_TYPE;
	    	break;
		case BPLUSTREE_DOUBLE_TYPE:
	    	type = B_DOUBLE_TYPE;
	    	break;
	}
   	return compareKeys(type, key1, keyLength1, key2, keyLength2);
}


status_t
BPlusTree::FindKey(bplustree_node *node, const uint8 *key, uint16 keyLength, uint16 *index,
	off_t *next)
{
	if (node->all_key_count == 0) {
		if (index)
			*index = 0;
		if (next)
			*next = node->OverflowLink();
		return B_ENTRY_NOT_FOUND;
	}

	off_t *values = node->Values();
	int16 saveIndex = -1;

	// binary search in the key array
	for (int16 first = 0, last = node->NumKeys() - 1; first <= last;) {
		uint16 i = (first + last) >> 1;

		uint16 searchLength;
		uint8 *searchKey = node->KeyAt(i, &searchLength);
		if (searchKey + searchLength + sizeof(off_t) + sizeof(uint16) > (uint8 *)node + fNodeSize
			|| searchLength > BPLUSTREE_MAX_KEY_LENGTH) {
			dprintf("bfs: bad B+tree data\n");
			return B_BAD_DATA;
		}

		int32 cmp = CompareKeys(key, keyLength, searchKey, searchLength);
		if (cmp < 0) {
			last = i - 1;
			saveIndex = i;
		} else if (cmp > 0) {
			saveIndex = first = i + 1;
		} else {
			if (index)
				*index = i;
			if (next)
				*next = BFS_ENDIAN_TO_HOST_INT64(values[i]);
			return B_OK;
		}
	}

	if (index)
		*index = saveIndex;
	if (next) {
		if (saveIndex == node->NumKeys())
			*next = node->OverflowLink();
		else
			*next = BFS_ENDIAN_TO_HOST_INT64(values[saveIndex]);
	}
	return B_ENTRY_NOT_FOUND;
}


/**	Prepares the stack to contain all nodes that were passed while
 *	following the key, from the root node to the leaf node that could
 *	or should contain that key.
 */

status_t
BPlusTree::SeekDown(Stack<node_and_key> &stack, const uint8 *key, uint16 keyLength)
{
	// set the root node to begin with
	node_and_key nodeAndKey;
	nodeAndKey.nodeOffset = fHeader->RootNode();

	CachedNode cached(this);
	bplustree_node *node;
	while ((node = cached.SetTo(nodeAndKey.nodeOffset)) != NULL) {
		// if we are already on leaf level, we're done
		if (node->OverflowLink() == BPLUSTREE_NULL) {
			// node that the keyIndex is not properly set here (but it's not
			// needed in the calling functions anyway)!
			nodeAndKey.keyIndex = 0;
			stack.Push(nodeAndKey);
			return B_OK;
		}

		off_t nextOffset;
		status_t status = FindKey(node, key, keyLength, &nodeAndKey.keyIndex, &nextOffset);
		
		if (status == B_ENTRY_NOT_FOUND && nextOffset == nodeAndKey.nodeOffset)
			return B_ERROR;

		// put the node offset & the correct keyIndex on the stack
		stack.Push(nodeAndKey);

		nodeAndKey.nodeOffset = nextOffset;
	}
	return B_ERROR;
}


#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
status_t
BPlusTree::FindFreeDuplicateFragment(bplustree_node *node, CachedNode *cached, off_t *_offset,
	bplustree_node **_fragment, uint32 *_index)
{
	off_t *values = node->Values();
	for (int32 i = 0;i < node->all_key_count;i++) {
		// does the value link to a duplicate fragment?
		if (bplustree_node::LinkType(values[i]) != BPLUSTREE_DUPLICATE_FRAGMENT)
			continue;

		bplustree_node *fragment = cached->SetTo(bplustree_node::FragmentOffset(
			BFS_ENDIAN_TO_HOST_INT64(values[i])), false);
		if (fragment == NULL) {
			FATAL(("Could not get duplicate fragment at %Ld\n",values[i]));
			continue;
		}
		
		// see if there is some space left for us
		int32 num = (fNodeSize >> 3) / (NUM_FRAGMENT_VALUES + 1);
		for (int32 j = 0;j < num;j++) {
			duplicate_array *array = fragment->FragmentAt(j);

			if (array->count == 0) {
				*_offset = bplustree_node::FragmentOffset(
					BFS_ENDIAN_TO_HOST_INT64(values[i]));
				*_fragment = fragment;
				*_index = j;
				return B_OK;
			}
		}
	}
	return B_ENTRY_NOT_FOUND;
}
#endif

/**	Searches the key in the tree, and stores the offset found in
 *	_value, if successful.
 *	It's very similar to BPlusTree::SeekDown(), but doesn't fill
 *	a stack while it descends the tree.
 *	Returns B_OK when the key could be found, B_ENTRY_NOT_FOUND
 *	if not. It can also return other errors to indicate that
 *	something went wrong.
 *	Note that this doesn't work with duplicates - it will just
 *	return B_BAD_TYPE if you call this function on a tree where
 *	duplicates are allowed.
 */

status_t
BPlusTree::Find(const uint8 *key, uint16 keyLength, off_t *_value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH || keyLength > BPLUSTREE_MAX_KEY_LENGTH
		|| key == NULL)
		return B_BAD_VALUE;

#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
	if (fAllowDuplicates)
		return B_BAD_TYPE;
#endif

	off_t nodeOffset = fHeader->RootNode();
	CachedNode cached(this);
	bplustree_node *node;

	while ((node = cached.SetTo(nodeOffset)) != NULL) {
		uint16 keyIndex = 0;
		off_t nextOffset;
		status_t status = FindKey(node, key, keyLength, &keyIndex, &nextOffset);

		if (node->OverflowLink() == BPLUSTREE_NULL) {
			if (status == B_OK && _value != NULL)
				*_value = BFS_ENDIAN_TO_HOST_INT64(node->Values()[keyIndex]);

			return status;
		} else if (nextOffset == nodeOffset)
			return B_ERROR;

		nodeOffset = nextOffset;
	}
	return B_ERROR;
}


//	#pragma mark -


TreeIterator::TreeIterator(BPlusTree *tree)
	:
	fTree(tree),
	fCurrentNodeOffset(BPLUSTREE_NULL)
{
}


TreeIterator::~TreeIterator()
{
}


status_t
TreeIterator::Goto(int8 to)
{
	if (fTree == NULL || fTree->fHeader == NULL)
		return B_BAD_VALUE;

	off_t nodeOffset = fTree->fHeader->RootNode();
	CachedNode cached(fTree);
	bplustree_node *node;

	while ((node = cached.SetTo(nodeOffset)) != NULL) {
		// is the node a leaf node?
		if (node->OverflowLink() == BPLUSTREE_NULL) {
			fCurrentNodeOffset = nodeOffset;
			fCurrentKey = to == BPLUSTREE_BEGIN ? -1 : node->NumKeys();
#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
			fDuplicateNode = BPLUSTREE_NULL;
#endif
			return B_OK;
		}

		// get the next node offset depending on the direction (and if there
		// are any keys in that node at all)
		off_t nextOffset;
		if (to == BPLUSTREE_END || node->all_key_count == 0)
			nextOffset = node->OverflowLink();
		else {
			if (node->AllKeyLength() > fTree->fNodeSize
				|| (uint32)node->Values() > (uint32)node + fTree->fNodeSize - 8 * node->NumKeys())
				return B_ERROR;

			nextOffset = BFS_ENDIAN_TO_HOST_INT64(node->Values()[0]);
		}
		if (nextOffset == nodeOffset)
			break;

		nodeOffset = nextOffset;
	}
	return B_ERROR;
}


/**	Iterates through the tree in the specified direction.
 *	When it iterates through duplicates, the "key" is only updated for the
 *	first entry - if you need to know when this happens, use the "duplicate"
 *	parameter which is 0 for no duplicate, 1 for the first, and 2 for all
 *	the other duplicates.
 *	That's not too nice, but saves the 256 bytes that would be needed to
 *	store the last key - if this will ever become an issue, it will be
 *	easy to change.
 *	The other advantage of this is, that the queries can skip all duplicates
 *	at once when they are not relevant to them.
 */

status_t
TreeIterator::Traverse(int8 direction, void *key, uint16 *keyLength, uint16 maxLength,
	off_t *value, uint16 *duplicate)
{
	if (fTree == NULL)
		return B_INTERRUPTED;
	if (fCurrentNodeOffset == BPLUSTREE_NULL
		&& Goto(direction == BPLUSTREE_FORWARD ? BPLUSTREE_BEGIN : BPLUSTREE_END) < B_OK) 
		return B_ERROR;

	// if the tree was emptied since the last call
	if (fCurrentNodeOffset == BPLUSTREE_FREE)
		return B_ENTRY_NOT_FOUND;

	CachedNode cached(fTree);
	bplustree_node *node;

#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
	if (fDuplicateNode != BPLUSTREE_NULL) {
		// regardless of traverse direction the duplicates are always presented
		// in the same order; since they are all considered as equal, this
		// shouldn't cause any problems

		if (!fIsFragment || fDuplicate < fNumDuplicates) {
			node = cached.SetTo(bplustree_node::FragmentOffset(fDuplicateNode),
				false);
		} else
			node = NULL;

		if (node != NULL) {
			if (!fIsFragment && fDuplicate >= fNumDuplicates) {
				// if the node is out of duplicates, we go directly to the next
				// one
				fDuplicateNode = node->right_link;
				if (fDuplicateNode != BPLUSTREE_NULL
					&& (node = cached.SetTo(fDuplicateNode, false)) != NULL) {
					fNumDuplicates
						= node->CountDuplicates(fDuplicateNode, false);
					fDuplicate = 0;
				}
			}
			if (fDuplicate < fNumDuplicates) {
				*value = node->DuplicateAt(fDuplicateNode, fIsFragment,
					fDuplicate++);
				if (duplicate)
					*duplicate = 2;
				return B_OK;
			}
		}
		fDuplicateNode = BPLUSTREE_NULL;
	}
#endif	/* BPLUSTREE_SUPPORTS_DUPLICATES */

	off_t savedNodeOffset = fCurrentNodeOffset;
	if ((node = cached.SetTo(fCurrentNodeOffset)) == NULL)
		return B_ERROR;

#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
	if (duplicate)
		*duplicate = 0;
#endif

	fCurrentKey += direction;
	
	// is the current key in the current node?
	while ((direction == BPLUSTREE_FORWARD && fCurrentKey >= node->NumKeys())
		   || (direction == BPLUSTREE_BACKWARD && fCurrentKey < 0))
	{
		fCurrentNodeOffset = direction == BPLUSTREE_FORWARD ? node->RightLink() : node->LeftLink();

		// are there any more nodes?
		if (fCurrentNodeOffset != BPLUSTREE_NULL)
		{
			node = cached.SetTo(fCurrentNodeOffset);
			if (!node)
				return B_ERROR;

			// reset current key
			fCurrentKey = direction == BPLUSTREE_FORWARD ? 0 : node->NumKeys();
		}
		else
		{
			// there are no nodes left, so turn back to the last key
			fCurrentNodeOffset = savedNodeOffset;
			fCurrentKey = direction == BPLUSTREE_FORWARD ? node->NumKeys() : -1;

			return B_ENTRY_NOT_FOUND;
		}
	}

	if (node->all_key_count == 0)
		return B_ERROR;	// B_ENTRY_NOT_FOUND ?

	uint16 length;
	uint8 *keyStart = node->KeyAt(fCurrentKey, &length);
	if (keyStart + length + sizeof(off_t) + sizeof(uint16) > (uint8 *)node + fTree->fNodeSize
		|| length > BPLUSTREE_MAX_KEY_LENGTH) {
		dprintf("bfs: bad b+tree data\n");
		return B_BAD_DATA;
	}

	length = min_c(length, maxLength);
	memcpy(key, keyStart, length);

	if (fTree->fHeader->DataType() == BPLUSTREE_STRING_TYPE)	// terminate string type
	{
		if (length == maxLength)
			length--;
		((char *)key)[length] = '\0';
	}
	*keyLength = length;

	off_t offset = BFS_ENDIAN_TO_HOST_INT64(node->Values()[fCurrentKey]);

#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
	// duplicate fragments?
	uint8 type = bplustree_node::LinkType(offset);
	if (type == BPLUSTREE_DUPLICATE_FRAGMENT || type == BPLUSTREE_DUPLICATE_NODE)
	{
		fDuplicateNode = offset;

		node = cached.SetTo(bplustree_node::FragmentOffset(fDuplicateNode), false);
		if (node == NULL)
			return B_ERROR;

		fIsFragment = type == BPLUSTREE_DUPLICATE_FRAGMENT;

		fNumDuplicates = node->CountDuplicates(offset, fIsFragment);
		if (fNumDuplicates)
		{
			offset = node->DuplicateAt(offset, fIsFragment, 0);
			fDuplicate = 1;
			if (duplicate)
				*duplicate = 1;
		}
		else
		{
			// shouldn't happen, but we're dealing here with potentially corrupt disks...
			fDuplicateNode = BPLUSTREE_NULL;
			offset = 0;
		}
	}
#endif	/* BPLUSTREE_SUPPORTS_DUPLICATES */
	*value = offset;

	return B_OK;
}


/**	This is more or less a copy of BPlusTree::Find() - but it just
 *	sets the current position in the iterator, regardless of if the
 *	key could be found or not.
 */

status_t 
TreeIterator::Find(const uint8 *key, uint16 keyLength)
{
	if (fTree == NULL)
		return B_INTERRUPTED;
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH || keyLength > BPLUSTREE_MAX_KEY_LENGTH
		|| key == NULL)
		return B_BAD_VALUE;

	off_t nodeOffset = fTree->fHeader->RootNode();

	CachedNode cached(fTree);
	bplustree_node *node;
	while ((node = cached.SetTo(nodeOffset)) != NULL) {
		uint16 keyIndex = 0;
		off_t nextOffset;
		status_t status = fTree->FindKey(node, key, keyLength, &keyIndex, &nextOffset);

		if (node->OverflowLink() == BPLUSTREE_NULL) {
			fCurrentNodeOffset = nodeOffset;
			fCurrentKey = keyIndex - 1;
#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
			fDuplicateNode = BPLUSTREE_NULL;
#endif
			return status;
		} else if (nextOffset == nodeOffset)
			return B_ERROR;

		nodeOffset = nextOffset;
	}
	return B_ERROR;
}


#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
void
TreeIterator::SkipDuplicates()
{
	fDuplicateNode = BPLUSTREE_NULL;
}
#endif


//	#pragma mark -


void 
bplustree_node::Initialize()
{
	left_link = right_link = overflow_link = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
	all_key_count = 0;
	all_key_length = 0;
}


uint8 *
bplustree_node::KeyAt(int32 index, uint16 *keyLength) const
{
	if (index < 0 || index > NumKeys())
		return NULL;

	uint8 *keyStart = Keys();
	uint16 *keyLengths = KeyLengths();

	*keyLength = BFS_ENDIAN_TO_HOST_INT16(keyLengths[index])
		- (index != 0 ? BFS_ENDIAN_TO_HOST_INT16(keyLengths[index - 1]) : 0);
	if (index > 0)
		keyStart += BFS_ENDIAN_TO_HOST_INT16(keyLengths[index - 1]);

	return keyStart;
}


#ifdef BPLUSTREE_SUPPORTS_DUPLICATES
uint8
bplustree_node::CountDuplicates(off_t offset, bool isFragment) const
{
	// the duplicate fragment handling is currently hard-coded to a node size
	// of 1024 bytes - with future versions of BFS, this may be a problem

	if (isFragment) {
		uint32 fragment = (NUM_FRAGMENT_VALUES + 1) * ((uint64)offset & 0x3ff);

		return ((off_t *)this)[fragment];
	}
	return OverflowLink();
}


off_t
bplustree_node::DuplicateAt(off_t offset, bool isFragment, int8 index) const
{
	uint32 start;
	if (isFragment)
		start = 8 * ((uint64)offset & 0x3ff);
	else
		start = 2;

	return ((off_t *)this)[start + 1 + index];
}


/**	Although the name suggests it, this function doesn't return the real
 *	used fragment count; at least, it can only count to two: it returns
 *	0, if there is no fragment used, 1 if there is only one fragment
 *	used, and 2 if there are at least 2 fragments used.
 */

int32
bplustree_node::FragmentsUsed(uint32 nodeSize)
{
	uint32 used = 0;
	for (uint32 i = 0; i < nodeSize / ((NUM_FRAGMENT_VALUES + 1) * sizeof(off_t)); i++) {
		duplicate_array *array = FragmentAt(i);
		if (array->count > 0 && ++used > 1)
			return used;
	}
	return used;
}
#endif	/* BPLUSTREE_SUPPORTS_DUPLICATES */


//	#pragma mark -


int32
BFS::compareKeys(type_code type, const void *key1, int keyLength1, const void *key2, int keyLength2)
{
	// if one of the keys is NULL, bail out gracefully
	if (key1 == NULL || key2 == NULL)
		return -1;

	switch (type)
	{
	    case B_STRING_TYPE:
    	{
			int len = min_c(keyLength1, keyLength2);
			int result = strncmp((const char *)key1, (const char *)key2, len);

			if (result == 0
				&& !(((const char *)key1)[len] == '\0' && ((const char *)key2)[len] == '\0'))
				result = keyLength1 - keyLength2;

			return result;
		}

		case B_SSIZE_T_TYPE:
		case B_INT32_TYPE:
			return *(int32 *)key1 - *(int32 *)key2;

		case B_SIZE_T_TYPE:
		case B_UINT32_TYPE:
			if (*(uint32 *)key1 == *(uint32 *)key2)
				return 0;
			else if (*(uint32 *)key1 > *(uint32 *)key2)
				return 1;

			return -1;

		case B_OFF_T_TYPE:
		case B_INT64_TYPE:
			if (*(int64 *)key1 == *(int64 *)key2)
				return 0;
			else if (*(int64 *)key1 > *(int64 *)key2)
				return 1;

			return -1;

		case B_UINT64_TYPE:
			if (*(uint64 *)key1 == *(uint64 *)key2)
				return 0;
			else if (*(uint64 *)key1 > *(uint64 *)key2)
				return 1;

			return -1;

		case B_FLOAT_TYPE:
		{
			float result = *(float *)key1 - *(float *)key2;
			if (result == 0.0f)
				return 0;

			return (result < 0.0f) ? -1 : 1;
		}

		case B_DOUBLE_TYPE:
		{
			double result = *(double *)key1 - *(double *)key2;
			if (result == 0.0)
				return 0;

			return (result < 0.0) ? -1 : 1;
		}
	}

	// if the type is unknown, the entries don't match...
	return -1;
}

