/* BPlusTree - BFS B+Tree implementation
 *
 * Roughly based on 'btlib' written by Marcus J. Ranum - it shares
 * no code but achieves binary compatibility with the on disk format.
 *
 * Copyright 2001-2006, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Debug.h"
#include "BPlusTree.h"
#include "Inode.h"
#include "Utility.h"
#include "Stack.h"

#include <util/kernel_cpp.h>
#include <TypeConstants.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#ifdef DEBUG
class NodeChecker {
	public:
		NodeChecker(bplustree_node *node, int32 nodeSize)
			:
			fNode(node),
			fSize(nodeSize)
		{
		}

		~NodeChecker()
		{
			fNode->CheckIntegrity(fSize);
		}

	private:
		bplustree_node *fNode;
		int32	fSize;
};
#endif

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

	if (fBlock != NULL) {
		release_block(fTree->fStream->GetVolume()->Device(), fBlockNumber);
	
		fBlock = NULL;
		fNode = NULL;
	}
}


bplustree_node *
CachedNode::SetTo(off_t offset, bool check)
{
	if (fTree == NULL || fTree->fStream == NULL) {
		REPORT_ERROR(B_BAD_VALUE);
		return NULL;
	}

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
			|| (int8 *)fNode->Values() + fNode->NumKeys() * sizeof(off_t) >
					(int8 *)fNode + fTree->fNodeSize) {
			FATAL(("invalid node read from offset %Ld, inode at %Ld\n",
					offset, fTree->fStream->ID()));
			return NULL;
		}
	}
	return fNode;
}


bplustree_header *
CachedNode::SetToHeader()
{
	if (fTree == NULL || fTree->fStream == NULL) {
		REPORT_ERROR(B_BAD_VALUE);
		return NULL;
	}

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
		Volume *volume = fTree->fStream->GetVolume();

		int32 blockOffset = (offset - fileOffset) / volume->BlockSize();
		fBlockNumber = volume->ToBlock(run) + blockOffset;

		fBlock = (uint8 *)get_block(volume->Device(), fBlockNumber, volume->BlockSize());
		if (fBlock) {
			// the node is somewhere in that block... (confusing offset calculation)
			fNode = (bplustree_node *)(fBlock + offset -
						(fileOffset + blockOffset * volume->BlockSize()));
		} else
			REPORT_ERROR(B_IO_ERROR);
	}
	return fNode;
}


status_t
CachedNode::Free(Transaction *transaction, off_t offset)
{
	if (transaction == NULL || fTree == NULL || fTree->fStream == NULL
		|| offset == BPLUSTREE_NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// ToDo: scan the free nodes list and remove all nodes at the end
	// of the tree - perhaps that shouldn't be done everytime that
	// function is called, perhaps it should be done when the directory
	// inode is closed or based on some calculation or whatever...

	// if the node is the last one in the tree, we shrink
	// the tree and file size by one node
	off_t lastOffset = fTree->fHeader->MaximumSize() - fTree->fNodeSize;
	if (offset == lastOffset) {
		fTree->fHeader->maximum_size = HOST_ENDIAN_TO_BFS_INT64(lastOffset);

		status_t status = fTree->fStream->SetFileSize(transaction, lastOffset);
		if (status < B_OK)
			return status;

		return fTree->fCachedHeader.WriteBack(transaction);
	}

	// add the node to the free nodes list
	fNode->left_link = fTree->fHeader->free_node_pointer;
	fNode->overflow_link = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_FREE);

	if (WriteBack(transaction) == B_OK) {
		fTree->fHeader->free_node_pointer = HOST_ENDIAN_TO_BFS_INT64(offset);
		return fTree->fCachedHeader.WriteBack(transaction);
	}
	return B_ERROR;
}


status_t
CachedNode::Allocate(Transaction *transaction, bplustree_node **_node, off_t *_offset)
{
	if (transaction == NULL || fTree == NULL || fTree->fHeader == NULL
		|| fTree->fStream == NULL) {
		RETURN_ERROR(B_BAD_VALUE);
	}

	status_t status;

	// if there are any free nodes, recycle them
	if (SetTo(fTree->fHeader->FreeNode(), false) != NULL) {
		*_offset = fTree->fHeader->FreeNode();
		
		// set new free node pointer
		fTree->fHeader->free_node_pointer = fNode->left_link;
		if ((status = fTree->fCachedHeader.WriteBack(transaction)) == B_OK) {
			fNode->Initialize();
			*_node = fNode;
			return B_OK;
		}
		return status;
	}
	// allocate space for a new node
	Inode *stream = fTree->fStream;
	if ((status = stream->Append(transaction, fTree->fNodeSize)) < B_OK)
		return status;

	// the maximum_size has to be changed before the call to SetTo() - or
	// else it will fail because the requested node is out of bounds
	off_t offset = fTree->fHeader->MaximumSize();
	fTree->fHeader->maximum_size = HOST_ENDIAN_TO_BFS_INT64(fTree->fHeader->MaximumSize() + fTree->fNodeSize);

	if (SetTo(offset, false) != NULL) {
		*_offset = offset;

		if (fTree->fCachedHeader.WriteBack(transaction) >= B_OK) {
			fNode->Initialize();
			*_node = fNode;
			return B_OK;
		}
	}
	RETURN_ERROR(B_ERROR);
}


status_t 
CachedNode::WriteBack(Transaction *transaction)
{
	if (transaction == NULL || fTree == NULL || fTree->fStream == NULL || fNode == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	return transaction->WriteBlocks(fBlockNumber, fBlock);
}


//	#pragma mark -


BPlusTree::BPlusTree(Transaction *transaction, Inode *stream, int32 nodeSize)
	:
	fStream(NULL),
	fHeader(NULL),
	fCachedHeader(this)
{
	SetTo(transaction, stream);
}


BPlusTree::BPlusTree(Inode *stream)
	:
	fStream(NULL),
	fHeader(NULL),
	fCachedHeader(this)
{
	SetTo(stream);
}


BPlusTree::BPlusTree()
	:
	fStream(NULL),
	fHeader(NULL),
	fCachedHeader(this),
	fNodeSize(BPLUSTREE_NODE_SIZE),
	fAllowDuplicates(true),
	fStatus(B_NO_INIT)
{
}


BPlusTree::~BPlusTree()
{
	// if there are any TreeIterators left, we need to stop them
	// (can happen when the tree's inode gets deleted while
	// traversing the tree - a TreeIterator doesn't lock the inode)
	if (fIteratorLock.Lock() < B_OK)
		return;

	TreeIterator *iterator = NULL;
	while ((iterator = fIterators.Next(iterator)) != NULL)
		iterator->Stop();

	fIteratorLock.Unlock();
}


/** Create a new B+Tree on the specified stream */

status_t
BPlusTree::SetTo(Transaction *transaction, Inode *stream, int32 nodeSize)
{
	// initializes in-memory B+Tree

	fCachedHeader.Unset();
	fStream = stream;

	fHeader = fCachedHeader.SetToHeader();
	if (fHeader == NULL) {
		// allocate space for new header + node!
		fStatus = stream->SetFileSize(transaction, nodeSize * 2);
		if (fStatus < B_OK)
			RETURN_ERROR(fStatus);
		
		fHeader = fCachedHeader.SetToHeader();
		if (fHeader == NULL)
			RETURN_ERROR(fStatus = B_ERROR);
	}

	fAllowDuplicates = ((stream->Mode() & S_INDEX_DIR) == S_INDEX_DIR
						&& stream->BlockRun() != stream->Parent())
						|| (stream->Mode() & S_ALLOW_DUPS) != 0;

	fNodeSize = nodeSize;

	// initialize b+tree header
 	fHeader->magic = HOST_ENDIAN_TO_BFS_INT32(BPLUSTREE_MAGIC);
 	fHeader->node_size = HOST_ENDIAN_TO_BFS_INT32(fNodeSize);
 	fHeader->max_number_of_levels = HOST_ENDIAN_TO_BFS_INT32(1);
 	fHeader->data_type = HOST_ENDIAN_TO_BFS_INT32(ModeToKeyType(stream->Mode()));
 	fHeader->root_node_pointer = HOST_ENDIAN_TO_BFS_INT64(nodeSize);
 	fHeader->free_node_pointer = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
 	fHeader->maximum_size = HOST_ENDIAN_TO_BFS_INT64(nodeSize * 2);

	if (fCachedHeader.WriteBack(transaction) < B_OK)
		RETURN_ERROR(fStatus = B_ERROR);

	// initialize b+tree root node
	CachedNode cached(this, fHeader->RootNode(), false);
	if (cached.Node() == NULL)
		RETURN_ERROR(B_ERROR);

	cached.Node()->Initialize();
	return fStatus = cached.WriteBack(transaction);
}


status_t
BPlusTree::SetTo(Inode *stream)
{
	if (stream == NULL || stream->Node() == NULL)
		RETURN_ERROR(fStatus = B_BAD_VALUE);

	// get on-disk B+Tree header

	fCachedHeader.Unset();
	fStream = stream;

	fHeader = fCachedHeader.SetToHeader();
	if (fHeader == NULL)
		RETURN_ERROR(fStatus = B_NO_INIT);
	
	// is header valid?

	if (fHeader->Magic() != BPLUSTREE_MAGIC
		|| fHeader->MaximumSize() != stream->Size()
		|| (fHeader->RootNode() % fHeader->NodeSize()) != 0
		|| !fHeader->IsValidLink(fHeader->RootNode())
		|| !fHeader->IsValidLink(fHeader->FreeNode()))
		RETURN_ERROR(fStatus = B_BAD_DATA);

	fNodeSize = fHeader->NodeSize();

	{
		uint32 toMode[] = {S_STR_INDEX, S_INT_INDEX, S_UINT_INDEX, S_LONG_LONG_INDEX,
						   S_ULONG_LONG_INDEX, S_FLOAT_INDEX, S_DOUBLE_INDEX};
		uint32 mode = stream->Mode() & (S_STR_INDEX | S_INT_INDEX | S_UINT_INDEX | S_LONG_LONG_INDEX
						   | S_ULONG_LONG_INDEX | S_FLOAT_INDEX | S_DOUBLE_INDEX);
	
		if (fHeader->DataType() > BPLUSTREE_DOUBLE_TYPE
			|| (stream->Mode() & S_INDEX_DIR) && toMode[fHeader->DataType()] != mode
			|| !stream->IsContainer()) {
			D(	dump_bplustree_header(fHeader);
				dump_inode(stream->Node());
			);
			RETURN_ERROR(fStatus = B_BAD_TYPE);
		}

		// although it's in stat.h, the S_ALLOW_DUPS flag is obviously unused
		// in the original BFS code - we will honour it nevertheless
		fAllowDuplicates = ((stream->Mode() & S_INDEX_DIR) == S_INDEX_DIR
							&& stream->BlockRun() != stream->Parent())
							|| (stream->Mode() & S_ALLOW_DUPS) != 0;
	}

	CachedNode cached(this, fHeader->RootNode());
	RETURN_ERROR(fStatus = cached.Node() ? B_OK : B_BAD_DATA);
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
		case B_SSIZE_T_TYPE:
		case B_INT32_TYPE:
			return BPLUSTREE_INT32_TYPE;
		case B_SIZE_T_TYPE:
		case B_UINT32_TYPE:
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


//	#pragma mark -


void
BPlusTree::UpdateIterators(off_t offset, off_t nextOffset, uint16 keyIndex, uint16 splitAt,
	int8 change)
{
	// Although every iterator which is affected by this update currently
	// waits on a semaphore, other iterators could be added/removed at
	// any time, so we need to protect this loop
	if (fIteratorLock.Lock() < B_OK)
		return;

	TreeIterator *iterator = NULL;
	while ((iterator = fIterators.Next(iterator)) != NULL)
		iterator->Update(offset, nextOffset, keyIndex, splitAt, change);

	fIteratorLock.Unlock();
}


void
BPlusTree::AddIterator(TreeIterator *iterator)
{
	if (fIteratorLock.Lock() < B_OK)
		return;

	fIterators.Add(iterator);

	fIteratorLock.Unlock();
}


void 
BPlusTree::RemoveIterator(TreeIterator *iterator)
{
	if (fIteratorLock.Lock() < B_OK)
		return;

	fIterators.Remove(iterator);

	fIteratorLock.Unlock();
}


int32
BPlusTree::CompareKeys(const void *key1, int keyLength1, const void *key2, int keyLength2)
{
	type_code type = 0;
	switch (fHeader->data_type) {
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
			fStream->GetVolume()->Panic();
			RETURN_ERROR(B_BAD_DATA);
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
			RETURN_ERROR(B_ERROR);

		// put the node offset & the correct keyIndex on the stack
		stack.Push(nodeAndKey);

		nodeAndKey.nodeOffset = nextOffset;
	}

	RETURN_ERROR(B_ERROR);
}


status_t
BPlusTree::FindFreeDuplicateFragment(bplustree_node *node, CachedNode *cached, off_t *_offset,
	bplustree_node **_fragment, uint32 *_index)
{
	off_t *values = node->Values();
	for (int32 i = 0; i < node->NumKeys(); i++) {
		off_t value = BFS_ENDIAN_TO_HOST_INT64(values[i]);

		// does the value link to a duplicate fragment?
		if (bplustree_node::LinkType(value) != BPLUSTREE_DUPLICATE_FRAGMENT)
			continue;

		bplustree_node *fragment = cached->SetTo(bplustree_node::FragmentOffset(value), false);
		if (fragment == NULL) {
			FATAL(("Could not get duplicate fragment at %Ld\n", value));
			continue;
		}

		// see if there is some space left for us
		int32 num = (fNodeSize >> 3) / (NUM_FRAGMENT_VALUES + 1);
		for (int32 j = 0;j < num;j++) {
			duplicate_array *array = fragment->FragmentAt(j);

			if (array->count == 0) {
				*_offset = bplustree_node::FragmentOffset(value);
				*_fragment = fragment;
				*_index = j;
				return B_OK;
			}
		}
	}
	return B_ENTRY_NOT_FOUND;
}


status_t
BPlusTree::InsertDuplicate(Transaction *transaction, CachedNode *cached, bplustree_node *node,
	uint16 index, off_t value)
{
	CachedNode cachedDuplicate(this);
	off_t *values = node->Values();
	off_t oldValue = BFS_ENDIAN_TO_HOST_INT64(values[index]);
	status_t status;
	off_t offset;

	if (bplustree_node::IsDuplicate(oldValue)) {
		// If it's a duplicate fragment, try to insert it into that, or if it
		// doesn't fit anymore, create a new duplicate node

		if (bplustree_node::LinkType(oldValue) == BPLUSTREE_DUPLICATE_FRAGMENT) {
			bplustree_node *duplicate = cachedDuplicate.SetTo(
				bplustree_node::FragmentOffset(oldValue), false);
			if (duplicate == NULL)
				return B_IO_ERROR;

			duplicate_array *array = duplicate->FragmentAt(bplustree_node::FragmentIndex(oldValue));
			if (array->CountItems() > NUM_FRAGMENT_VALUES
				|| array->CountItems() < 1) {
				FATAL(("insertDuplicate: Invalid array[%ld] size in fragment %Ld == %Ld!\n",
					bplustree_node::FragmentIndex(oldValue),
					bplustree_node::FragmentOffset(oldValue),
					array->CountItems()));
				return B_BAD_DATA;
			}

			if (array->CountItems() < NUM_FRAGMENT_VALUES) {
				array->Insert(value);
			} else {
				// test if the fragment will be empty if we remove this key's values			
				if (duplicate->FragmentsUsed(fNodeSize) < 2) {
					// the node will be empty without our values, so let us
					// reuse it as a duplicate node
					offset = bplustree_node::FragmentOffset(oldValue);

					memmove(duplicate->DuplicateArray(), array, (NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));
					duplicate->left_link = duplicate->right_link = HOST_ENDIAN_TO_BFS_INT64(
						(uint64)BPLUSTREE_NULL);

					array = duplicate->DuplicateArray();
					array->Insert(value);
				} else {
					// create a new duplicate node
					CachedNode cachedNewDuplicate(this);
					bplustree_node *newDuplicate;
					status = cachedNewDuplicate.Allocate(transaction, &newDuplicate, &offset);
					if (status < B_OK)
						return status;

					// copy the array from the fragment node to the duplicate node
					// and free the old entry (by zero'ing all values)
					newDuplicate->overflow_link = array->count;
					memcpy(&newDuplicate->all_key_count, &array->values[0],
						array->CountItems() * sizeof(off_t));
					memset(array, 0, (NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));

					array = newDuplicate->DuplicateArray();
					array->Insert(value);

					// if this fails, the old fragments node will contain wrong
					// data... (but since it couldn't be written, it shouldn't
					// be fatal)
					if ((status = cachedNewDuplicate.WriteBack(transaction)) < B_OK)
						return status;
				}

				// update the main pointer to link to a duplicate node
				values[index] = HOST_ENDIAN_TO_BFS_INT64(bplustree_node::MakeLink(
					BPLUSTREE_DUPLICATE_NODE, offset));
				if ((status = cached->WriteBack(transaction)) < B_OK)
					return status;
			}

			return cachedDuplicate.WriteBack(transaction);
		}

		// Put the value into a dedicated duplicate node

		// search for free space in the duplicate nodes of that key
		duplicate_array *array;
		bplustree_node *duplicate;
		off_t duplicateOffset;
		do {
			duplicateOffset = bplustree_node::FragmentOffset(oldValue);
			duplicate = cachedDuplicate.SetTo(duplicateOffset, false);
			if (duplicate == NULL)
				return B_IO_ERROR;

			array = duplicate->DuplicateArray();
			if (array->CountItems() > NUM_DUPLICATE_VALUES || array->CountItems() < 0) {
				FATAL(("removeDuplicate: Invalid array size in duplicate %Ld == %Ld!\n",
					duplicateOffset, array->CountItems()));
				return B_BAD_DATA;
			}
		} while (array->CountItems() >= NUM_DUPLICATE_VALUES
				&& (oldValue = duplicate->RightLink()) != BPLUSTREE_NULL);

		if (array->CountItems() < NUM_DUPLICATE_VALUES) {
			array->Insert(value);
		} else {
			// no space left - add a new duplicate node

			CachedNode cachedNewDuplicate(this);
			bplustree_node *newDuplicate;
			status = cachedNewDuplicate.Allocate(transaction, &newDuplicate, &offset);
			if (status < B_OK)
				return status;

			// link the two nodes together
			duplicate->right_link = HOST_ENDIAN_TO_BFS_INT64(offset);
			newDuplicate->left_link = HOST_ENDIAN_TO_BFS_INT64(duplicateOffset);

			array = newDuplicate->DuplicateArray();
			array->count = 0;
			array->Insert(value);

			status = cachedNewDuplicate.WriteBack(transaction);
			if (status < B_OK)
				return status;
		}
		return cachedDuplicate.WriteBack(transaction);
	}

	// Search for a free duplicate fragment or create a new one
	// to insert the duplicate value into

	uint32 fragmentIndex = 0;
	bplustree_node *fragment;
	if (FindFreeDuplicateFragment(node, &cachedDuplicate, &offset, &fragment, &fragmentIndex) < B_OK) {
		// allocate a new duplicate fragment node
		if ((status = cachedDuplicate.Allocate(transaction, &fragment, &offset)) < B_OK)
			return status;

		memset(fragment, 0, fNodeSize);
	}
	duplicate_array *array = fragment->FragmentAt(fragmentIndex);
	array->Insert(oldValue);
	array->Insert(value);

	if ((status = cachedDuplicate.WriteBack(transaction)) < B_OK)
		return status;

	values[index] = HOST_ENDIAN_TO_BFS_INT64(bplustree_node::MakeLink(
		BPLUSTREE_DUPLICATE_FRAGMENT, offset, fragmentIndex));

	return cached->WriteBack(transaction);
}


void
BPlusTree::InsertKey(bplustree_node *node, uint16 index, uint8 *key, uint16 keyLength,
	off_t value)
{
	// should never happen, but who knows?
	if (index > node->NumKeys())
		return;

	off_t *values = node->Values();
	uint16 *keyLengths = node->KeyLengths();
	uint8 *keys = node->Keys();

	node->all_key_count = HOST_ENDIAN_TO_BFS_INT16(node->NumKeys() + 1);
	node->all_key_length = HOST_ENDIAN_TO_BFS_INT16(node->AllKeyLength() + keyLength);

	off_t *newValues = node->Values();
	uint16 *newKeyLengths = node->KeyLengths();

	// move values and copy new value into them
	memmove(newValues + index + 1, values + index, sizeof(off_t) * (node->NumKeys() - 1 - index));
	memmove(newValues, values, sizeof(off_t) * index);

	newValues[index] = HOST_ENDIAN_TO_BFS_INT64(value);

	// move and update key length index
	for (uint16 i = node->NumKeys(); i-- > index + 1;)
		newKeyLengths[i] = HOST_ENDIAN_TO_BFS_INT16(BFS_ENDIAN_TO_HOST_INT16(keyLengths[i - 1]) + keyLength);
	memmove(newKeyLengths, keyLengths, sizeof(uint16) * index);

	int32 keyStart;
	newKeyLengths[index] = HOST_ENDIAN_TO_BFS_INT16(keyLength + (keyStart = index > 0 ? BFS_ENDIAN_TO_HOST_INT16(newKeyLengths[index - 1]) : 0));

	// move keys and copy new key into them
	uint16 length = BFS_ENDIAN_TO_HOST_INT16(newKeyLengths[index]);
	int32 size = node->AllKeyLength() - length;
	if (size > 0)
		memmove(keys + length, keys + length - keyLength, size);

	memcpy(keys + keyStart, key, keyLength);
}


status_t
BPlusTree::SplitNode(bplustree_node *node, off_t nodeOffset, bplustree_node *other,
	off_t otherOffset, uint16 *_keyIndex, uint8 *key, uint16 *_keyLength, off_t *_value)
{
	if (*_keyIndex > node->NumKeys() + 1)
		return B_BAD_VALUE;

	uint16 *inKeyLengths = node->KeyLengths();
	off_t *inKeyValues = node->Values();
	uint8 *inKeys = node->Keys();
	uint8 *outKeys = other->Keys();
	int32 keyIndex = *_keyIndex;	// can become less than zero!

	// how many keys will fit in one (half) page?
	// that loop will find the answer to this question and
	// change the key lengths indices for their new home

	// "bytes" is the number of bytes written for the new key,
	// "bytesBefore" are the bytes before that key
	// "bytesAfter" are the bytes after the new key, if any
	int32 bytes = 0, bytesBefore = 0, bytesAfter = 0;

	size_t size = fNodeSize >> 1;
	int32 out, in;
	for (in = out = 0; in < node->NumKeys() + 1;) {
		if (!bytes)
			bytesBefore = in > 0 ? BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in - 1]) : 0;

		if (in == keyIndex && !bytes) {
			bytes = *_keyLength;
		} else {
			if (keyIndex < out)
				bytesAfter = BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in]) - bytesBefore;

			in++;
		}
		out++;

		if (round_up(sizeof(bplustree_node) + bytesBefore + bytesAfter + bytes)
				+ out * (sizeof(uint16) + sizeof(off_t)) >= size) {
			// we have found the number of keys in the new node!
			break;
		}
	}

	// if the new key was not inserted, set the length of the keys
	// that can be copied directly
	if (keyIndex >= out && in > 0)
		bytesBefore = BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in - 1]);

	if (bytesBefore < 0 || bytesAfter < 0)
		return B_BAD_DATA;

	other->left_link = node->left_link;
	other->right_link = HOST_ENDIAN_TO_BFS_INT64(nodeOffset);
	other->all_key_length = HOST_ENDIAN_TO_BFS_INT16(bytes + bytesBefore + bytesAfter);
	other->all_key_count = HOST_ENDIAN_TO_BFS_INT16(out);

	uint16 *outKeyLengths = other->KeyLengths();
	off_t *outKeyValues = other->Values();
	int32 keys = out > keyIndex ? keyIndex : out;

	if (bytesBefore) {
		// copy the keys
		memcpy(outKeys, inKeys, bytesBefore);
		memcpy(outKeyLengths, inKeyLengths, keys * sizeof(uint16));
		memcpy(outKeyValues, inKeyValues, keys * sizeof(off_t));
	}
	if (bytes) {
		// copy the newly inserted key
		memcpy(outKeys + bytesBefore, key, bytes);
		outKeyLengths[keyIndex] = HOST_ENDIAN_TO_BFS_INT16(bytes + bytesBefore);
		outKeyValues[keyIndex] = HOST_ENDIAN_TO_BFS_INT64(*_value);

		if (bytesAfter) {
			// copy the keys after the new key
			memcpy(outKeys + bytesBefore + bytes, inKeys + bytesBefore, bytesAfter);
			keys = out - keyIndex - 1;
			for (int32 i = 0;i < keys;i++)
				outKeyLengths[keyIndex + i + 1] = HOST_ENDIAN_TO_BFS_INT16(BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[keyIndex + i]) + bytes);
			memcpy(outKeyValues + keyIndex + 1, inKeyValues + keyIndex, keys * sizeof(off_t));
		}
	}

	// if the new key was already inserted, we shouldn't use it again
	if (in != out)
		keyIndex--;

	int32 total = bytesBefore + bytesAfter;

	// these variables are for the key that will be returned
	// to the parent node
	uint8 *newKey = NULL;
	uint16 newLength;
	bool newAllocated = false;

	// If we have split an index node, we have to drop the first key
	// of the next node (which can also be the new key to insert).
	// The dropped key is also the one which has to be inserted in
	// the parent node, so we will set the "newKey" already here.
	if (node->OverflowLink() != BPLUSTREE_NULL) {
		if (in == keyIndex) {
			newKey = key;
			newLength = *_keyLength;

			other->overflow_link = HOST_ENDIAN_TO_BFS_INT64(*_value);
			keyIndex--;
		} else {
			// If a key is dropped (is not the new key), we have to copy
			// it, because it would be lost if not.
			uint8 *droppedKey = node->KeyAt(in, &newLength);
			if (droppedKey + newLength + sizeof(off_t) + sizeof(uint16) > (uint8 *)node + fNodeSize
				|| newLength > BPLUSTREE_MAX_KEY_LENGTH) {
				fStream->GetVolume()->Panic();
				RETURN_ERROR(B_BAD_DATA);
			}
			newKey = (uint8 *)malloc(newLength);
			if (newKey == NULL)
				return B_NO_MEMORY;
			memcpy(newKey, droppedKey, newLength);

			other->overflow_link = inKeyValues[in];
			total = BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in++]);
		}
	}

	// and now the same game for the other page and the rest of the keys
	// (but with memmove() instead of memcpy(), because they may overlap)

	bytesBefore = bytesAfter = bytes = 0;
	out = 0;
	int32 skip = in;
	while (in < node->NumKeys() + 1) {
		if (in == keyIndex && !bytes) {
			// it's enough to set bytesBefore once here, because we do
			// not need to know the exact length of all keys in this
			// loop
			bytesBefore = in > skip ? BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in - 1]) : 0;
			bytes = *_keyLength;
		} else {
			if (in < node->NumKeys()) {
				inKeyLengths[in] = HOST_ENDIAN_TO_BFS_INT16(BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in]) - total);
				if (bytes) {
					inKeyLengths[in] = HOST_ENDIAN_TO_BFS_INT16(BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in]) + bytes);
					bytesAfter = BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in]) - bytesBefore - bytes;
				}
			}
			in++;
		}

		out++;

		// break out when all keys are done
		if (in > node->NumKeys() && keyIndex < in)
			break;
	}

	// adjust the byte counts (since we were a bit lazy in the loop)
	if (keyIndex >= in && keyIndex - skip < out)
		bytesAfter = BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in]) - bytesBefore - total;
	else if (keyIndex < skip)
		bytesBefore = node->AllKeyLength() - total;

	if (bytesBefore < 0 || bytesAfter < 0)
		return B_BAD_DATA;

	node->left_link = HOST_ENDIAN_TO_BFS_INT64(otherOffset);
		// right link, and overflow link can stay the same
	node->all_key_length = HOST_ENDIAN_TO_BFS_INT16(bytes + bytesBefore + bytesAfter);
	node->all_key_count = HOST_ENDIAN_TO_BFS_INT16(out - 1);

	// array positions have changed
	outKeyLengths = node->KeyLengths();
	outKeyValues = node->Values();

	// move the keys in the old node: the order is important here,
	// because we don't want to overwrite any contents

	keys = keyIndex <= skip ? out : keyIndex - skip;
	keyIndex -= skip;

	if (bytesBefore)
		memmove(inKeys, inKeys + total, bytesBefore);
	if (bytesAfter)
		memmove(inKeys + bytesBefore + bytes, inKeys + total + bytesBefore, bytesAfter);

	if (bytesBefore)
		memmove(outKeyLengths, inKeyLengths + skip, keys * sizeof(uint16));
	in = out - keyIndex - 1;
	if (bytesAfter)
		memmove(outKeyLengths + keyIndex + 1, inKeyLengths + skip + keyIndex, in * sizeof(uint16));

	if (bytesBefore)
		memmove(outKeyValues, inKeyValues + skip, keys * sizeof(off_t));
	if (bytesAfter)
		memmove(outKeyValues + keyIndex + 1, inKeyValues + skip + keyIndex, in * sizeof(off_t));

	if (bytes) {
		// finally, copy the newly inserted key (don't overwrite anything)
		memcpy(inKeys + bytesBefore, key, bytes);
		outKeyLengths[keyIndex] = HOST_ENDIAN_TO_BFS_INT16(bytes + bytesBefore);
		outKeyValues[keyIndex] = HOST_ENDIAN_TO_BFS_INT64(*_value);
	}

	// Prepare the key that will be inserted in the parent node which
	// is either the dropped key or the last of the other node.
	// If it's the dropped key, "newKey" was already set earlier.

	if (newKey == NULL)
		newKey = other->KeyAt(other->NumKeys() - 1, &newLength);

	memcpy(key, newKey, newLength);
	*_keyLength = newLength;
	*_value = otherOffset;

	if (newAllocated)
		free(newKey);

	return B_OK;
}


status_t
BPlusTree::Insert(Transaction *transaction, const uint8 *key, uint16 keyLength, off_t value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH || keyLength > BPLUSTREE_MAX_KEY_LENGTH)
		RETURN_ERROR(B_BAD_VALUE);

	// lock access to stream
	WriteLocked locked(fStream->Lock());

	Stack<node_and_key> stack;
	if (SeekDown(stack, key, keyLength) != B_OK)
		RETURN_ERROR(B_ERROR);

	uint8 keyBuffer[BPLUSTREE_MAX_KEY_LENGTH + 1];

	memcpy(keyBuffer, key, keyLength);
	keyBuffer[keyLength] = 0;

	node_and_key nodeAndKey;
	bplustree_node *node;

	CachedNode cached(this);
	while (stack.Pop(&nodeAndKey) && (node = cached.SetTo(nodeAndKey.nodeOffset)) != NULL) {
#ifdef DEBUG
		NodeChecker checker(node, fNodeSize);
#endif
		if (node->IsLeaf()) {
			// first round, check for duplicate entries
			status_t status = FindKey(node, key, keyLength, &nodeAndKey.keyIndex);

			// is this a duplicate entry?
			if (status == B_OK) {
				if (fAllowDuplicates)
					return InsertDuplicate(transaction, &cached, node, nodeAndKey.keyIndex, value);

				RETURN_ERROR(B_NAME_IN_USE);
			}
		}

		// is the node big enough to hold the pair?
		if (int32(round_up(sizeof(bplustree_node) + node->AllKeyLength() + keyLength)
			+ (node->NumKeys() + 1) * (sizeof(uint16) + sizeof(off_t))) < fNodeSize)
		{
			InsertKey(node, nodeAndKey.keyIndex, keyBuffer, keyLength, value);
			UpdateIterators(nodeAndKey.nodeOffset, BPLUSTREE_NULL, nodeAndKey.keyIndex, 0, 1);

			return cached.WriteBack(transaction);
		} else {
			CachedNode cachedNewRoot(this);
			CachedNode cachedOther(this);

			// do we need to allocate a new root node? if so, then do
			// it now
			off_t newRoot = BPLUSTREE_NULL;
			if (nodeAndKey.nodeOffset == fHeader->RootNode()) {
				bplustree_node *root;
				status_t status = cachedNewRoot.Allocate(transaction, &root, &newRoot);
				if (status < B_OK) {
					// The tree is most likely corrupted!
					// But it's still sane at leaf level - we could set
					// a flag in the header that forces the tree to be
					// rebuild next time...
					// But since we will have journaling, that's not a big
					// problem anyway.
					RETURN_ERROR(status);
				}
			}

			// reserve space for the other node
			bplustree_node *other;
			off_t otherOffset;
			status_t status = cachedOther.Allocate(transaction, &other, &otherOffset);
			if (status < B_OK) {
				cachedNewRoot.Free(transaction, newRoot);
				RETURN_ERROR(status);
			}

			if (SplitNode(node, nodeAndKey.nodeOffset, other, otherOffset,
					&nodeAndKey.keyIndex, keyBuffer, &keyLength, &value) < B_OK) {
				// free root node & other node here
				cachedNewRoot.Free(transaction, newRoot);
				cachedOther.Free(transaction, otherOffset);					

				RETURN_ERROR(B_ERROR);
			}

			// write the updated nodes back
		
			if (cached.WriteBack(transaction) < B_OK
				|| cachedOther.WriteBack(transaction) < B_OK)
				RETURN_ERROR(B_ERROR);

			UpdateIterators(nodeAndKey.nodeOffset, otherOffset, nodeAndKey.keyIndex,
				node->NumKeys(), 1);

			// update the right link of the node in the left of the new node
			if ((other = cachedOther.SetTo(other->LeftLink())) != NULL) {
				other->right_link = HOST_ENDIAN_TO_BFS_INT64(otherOffset);
				if (cachedOther.WriteBack(transaction) < B_OK)
					RETURN_ERROR(B_ERROR);
			}

			// create a new root if necessary
			if (newRoot != BPLUSTREE_NULL) {
				bplustree_node *root = cachedNewRoot.Node();

				InsertKey(root, 0, keyBuffer, keyLength, node->LeftLink());
				root->overflow_link = HOST_ENDIAN_TO_BFS_INT64(nodeAndKey.nodeOffset);

				if (cachedNewRoot.WriteBack(transaction) < B_OK)
					RETURN_ERROR(B_ERROR);

				// finally, update header to point to the new root
				fHeader->root_node_pointer = HOST_ENDIAN_TO_BFS_INT64(newRoot);
				fHeader->max_number_of_levels = HOST_ENDIAN_TO_BFS_INT32(fHeader->MaxNumberOfLevels() + 1);

				return fCachedHeader.WriteBack(transaction);
			}
		}
	}

	RETURN_ERROR(B_ERROR);
}


status_t
BPlusTree::RemoveDuplicate(Transaction *transaction, bplustree_node *node, CachedNode *cached,
	uint16 index, off_t value)
{
	CachedNode cachedDuplicate(this);
	off_t *values = node->Values();
	off_t oldValue = BFS_ENDIAN_TO_HOST_INT64(values[index]);
	status_t status;

	off_t duplicateOffset = bplustree_node::FragmentOffset(oldValue);
	bplustree_node *duplicate = cachedDuplicate.SetTo(duplicateOffset, false);
	if (duplicate == NULL)
		return B_IO_ERROR;

	// if it's a duplicate fragment, remove the entry from there
	if (bplustree_node::LinkType(oldValue) == BPLUSTREE_DUPLICATE_FRAGMENT) {
		duplicate_array *array = duplicate->FragmentAt(bplustree_node::FragmentIndex(oldValue));

		if (array->CountItems() > NUM_FRAGMENT_VALUES
			|| array->CountItems() < 1) {
			FATAL(("removeDuplicate: Invalid array[%ld] size in fragment %Ld == %Ld!\n",
				bplustree_node::FragmentIndex(oldValue), duplicateOffset, array->CountItems()));
			return B_BAD_DATA;
		}
		if (!array->Remove(value)) {
			FATAL(("Oh no, value %Ld not found in fragments of node %Ld...\n",
				value, duplicateOffset));
			return B_ENTRY_NOT_FOUND;
		}

		// remove the array from the fragment node if it is empty
		if (array->CountItems() == 1) {
			// set the link to the remaining value
			values[index] = array->values[0];

			// Remove the whole fragment node, if this was the only array,
			// otherwise free the array and write the changes back
			if (duplicate->FragmentsUsed(fNodeSize) == 1)
				status = cachedDuplicate.Free(transaction, duplicateOffset);
			else {
				array->count = 0;
				status = cachedDuplicate.WriteBack(transaction);
			}
			if (status < B_OK)
				return status;

			return cached->WriteBack(transaction);
		}
		return cachedDuplicate.WriteBack(transaction);
	}

	// Remove value from a duplicate node!

	duplicate_array *array = NULL;

	if (duplicate->LeftLink() != BPLUSTREE_NULL) {
		FATAL(("invalid duplicate node: first left link points to %Ld!\n", duplicate->LeftLink()));
		return B_BAD_DATA;
	}

	// Search the duplicate nodes until the entry could be found (and removed)
	while (duplicate != NULL) {
		array = duplicate->DuplicateArray();
		if (array->CountItems() > NUM_DUPLICATE_VALUES
			|| array->CountItems() < 0) {
			FATAL(("removeDuplicate: Invalid array size in duplicate %Ld == %Ld!\n",
				duplicateOffset, array->CountItems()));
			return B_BAD_DATA;
		}

		if (array->Remove(value))
			break;

		if ((duplicateOffset = duplicate->RightLink()) == BPLUSTREE_NULL)
			RETURN_ERROR(B_ENTRY_NOT_FOUND);
		
		duplicate = cachedDuplicate.SetTo(duplicateOffset, false);
	}
	if (duplicate == NULL)
		RETURN_ERROR(B_IO_ERROR);

	while (true) {
		off_t left = duplicate->LeftLink();
		off_t right = duplicate->RightLink();
		bool isLast = left == BPLUSTREE_NULL && right == BPLUSTREE_NULL;

		if (isLast && array->CountItems() == 1 || array->CountItems() == 0) {
			// Free empty duplicate page, link their siblings together, and
			// update the duplicate link if needed (which should not be, if
			// we are the only one working on that tree...)
	
			if (duplicateOffset == bplustree_node::FragmentOffset(oldValue)
				|| array->CountItems() == 1) {
				if (array->CountItems() == 1 && isLast)
					values[index] = array->values[0];
				else if (isLast) {
					FATAL(("removed last value from duplicate!\n"));
				} else {
					values[index] = HOST_ENDIAN_TO_BFS_INT64(bplustree_node::MakeLink(
						BPLUSTREE_DUPLICATE_NODE, right));
				}

				if ((status = cached->WriteBack(transaction)) < B_OK)
					return status;
			}

			if ((status = cachedDuplicate.Free(transaction, duplicateOffset)) < B_OK)
				return status;

			if (left != BPLUSTREE_NULL
				&& (duplicate = cachedDuplicate.SetTo(left, false)) != NULL) {
				duplicate->right_link = HOST_ENDIAN_TO_BFS_INT64(right);

				// If the next node is the last node, we need to free that node
				// and convert the duplicate entry back into a normal entry
				array = duplicate->DuplicateArray();
				if (right == BPLUSTREE_NULL && duplicate->LeftLink() == BPLUSTREE_NULL
					&& duplicate->DuplicateArray()->count <= NUM_FRAGMENT_VALUES) {
					duplicateOffset = left;
					continue;
				}

				status = cachedDuplicate.WriteBack(transaction);
				if (status < B_OK)
					return status;
			}
			if (right != BPLUSTREE_NULL
				&& (duplicate = cachedDuplicate.SetTo(right, false)) != NULL) {
				duplicate->left_link = HOST_ENDIAN_TO_BFS_INT64(left);

				// Again, we may need to turn the duplicate entry back into a normal entry
				array = duplicate->DuplicateArray();
				if (left == BPLUSTREE_NULL && duplicate->RightLink() == BPLUSTREE_NULL
					&& duplicate->DuplicateArray()->CountItems() <= NUM_FRAGMENT_VALUES) {
					duplicateOffset = right;
					continue;
				}

				return cachedDuplicate.WriteBack(transaction);
			}
			return status;
		} else if (isLast && array->CountItems() <= NUM_FRAGMENT_VALUES) {
			// If the number of entries fits in a duplicate fragment, then
			// either find a free fragment node, or convert this node to a
			// fragment node.
			CachedNode cachedOther(this);

			bplustree_node *fragment = NULL;
			uint32 fragmentIndex = 0;
			off_t offset;
			if (FindFreeDuplicateFragment(node, &cachedOther, &offset,
					&fragment, &fragmentIndex) < B_OK) {
				// convert node
				memmove(duplicate, array, (NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));
				memset((off_t *)duplicate + NUM_FRAGMENT_VALUES + 1, 0,
					fNodeSize - (NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));
			} else {
				// move to other node
				duplicate_array *target = fragment->FragmentAt(fragmentIndex);
				memcpy(target, array, (NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));

				cachedDuplicate.Free(transaction, duplicateOffset);
				duplicateOffset = offset;
			}
			values[index] = HOST_ENDIAN_TO_BFS_INT64(bplustree_node::MakeLink(
				BPLUSTREE_DUPLICATE_FRAGMENT, duplicateOffset, fragmentIndex));

			if ((status = cached->WriteBack(transaction)) < B_OK)
				return status;

			if (fragment != NULL)
				return cachedOther.WriteBack(transaction);
		}
		return cachedDuplicate.WriteBack(transaction);
	}
}


/** Removes the key with the given index from the specified node.
 *	Since it has to get the key from the node anyway (to obtain it's
 *	pointer), it's not needed to pass the key & its length, although
 *	the calling method (BPlusTree::Remove()) have this data.
 */

void
BPlusTree::RemoveKey(bplustree_node *node, uint16 index)
{
	// should never happen, but who knows?
	if (index > node->NumKeys() && node->NumKeys() > 0) {
		FATAL(("Asked me to remove key outer limits: %u\n", index));
		return;
	}

	off_t *values = node->Values();

	// if we would have to drop the overflow link, drop
	// the last key instead and update the overflow link
	// to the value of that one
	if (!node->IsLeaf() && index == node->NumKeys())
		node->overflow_link = values[--index];

	uint16 length;
	uint8 *key = node->KeyAt(index, &length);
	if (key + length + sizeof(off_t) + sizeof(uint16) > (uint8 *)node + fNodeSize
		|| length > BPLUSTREE_MAX_KEY_LENGTH) {
		FATAL(("Key length to long: %s, %u (inode at %ld,%u)\n", key, length,
			fStream->BlockRun().allocation_group, fStream->BlockRun().start));
		fStream->GetVolume()->Panic();
		return;
	}

	uint16 *keyLengths = node->KeyLengths();
	uint8 *keys = node->Keys();

	node->all_key_count = HOST_ENDIAN_TO_BFS_INT16(node->NumKeys() - 1);
	node->all_key_length = HOST_ENDIAN_TO_BFS_INT64(node->AllKeyLength() - length);

	off_t *newValues = node->Values();
	uint16 *newKeyLengths = node->KeyLengths();

	// move key data
	memmove(key, key + length, node->AllKeyLength() - (key - keys));

	// move and update key lengths
	if (index > 0 && newKeyLengths != keyLengths)
		memmove(newKeyLengths, keyLengths, index * sizeof(uint16));
	for (uint16 i = index; i < node->NumKeys(); i++) {
		newKeyLengths[i] = HOST_ENDIAN_TO_BFS_INT16(
			BFS_ENDIAN_TO_HOST_INT16(keyLengths[i + 1]) - length);
	}

	// move values
	if (index > 0)
		memmove(newValues, values, index * sizeof(off_t));
	if (node->NumKeys() > index)
		memmove(newValues + index, values + index + 1, (node->NumKeys() - index) * sizeof(off_t));
}


/**	Removes the specified key from the tree. The "value" parameter is only used
 *	for trees which allow duplicates, so you may safely ignore it.
 *	It's not an optional parameter, so at least you have to think about it.
 */

status_t
BPlusTree::Remove(Transaction *transaction, const uint8 *key, uint16 keyLength, off_t value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH || keyLength > BPLUSTREE_MAX_KEY_LENGTH)
		RETURN_ERROR(B_BAD_VALUE);

	// lock access to stream
	WriteLocked locked(fStream->Lock());

	Stack<node_and_key> stack;
	if (SeekDown(stack, key, keyLength) != B_OK)
		RETURN_ERROR(B_ERROR);

	node_and_key nodeAndKey;
	bplustree_node *node;

	CachedNode cached(this);
	while (stack.Pop(&nodeAndKey) && (node = cached.SetTo(nodeAndKey.nodeOffset)) != NULL) {
#ifdef DEBUG
		NodeChecker checker(node, fNodeSize);
#endif
		if (node->IsLeaf()) {
			// first round, check for duplicate entries
			status_t status = FindKey(node, key, keyLength, &nodeAndKey.keyIndex);
			if (status < B_OK)
				RETURN_ERROR(status);

			// If we will remove the last key, the iterator will be set
			// to the next node after the current - if there aren't any
			// more nodes, we need a way to prevent the TreeIterators to
			// touch the old node again, we use BPLUSTREE_FREE for this
			off_t next = node->RightLink() == BPLUSTREE_NULL ? BPLUSTREE_FREE : node->RightLink();
			UpdateIterators(nodeAndKey.nodeOffset, node->NumKeys() == 1 ?
								next : BPLUSTREE_NULL, nodeAndKey.keyIndex, 0 , -1);

			// is this a duplicate entry?
			if (bplustree_node::IsDuplicate(BFS_ENDIAN_TO_HOST_INT64(
					node->Values()[nodeAndKey.keyIndex]))) {
				if (fAllowDuplicates)
					return RemoveDuplicate(transaction, node, &cached, nodeAndKey.keyIndex, value);
				else
					RETURN_ERROR(B_NAME_IN_USE);
			}
		}

		// if it's an empty root node, we have to convert it
		// to a leaf node by dropping the overflow link, or,
		// if it's already a leaf node, just empty it
		if (nodeAndKey.nodeOffset == fHeader->RootNode()
			&& (node->NumKeys() == 0 || node->NumKeys() == 1 && node->IsLeaf())) {
			node->overflow_link = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
			node->all_key_count = 0;
			node->all_key_length = 0;

			if (cached.WriteBack(transaction) < B_OK)
				return B_IO_ERROR;

			// if we've made a leaf node out of the root node, we need
			// to reset the maximum number of levels in the header
			if (fHeader->MaxNumberOfLevels() != 1) {
				fHeader->max_number_of_levels = HOST_ENDIAN_TO_BFS_INT32(1);
				return fCachedHeader.WriteBack(transaction);
			}
			return B_OK;
		}

		// if there is only one key left, we don't have to remove
		// it, we can just dump the node (index nodes still have
		// the overflow link, so we have to drop the last key)
		if (node->NumKeys() > 1
			|| !node->IsLeaf() && node->NumKeys() == 1) {
			RemoveKey(node, nodeAndKey.keyIndex);
			return cached.WriteBack(transaction);
		}

		// when we are here, we can just free the node, but
		// we have to update the right/left link of the
		// siblings first
		CachedNode otherCached(this);
		bplustree_node *other = otherCached.SetTo(node->LeftLink());
		if (other != NULL) {
			other->right_link = node->right_link;
			if (otherCached.WriteBack(transaction) < B_OK)
				return B_IO_ERROR;
		}

		if ((other = otherCached.SetTo(node->RightLink())) != NULL) {
			other->left_link = node->left_link;
			if (otherCached.WriteBack(transaction) < B_OK)
				return B_IO_ERROR;
		}

		cached.Free(transaction, nodeAndKey.nodeOffset);
	}
	RETURN_ERROR(B_ERROR);
}


/**	Replaces the value for the key in the tree.
 *	Returns B_OK if the key could be found and its value replaced,
 *	B_ENTRY_NOT_FOUND if the key couldn't be found, and other errors
 *	to indicate that something went terribly wrong.
 *	Note that this doesn't work with duplicates - it will just
 *	return B_BAD_TYPE if you call this function on a tree where
 *	duplicates are allowed.
 */

status_t
BPlusTree::Replace(Transaction *transaction, const uint8 *key, uint16 keyLength, off_t value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH || keyLength > BPLUSTREE_MAX_KEY_LENGTH
		|| key == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	if (fAllowDuplicates)
		RETURN_ERROR(B_BAD_TYPE);

	// lock access to stream (a read lock is okay for this purpose)
	ReadLocked locked(fStream->Lock());

	off_t nodeOffset = fHeader->RootNode();
	CachedNode cached(this);
	bplustree_node *node;

	while ((node = cached.SetTo(nodeOffset)) != NULL) {
		uint16 keyIndex = 0;
		off_t nextOffset;
		status_t status = FindKey(node, key, keyLength, &keyIndex, &nextOffset);

		if (node->OverflowLink() == BPLUSTREE_NULL) {
			if (status == B_OK) {
				node->Values()[keyIndex] = HOST_ENDIAN_TO_BFS_INT64(value);
				return cached.WriteBack(transaction);
			}

			return status;
		} else if (nextOffset == nodeOffset)
			RETURN_ERROR(B_ERROR);

		nodeOffset = nextOffset;
	}
	RETURN_ERROR(B_ERROR);
}


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
		RETURN_ERROR(B_BAD_VALUE);

	if (fAllowDuplicates)
		RETURN_ERROR(B_BAD_TYPE);

	// lock access to stream
	ReadLocked locked(fStream->Lock());

	off_t nodeOffset = fHeader->RootNode();
	CachedNode cached(this);
	bplustree_node *node;

#ifdef DEBUG
	int32 levels = 0;
#endif

	while ((node = cached.SetTo(nodeOffset)) != NULL) {
		uint16 keyIndex = 0;
		off_t nextOffset;
		status_t status = FindKey(node, key, keyLength, &keyIndex, &nextOffset);

#ifdef DEBUG
		levels++;
#endif
		if (node->OverflowLink() == BPLUSTREE_NULL) {
			if (status == B_OK && _value != NULL)
				*_value = BFS_ENDIAN_TO_HOST_INT64(node->Values()[keyIndex]);

#ifdef DEBUG
			if (levels != (int32)fHeader->MaxNumberOfLevels())
				DEBUGGER(("levels don't match"));
#endif
			return status;
		} else if (nextOffset == nodeOffset)
			RETURN_ERROR(B_ERROR);

		nodeOffset = nextOffset;
	}
	FATAL(("b+tree node at %Ld could not be loaded\n", nodeOffset));
	RETURN_ERROR(B_ERROR);
}


//	#pragma mark -


TreeIterator::TreeIterator(BPlusTree *tree)
	:
	fTree(tree),
	fCurrentNodeOffset(BPLUSTREE_NULL),
	fNext(NULL)
{
	tree->AddIterator(this);
}


TreeIterator::~TreeIterator()
{
	if (fTree)
		fTree->RemoveIterator(this);
}


status_t
TreeIterator::Goto(int8 to)
{
	if (fTree == NULL || fTree->fHeader == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// lock access to stream
	ReadLocked locked(fTree->fStream->Lock());

	off_t nodeOffset = fTree->fHeader->RootNode();
	CachedNode cached(fTree);
	bplustree_node *node;

	while ((node = cached.SetTo(nodeOffset)) != NULL) {
		// is the node a leaf node?
		if (node->OverflowLink() == BPLUSTREE_NULL) {
			fCurrentNodeOffset = nodeOffset;
			fCurrentKey = to == BPLUSTREE_BEGIN ? -1 : node->NumKeys();
			fDuplicateNode = BPLUSTREE_NULL;

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
				RETURN_ERROR(B_ERROR);

			nextOffset = BFS_ENDIAN_TO_HOST_INT64(node->Values()[0]);
		}
		if (nextOffset == nodeOffset)
			break;

		nodeOffset = nextOffset;
	}
	FATAL(("%s fails\n", __FUNCTION__));

	RETURN_ERROR(B_ERROR);
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
		RETURN_ERROR(B_ERROR);

	// if the tree was emptied since the last call
	if (fCurrentNodeOffset == BPLUSTREE_FREE)
		return B_ENTRY_NOT_FOUND;

	// lock access to stream
	ReadLocked locked(fTree->fStream->Lock());

	CachedNode cached(fTree);
	bplustree_node *node;

	if (fDuplicateNode != BPLUSTREE_NULL) {
		// regardless of traverse direction the duplicates are always presented in
		// the same order; since they are all considered as equal, this shouldn't
		// cause any problems

		if (!fIsFragment || fDuplicate < fNumDuplicates)
			node = cached.SetTo(bplustree_node::FragmentOffset(fDuplicateNode), false);
		else
			node = NULL;

		if (node != NULL) {
			if (!fIsFragment && fDuplicate >= fNumDuplicates) {
				// if the node is out of duplicates, we go directly to the next one
				fDuplicateNode = node->RightLink();
				if (fDuplicateNode != BPLUSTREE_NULL
					&& (node = cached.SetTo(fDuplicateNode, false)) != NULL) {
					fNumDuplicates = node->CountDuplicates(fDuplicateNode, false);
					fDuplicate = 0;
				}
			}
			if (fDuplicate < fNumDuplicates) {
				*value = node->DuplicateAt(fDuplicateNode, fIsFragment, fDuplicate++);
				if (duplicate)
					*duplicate = 2;
				return B_OK;
			}
		}
		fDuplicateNode = BPLUSTREE_NULL;
	}

	off_t savedNodeOffset = fCurrentNodeOffset;
	int32 savedKey = fCurrentKey;

	if ((node = cached.SetTo(fCurrentNodeOffset)) == NULL)
		RETURN_ERROR(B_ERROR);

	if (duplicate)
		*duplicate = 0;

	fCurrentKey += direction;
	
	// is the current key in the current node?
	while ((direction == BPLUSTREE_FORWARD && fCurrentKey >= node->NumKeys())
			|| (direction == BPLUSTREE_BACKWARD && fCurrentKey < 0)) {
		fCurrentNodeOffset = direction == BPLUSTREE_FORWARD ? node->RightLink() : node->LeftLink();

		// are there any more nodes?
		if (fCurrentNodeOffset != BPLUSTREE_NULL) {
			node = cached.SetTo(fCurrentNodeOffset);
			if (!node)
				RETURN_ERROR(B_ERROR);

			// reset current key
			fCurrentKey = direction == BPLUSTREE_FORWARD ? 0 : node->NumKeys();
		} else {
			// there are no nodes left, so turn back to the last key
			fCurrentNodeOffset = savedNodeOffset;
			fCurrentKey = savedKey;

			return B_ENTRY_NOT_FOUND;
		}
	}

	if (node->all_key_count == 0)
		RETURN_ERROR(B_ERROR);	// B_ENTRY_NOT_FOUND ?

	uint16 length;
	uint8 *keyStart = node->KeyAt(fCurrentKey, &length);
	if (keyStart + length + sizeof(off_t) + sizeof(uint16) > (uint8 *)node + fTree->fNodeSize
		|| length > BPLUSTREE_MAX_KEY_LENGTH) {
		fTree->fStream->GetVolume()->Panic();
		RETURN_ERROR(B_BAD_DATA);
	}

	length = min_c(length, maxLength);
	memcpy(key, keyStart, length);
	
	if (fTree->fHeader->data_type == BPLUSTREE_STRING_TYPE)	{
		// terminate string type
		if (length == maxLength)
			length--;
		((char *)key)[length] = '\0';
	}
	*keyLength = length;

	off_t offset = BFS_ENDIAN_TO_HOST_INT64(node->Values()[fCurrentKey]);

	// duplicate fragments?
	uint8 type = bplustree_node::LinkType(offset);
	if (type == BPLUSTREE_DUPLICATE_FRAGMENT || type == BPLUSTREE_DUPLICATE_NODE) {
		fDuplicateNode = offset;

		node = cached.SetTo(bplustree_node::FragmentOffset(fDuplicateNode), false);
		if (node == NULL)
			RETURN_ERROR(B_ERROR);

		fIsFragment = type == BPLUSTREE_DUPLICATE_FRAGMENT;

		fNumDuplicates = node->CountDuplicates(offset, fIsFragment);
		if (fNumDuplicates) {
			offset = node->DuplicateAt(offset, fIsFragment, 0);
			fDuplicate = 1;
			if (duplicate)
				*duplicate = 1;
		} else {
			// shouldn't happen, but we're dealing here with potentially corrupt disks...
			fDuplicateNode = BPLUSTREE_NULL;
			offset = 0;
		}
	}
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
		RETURN_ERROR(B_BAD_VALUE);

	// lock access to stream
	ReadLocked locked(fTree->fStream->Lock());

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
			fDuplicateNode = BPLUSTREE_NULL;

			return status;
		} else if (nextOffset == nodeOffset)
			RETURN_ERROR(B_ERROR);

		nodeOffset = nextOffset;
	}
	RETURN_ERROR(B_ERROR);
}


void 
TreeIterator::SkipDuplicates()
{
	fDuplicateNode = BPLUSTREE_NULL;
}


void 
TreeIterator::Update(off_t offset, off_t nextOffset, uint16 keyIndex, uint16 splitAt, int8 change)
{
	if (offset != fCurrentNodeOffset)
		return;

	if (nextOffset != BPLUSTREE_NULL) {
		fCurrentNodeOffset = nextOffset;
		if (splitAt <= fCurrentKey) {
			fCurrentKey -= splitAt;
			keyIndex -= splitAt;
		}
	}

	// Adjust fCurrentKey to point to the same key as before.
	// Note, that if a key is inserted at the current position
	// it won't be included in this tree transition.
	if (keyIndex <= fCurrentKey)
		fCurrentKey += change;

	// ToDo: duplicate handling!
}


void 
TreeIterator::Stop()
{
	fTree = NULL;
}


#ifdef DEBUG
void 
TreeIterator::Dump()
{
	__out("TreeIterator at %p:\n", this);
	__out("\tfTree = %p\n", fTree);
	__out("\tfCurrentNodeOffset = %Ld\n", fCurrentNodeOffset);
	__out("\tfCurrentKey = %ld\n", fCurrentKey);
	__out("\tfDuplicateNode = %Ld (%Ld, 0x%Lx)\n", bplustree_node::FragmentOffset(fDuplicateNode), fDuplicateNode, fDuplicateNode);
	__out("\tfDuplicate = %u\n", fDuplicate);
	__out("\tfNumDuplicates = %u\n", fNumDuplicates);
	__out("\tfIsFragment = %s\n", fIsFragment ? "true" : "false");
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
		if (array->CountItems() > 0 && ++used > 1)
			return used;
	}
	return used;
}


#ifdef DEBUG
void 
bplustree_node::CheckIntegrity(uint32 nodeSize)
{
	if (NumKeys() > nodeSize || AllKeyLength() > nodeSize)
		DEBUGGER(("invalid node: key/length count"));

	for (int32 i = 0; i < NumKeys(); i++) {
		uint16 length;
		uint8 *key = KeyAt(i, &length);
		if (key + length + sizeof(off_t) + sizeof(uint16) > (uint8 *)this + nodeSize
			|| length > BPLUSTREE_MAX_KEY_LENGTH)
			DEBUGGER(("invalid node: keys corrupted"));
	}
}
#endif


//	#pragma mark -


int32
compareKeys(type_code type, const void *key1, int keyLength1, const void *key2, int keyLength2)
{
	// if one of the keys is NULL, bail out gracefully
	if (key1 == NULL || key2 == NULL) {
//#if USER
//		// that's here to see if it's reasonable to handle this case nicely at all
//		DEBUGGER(("compareKeys() got NULL key!"));
//#endif
		// even if it's most probably a bug in the calling code, we try to
		// give a meaningful result
		if (key1 == NULL && key2 != NULL)
			return 1;
		else if (key2 == NULL && key1 != NULL)
			return -1;

		return 0;
	}

	switch (type) {
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

