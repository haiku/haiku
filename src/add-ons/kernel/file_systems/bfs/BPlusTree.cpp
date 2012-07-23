/*
 * Copyright 2001-2012, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 *
 * Roughly based on 'btlib' written by Marcus J. Ranum - it shares
 * no code but achieves binary compatibility with the on disk format.
 */


//! B+Tree implementation


#include "Debug.h"
#include "BPlusTree.h"
#include "Inode.h"
#include "Utility.h"


/*!	Simple array used for the duplicate handling in the B+Tree. This is an
	on disk structure.
*/
struct duplicate_array {
	off_t	count;
	off_t	values[0];

	inline bool IsEmpty() const
	{
		return count == 0;
	}

	inline int32 Count() const
	{
		return (int32)BFS_ENDIAN_TO_HOST_INT64(count);
	}

	inline off_t ValueAt(uint32 index) const
	{
		return BFS_ENDIAN_TO_HOST_INT64(values[index]);
	}

	inline void SetValueAt(uint32 index, off_t value)
	{
		values[index] = HOST_ENDIAN_TO_BFS_INT64(value);
	}

	inline int32 Find(off_t value) const
	{
		int32 i;
		return _FindInternal(value, i) ? i : -1;
	}

	void Insert(off_t value);
	bool Remove(off_t value);

private:
	bool _FindInternal(off_t value, int32& index) const;
} _PACKED;


#ifdef DEBUG
class NodeChecker {
public:
	NodeChecker(const bplustree_node* node, int32 nodeSize, const char* text)
		:
		fNode(node),
		fSize(nodeSize),
		fText(text)
	{
		Check("integrity check failed on construction.");
	}

	~NodeChecker()
	{
		Check("integrity check failed on destruction.");
	}

	void
	Check(const char* message)
	{
		if (fNode->CheckIntegrity(fSize) != B_OK) {
			dprintf("%s: %s\n", fText, message);
			DEBUGGER(("NodeChecker integrity check failed!"));
		}
	}

private:
	const bplustree_node*	fNode;
	int32					fSize;
	const char*				fText;
};
#endif


class BitmapArray {
public:
								BitmapArray(size_t numBits);
								~BitmapArray();

			status_t			InitCheck() const;

			bool				IsSet(size_t index) const;
			void				Set(size_t index, bool set);

			size_t				CountSet() const { return fCountSet; }

private:
			uint8*				fBitmap;
			size_t				fSize;
			size_t				fCountSet;
};


struct TreeCheck {
	TreeCheck(BPlusTree* tree)
		:
		fLevelCount(0),
		fFreeCount(0),
		fNodeSize(tree->NodeSize()),
		fMaxLevels(tree->fHeader.MaxNumberOfLevels()),
		fFoundErrors(0),
		fVisited(tree->Stream()->Size() / tree->NodeSize()),
		fVisitedFragment(tree->Stream()->Size() / tree->NodeSize())
	{
		fPreviousOffsets = (off_t*)malloc(
			sizeof(off_t) * tree->fHeader.MaxNumberOfLevels());
		if (fPreviousOffsets != NULL) {
			for (size_t i = 0; i < fMaxLevels; i++)
				fPreviousOffsets[i] = BPLUSTREE_NULL;
		}
	}

	~TreeCheck()
	{
		free(fPreviousOffsets);
	}

	status_t InitCheck() const
	{
		if (fPreviousOffsets == NULL)
			return B_NO_MEMORY;

		status_t status = fVisited.InitCheck();
		if (status != B_OK)
			return status;

		return fVisitedFragment.InitCheck();
	}

	bool Visited(off_t offset) const
	{
		return fVisited.IsSet(offset / fNodeSize);
	}

	void SetVisited(off_t offset)
	{
		fVisited.Set(offset / fNodeSize, true);
	}

	size_t VisitedCount() const
	{
		return fVisited.CountSet();
	}

	bool VisitedFragment(off_t offset) const
	{
		return fVisitedFragment.IsSet(offset / fNodeSize);
	}

	void SetVisitedFragment(off_t offset)
	{
		fVisitedFragment.Set(offset / fNodeSize, true);
	}

	uint32 MaxLevels() const
	{
		return fLevelCount;
	}

	void SetLevel(uint32 level)
	{
		if (fLevelCount < level)
			fLevelCount = level;
	}

	off_t PreviousOffset(uint32 level)
	{
		return fPreviousOffsets[level];
	}

	void SetPreviousOffset(uint32 level, off_t offset)
	{
		fPreviousOffsets[level] = offset;
	}

	void FoundError()
	{
		fFoundErrors++;
	}

	bool ErrorsFound()
	{
		return fFoundErrors != 0;
	}

private:
			uint32				fLevelCount;
			uint32				fFreeCount;
			uint32				fNodeSize;
			uint32				fMaxLevels;
			uint32				fFoundErrors;
			BitmapArray			fVisited;
			BitmapArray			fVisitedFragment;
			off_t*				fPreviousOffsets;
};


// #pragma mark -


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
CachedNode::UnsetUnchanged(Transaction& transaction)
{
	if (fTree == NULL || fTree->fStream == NULL)
		return;

	if (fNode != NULL) {
		void* cache = fTree->fStream->GetVolume()->BlockCache();

		block_cache_set_dirty(cache, fBlockNumber, false, transaction.ID());
		block_cache_put(cache, fBlockNumber);
		fNode = NULL;
	}
}


void
CachedNode::Unset()
{
	if (fTree == NULL || fTree->fStream == NULL)
		return;

	if (fNode != NULL) {
		if (fWritable && fOffset == 0) {
			// The B+tree header has been updated - we need to update the
			// BPlusTrees copy of it, as well.
			memcpy(&fTree->fHeader, fNode, sizeof(bplustree_header));
		}

		block_cache_put(fTree->fStream->GetVolume()->BlockCache(),
			fBlockNumber);
		fNode = NULL;
	}
}


const bplustree_node*
CachedNode::SetTo(off_t offset, bool check)
{
	const bplustree_node* node;
	if (SetTo(offset, &node, check) == B_OK)
		return node;

	return NULL;
}


status_t
CachedNode::SetTo(off_t offset, const bplustree_node** _node, bool check)
{
	if (fTree == NULL || fTree->fStream == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Unset();

	// You can only ask for nodes at valid positions - you can't
	// even access the b+tree header with this method (use SetToHeader()
	// instead)
	if (offset > fTree->fHeader.MaximumSize() - fTree->fNodeSize
		|| offset <= 0
		|| (offset % fTree->fNodeSize) != 0)
		RETURN_ERROR(B_BAD_VALUE);

	if (InternalSetTo(NULL, offset) != NULL && check) {
		// sanity checks (links, all_key_count)
		if (!fTree->fHeader.CheckNode(fNode)) {
			FATAL(("invalid node [%p] read from offset %" B_PRIdOFF " (block %"
				B_PRIdOFF "), inode at %" B_PRIdINO "\n", fNode, offset,
				fBlockNumber, fTree->fStream->ID()));
			return B_BAD_DATA;
		}
	}

	*_node = fNode;
	return B_OK;
}


bplustree_node*
CachedNode::SetToWritable(Transaction& transaction, off_t offset, bool check)
{
	if (fTree == NULL || fTree->fStream == NULL) {
		REPORT_ERROR(B_BAD_VALUE);
		return NULL;
	}

	Unset();

	// You can only ask for nodes at valid positions - you can't
	// even access the b+tree header with this method (use SetToHeader()
	// instead)
	if (offset > fTree->fHeader.MaximumSize() - fTree->fNodeSize
		|| offset <= 0
		|| (offset % fTree->fNodeSize) != 0)
		return NULL;

	if (InternalSetTo(&transaction, offset) != NULL && check) {
		// sanity checks (links, all_key_count)
		if (!fTree->fHeader.CheckNode(fNode)) {
			FATAL(("invalid node [%p] read from offset %" B_PRIdOFF " (block %"
				B_PRIdOFF "), inode at %" B_PRIdINO "\n", fNode, offset,
				fBlockNumber, fTree->fStream->ID()));
			return NULL;
		}
	}
	return fNode;
}


bplustree_node*
CachedNode::MakeWritable(Transaction& transaction)
{
	if (fNode == NULL)
		return NULL;

	if (block_cache_make_writable(transaction.GetVolume()->BlockCache(),
			fBlockNumber, transaction.ID()) == B_OK)
		return fNode;

	return NULL;
}


const bplustree_header*
CachedNode::SetToHeader()
{
	if (fTree == NULL || fTree->fStream == NULL) {
		REPORT_ERROR(B_BAD_VALUE);
		return NULL;
	}

	Unset();

	InternalSetTo(NULL, 0LL);
	return (bplustree_header*)fNode;
}


bplustree_header*
CachedNode::SetToWritableHeader(Transaction& transaction)
{
	if (fTree == NULL || fTree->fStream == NULL) {
		REPORT_ERROR(B_BAD_VALUE);
		return NULL;
	}

	Unset();

	InternalSetTo(&transaction, 0LL);

	if (fNode != NULL && !fTree->fInTransaction) {
		transaction.AddListener(fTree);
		fTree->fInTransaction = true;

		if (!transaction.GetVolume()->IsInitializing()) {
			acquire_vnode(transaction.GetVolume()->FSVolume(),
				fTree->fStream->ID());
		}
	}

	return (bplustree_header*)fNode;
}


bplustree_node*
CachedNode::InternalSetTo(Transaction* transaction, off_t offset)
{
	fNode = NULL;
	fOffset = offset;

	off_t fileOffset;
	block_run run;
	if (offset < fTree->fStream->Size()
		&& fTree->fStream->FindBlockRun(offset, run, fileOffset) == B_OK) {
		Volume* volume = fTree->fStream->GetVolume();

		int32 blockOffset = (offset - fileOffset) / volume->BlockSize();
		fBlockNumber = volume->ToBlock(run) + blockOffset;
		uint8* block;

		if (transaction != NULL) {
			block = (uint8*)block_cache_get_writable(volume->BlockCache(),
				fBlockNumber, transaction->ID());
			fWritable = true;
		} else {
			block = (uint8*)block_cache_get(volume->BlockCache(), fBlockNumber);
			fWritable = false;
		}

		if (block != NULL) {
			// The node is somewhere in that block...
			// (confusing offset calculation)
			fNode = (bplustree_node*)(block + offset
				- (fileOffset + (blockOffset << volume->BlockShift())));
		} else
			REPORT_ERROR(B_IO_ERROR);
	}
	return fNode;
}


status_t
CachedNode::Free(Transaction& transaction, off_t offset)
{
	if (fTree == NULL || fTree->fStream == NULL || offset == BPLUSTREE_NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// TODO: scan the free nodes list and remove all nodes at the end
	// of the tree - perhaps that shouldn't be done everytime that
	// function is called, perhaps it should be done when the directory
	// inode is closed or based on some calculation or whatever...

	CachedNode cached(fTree);
	bplustree_header* header = cached.SetToWritableHeader(transaction);
	if (header == NULL)
		return B_IO_ERROR;

#if 0
	// TODO: temporarily disabled because CheckNode() doesn't like this...
	// 		Also, it's such an edge case that it's almost useless, anyway.
	// if the node is the last one in the tree, we shrink
	// the tree and file size by one node
	off_t lastOffset = header->MaximumSize() - fTree->fNodeSize;
	if (offset == lastOffset) {
		status_t status = fTree->fStream->SetFileSize(transaction, lastOffset);
		if (status != B_OK)
			return status;

		header->maximum_size = HOST_ENDIAN_TO_BFS_INT64(lastOffset);
		return B_OK;
	}
#endif

	// add the node to the free nodes list
	fNode->left_link = header->free_node_pointer;
	fNode->overflow_link = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_FREE);

	header->free_node_pointer = HOST_ENDIAN_TO_BFS_INT64(offset);
	return B_OK;
}


status_t
CachedNode::Allocate(Transaction& transaction, bplustree_node** _node,
	off_t* _offset)
{
	if (fTree == NULL || fTree->fStream == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Unset();

	bplustree_header* header;
	status_t status;

	// if there are any free nodes, recycle them
	if (SetToWritable(transaction, fTree->fHeader.FreeNode(), false) != NULL) {
		CachedNode cached(fTree);
		header = cached.SetToWritableHeader(transaction);
		if (header == NULL)
			return B_IO_ERROR;

		*_offset = header->FreeNode();
		*_node = fNode;

		// set new free node pointer
		header->free_node_pointer = fNode->left_link;
		fNode->Initialize();
		return B_OK;
	}

	// allocate space for a new node
	Inode* stream = fTree->fStream;
	if ((status = stream->Append(transaction, fTree->fNodeSize)) != B_OK)
		return status;

	CachedNode cached(fTree);
	header = cached.SetToWritableHeader(transaction);
	if (header == NULL)
		return B_IO_ERROR;

	// the maximum_size has to be changed before the call to SetTo() - or
	// else it will fail because the requested node is out of bounds
	off_t offset = header->MaximumSize();
	header->maximum_size = HOST_ENDIAN_TO_BFS_INT64(header->MaximumSize()
		+ fTree->fNodeSize);

	cached.Unset();
		// SetToWritable() below needs the new values in the tree's header

	if (SetToWritable(transaction, offset, false) == NULL)
		RETURN_ERROR(B_ERROR);

	fNode->Initialize();

	*_offset = offset;
	*_node = fNode;
	return B_OK;
}


//	#pragma mark -


BPlusTree::BPlusTree(Transaction& transaction, Inode* stream, int32 nodeSize)
	:
	fStream(NULL),
	fInTransaction(false)
{
	mutex_init(&fIteratorLock, "bfs b+tree iterator");
	SetTo(transaction, stream);
}


BPlusTree::BPlusTree(Inode* stream)
	:
	fStream(NULL),
	fInTransaction(false)
{
	mutex_init(&fIteratorLock, "bfs b+tree iterator");
	SetTo(stream);
}


BPlusTree::BPlusTree()
	:
	fStream(NULL),
	fNodeSize(BPLUSTREE_NODE_SIZE),
	fAllowDuplicates(true),
	fInTransaction(false),
	fStatus(B_NO_INIT)
{
	mutex_init(&fIteratorLock, "bfs b+tree iterator");
}


BPlusTree::~BPlusTree()
{
	// if there are any TreeIterators left, we need to stop them
	// (can happen when the tree's inode gets deleted while
	// traversing the tree - a TreeIterator doesn't lock the inode)
	mutex_lock(&fIteratorLock);

	SinglyLinkedList<TreeIterator>::Iterator iterator
		= fIterators.GetIterator();
	while (iterator.HasNext())
		iterator.Next()->Stop();

	mutex_destroy(&fIteratorLock);

	ASSERT(!fInTransaction);
}


/*! Create a new B+Tree on the specified stream */
status_t
BPlusTree::SetTo(Transaction& transaction, Inode* stream, int32 nodeSize)
{
	// initializes in-memory B+Tree

	fStream = stream;

	CachedNode cached(this);
	bplustree_header* header = cached.SetToWritableHeader(transaction);
	if (header == NULL) {
		// allocate space for new header + node!
		fStatus = stream->SetFileSize(transaction, nodeSize * 2);
		if (fStatus != B_OK)
			RETURN_ERROR(fStatus);

		header = cached.SetToWritableHeader(transaction);
		if (header == NULL)
			RETURN_ERROR(fStatus = B_ERROR);
	}

	fAllowDuplicates = stream->IsIndex()
		|| (stream->Mode() & S_ALLOW_DUPS) != 0;

	fNodeSize = nodeSize;

	// initialize b+tree header
 	header->magic = HOST_ENDIAN_TO_BFS_INT32(BPLUSTREE_MAGIC);
 	header->node_size = HOST_ENDIAN_TO_BFS_INT32(fNodeSize);
 	header->max_number_of_levels = HOST_ENDIAN_TO_BFS_INT32(1);
 	header->data_type = HOST_ENDIAN_TO_BFS_INT32(ModeToKeyType(stream->Mode()));
 	header->root_node_pointer = HOST_ENDIAN_TO_BFS_INT64(nodeSize);
 	header->free_node_pointer
 		= HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
 	header->maximum_size = HOST_ENDIAN_TO_BFS_INT64(nodeSize * 2);

	cached.Unset();

	// initialize b+tree root node
	cached.SetToWritable(transaction, fHeader.RootNode(), false);
	if (cached.Node() == NULL)
		RETURN_ERROR(B_IO_ERROR);

	cached.Node()->Initialize();

	return fStatus = B_OK;
}


status_t
BPlusTree::SetTo(Inode* stream)
{
	if (stream == NULL)
		RETURN_ERROR(fStatus = B_BAD_VALUE);

	fStream = stream;

	// get on-disk B+Tree header

	CachedNode cached(this);
	const bplustree_header* header = cached.SetToHeader();
	if (header != NULL)
		memcpy(&fHeader, header, sizeof(bplustree_header));
	else
		RETURN_ERROR(fStatus = B_IO_ERROR);

	// is header valid?

	if (fHeader.MaximumSize() != stream->Size()) {
		dprintf("B+tree header size %" B_PRIdOFF " doesn't fit file size %"
			B_PRIdOFF "!\n", fHeader.MaximumSize(), stream->Size());
		// we can't change the header since we don't have a transaction
		//fHeader.maximum_size = HOST_ENDIAN_TO_BFS_INT64(stream->Size());
	}
	if (!fHeader.IsValid()) {
#ifdef DEBUG
		dump_bplustree_header(&fHeader);
		dump_block((const char*)&fHeader, 128);
#endif
		RETURN_ERROR(fStatus = B_BAD_DATA);
	}

	fNodeSize = fHeader.NodeSize();

	// validity check
	static const uint32 kToMode[] = {S_STR_INDEX, S_INT_INDEX, S_UINT_INDEX,
		S_LONG_LONG_INDEX, S_ULONG_LONG_INDEX, S_FLOAT_INDEX,
		S_DOUBLE_INDEX};
	uint32 mode = stream->Mode() & (S_STR_INDEX | S_INT_INDEX
		| S_UINT_INDEX | S_LONG_LONG_INDEX | S_ULONG_LONG_INDEX
		| S_FLOAT_INDEX | S_DOUBLE_INDEX);

	if (fHeader.DataType() > BPLUSTREE_DOUBLE_TYPE
		|| ((stream->Mode() & S_INDEX_DIR) != 0
			&& kToMode[fHeader.DataType()] != mode)
		|| !stream->IsContainer()) {
		D(	dump_bplustree_header(&fHeader);
			dump_inode(&stream->Node());
		);
		RETURN_ERROR(fStatus = B_BAD_TYPE);
	}

	// although it's in stat.h, the S_ALLOW_DUPS flag is obviously unused
	// in the original BFS code - we will honour it nevertheless
	fAllowDuplicates = stream->IsIndex()
		|| (stream->Mode() & S_ALLOW_DUPS) != 0;

	cached.SetTo(fHeader.RootNode());
	RETURN_ERROR(fStatus = cached.Node() ? B_OK : B_BAD_DATA);
}


status_t
BPlusTree::InitCheck()
{
	return fStatus;
}


status_t
BPlusTree::Validate(bool repair, bool& _errorsFound)
{
	TreeCheck check(this);
	if (check.InitCheck() != B_OK)
		return B_NO_MEMORY;

	check.SetVisited(0);

	// Walk the free nodes

	CachedNode cached(this);
	off_t freeOffset = fHeader.FreeNode();
	while (freeOffset > 0) {
		const bplustree_node* node;
		status_t status = cached.SetTo(freeOffset, &node, false);
		if (status != B_OK) {
			if (status == B_IO_ERROR)
				return B_IO_ERROR;

			dprintf("inode %" B_PRIdOFF ": free node at %" B_PRIdOFF " could "
				"not be read: %s\n", fStream->ID(), freeOffset,
				strerror(status));
			check.FoundError();
			break;
		}

		if (check.Visited(freeOffset)) {
			dprintf("inode %" B_PRIdOFF ": free node at %" B_PRIdOFF
				" circular!\n", fStream->ID(), freeOffset);
			// TODO: if 'repair' is true, we could collect all unvisited nodes
			// at the end, and put the into the free list
			check.FoundError();
			break;
		}

		check.SetVisited(freeOffset);

		if (node->OverflowLink() != BPLUSTREE_FREE) {
			dprintf("inode %" B_PRIdOFF ": free node at %" B_PRIdOFF
				" misses free mark!\n", fStream->ID(), freeOffset);
		}
		freeOffset = node->LeftLink();
	}

	// Iterate over the complete tree recursively

	const bplustree_node* root;
	status_t status = cached.SetTo(fHeader.RootNode(), &root, true);
	if (status != B_OK)
		return status;

	status = _ValidateChildren(check, 0, fHeader.RootNode(), NULL, 0, root);

	if (check.ErrorsFound())
		_errorsFound = true;

	if (status != B_OK)
		return status;

	if (check.MaxLevels() + 1 != fHeader.MaxNumberOfLevels()) {
		dprintf("inode %" B_PRIdOFF ": found %" B_PRIu32 " max levels, "
			"declared %" B_PRIu32 "!\n", fStream->ID(), check.MaxLevels(),
			fHeader.MaxNumberOfLevels());
	}

	if (check.VisitedCount() != fHeader.MaximumSize() / fNodeSize) {
		dprintf("inode %" B_PRIdOFF ": visited %" B_PRIuSIZE " from %" B_PRIdOFF
			" nodes.\n", fStream->ID(), check.VisitedCount(),
			fHeader.MaximumSize() / fNodeSize);
	}

	return B_OK;
}


status_t
BPlusTree::MakeEmpty()
{
	// Put all nodes into the free list in order
	Transaction transaction(fStream->GetVolume(), fStream->BlockNumber());

	// Reset the header, and root node
	CachedNode cached(this);
	bplustree_header* header = cached.SetToWritableHeader(transaction);
	if (header == NULL)
		return B_IO_ERROR;

	header->max_number_of_levels = HOST_ENDIAN_TO_BFS_INT32(1);
	header->root_node_pointer = HOST_ENDIAN_TO_BFS_INT64(NodeSize());
	if (fStream->Size() > NodeSize() * 2)
		header->free_node_pointer = HOST_ENDIAN_TO_BFS_INT64(2 * NodeSize());
	else {
		header->free_node_pointer
			= HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
	}

	bplustree_node* node = cached.SetToWritable(transaction, NodeSize(), false);
	if (node == NULL)
		return B_IO_ERROR;

	node->left_link = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
	node->right_link = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
	node->overflow_link = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
	node->all_key_count = 0;
	node->all_key_length = 0;

	for (off_t offset = 2 * NodeSize(); offset < fStream->Size();
			offset += NodeSize()) {
		bplustree_node* node = cached.SetToWritable(transaction, offset, false);
		if (node == NULL) {
			dprintf("--> could not open %lld\n", offset);
			return B_IO_ERROR;
		}
		if (offset < fStream->Size() - NodeSize())
			node->left_link = HOST_ENDIAN_TO_BFS_INT64(offset + NodeSize());
		else
			node->left_link = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);

		node->overflow_link = HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_FREE);

		// It's not important to write it out in a single transaction; just
		// make sure it doesn't get too large
		if (offset % (1024 * 1024) == 0) {
			transaction.Done();
			transaction.Start(fStream->GetVolume(), fStream->BlockNumber());
		}
	}

	return transaction.Done();
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


//	#pragma mark - TransactionListener implementation


void
BPlusTree::TransactionDone(bool success)
{
	if (!success) {
		// update header from disk
		CachedNode cached(this);
		const bplustree_header* header = cached.SetToHeader();
		if (header != NULL)
			memcpy(&fHeader, header, sizeof(bplustree_header));
	}
}


void
BPlusTree::RemovedFromTransaction()
{
	fInTransaction = false;

	if (!fStream->GetVolume()->IsInitializing())
		put_vnode(fStream->GetVolume()->FSVolume(), fStream->ID());
}


//	#pragma mark -


void
BPlusTree::_UpdateIterators(off_t offset, off_t nextOffset, uint16 keyIndex,
	uint16 splitAt, int8 change)
{
	// Although every iterator which is affected by this update currently
	// waits on a semaphore, other iterators could be added/removed at
	// any time, so we need to protect this loop
	MutexLocker _(fIteratorLock);

	SinglyLinkedList<TreeIterator>::Iterator iterator
		= fIterators.GetIterator();
	while (iterator.HasNext())
		iterator.Next()->Update(offset, nextOffset, keyIndex, splitAt, change);
}


void
BPlusTree::_AddIterator(TreeIterator* iterator)
{
	MutexLocker _(fIteratorLock);
	fIterators.Add(iterator);
}


void
BPlusTree::_RemoveIterator(TreeIterator* iterator)
{
	MutexLocker _(fIteratorLock);
	fIterators.Remove(iterator);
}


int32
BPlusTree::_CompareKeys(const void* key1, int keyLength1, const void* key2,
	int keyLength2)
{
	type_code type = 0;
	switch (fHeader.DataType()) {
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
BPlusTree::_FindKey(const bplustree_node* node, const uint8* key,
	uint16 keyLength, uint16* _index, off_t* _next)
{
#ifdef DEBUG
	NodeChecker checker(node, fNodeSize, "find");
#endif

	if (node->all_key_count == 0) {
		if (_index)
			*_index = 0;
		if (_next)
			*_next = node->OverflowLink();
		return B_ENTRY_NOT_FOUND;
	}

	off_t* values = node->Values();
	int16 saveIndex = -1;

	// binary search in the key array
	for (int16 first = 0, last = node->NumKeys() - 1; first <= last;) {
		uint16 i = (first + last) >> 1;

		uint16 searchLength;
		uint8* searchKey = node->KeyAt(i, &searchLength);
		if (searchKey + searchLength + sizeof(off_t) + sizeof(uint16)
				> (uint8*)node + fNodeSize
			|| searchLength > BPLUSTREE_MAX_KEY_LENGTH) {
			fStream->GetVolume()->Panic();
			RETURN_ERROR(B_BAD_DATA);
		}

		int32 cmp = _CompareKeys(key, keyLength, searchKey, searchLength);
		if (cmp < 0) {
			last = i - 1;
			saveIndex = i;
		} else if (cmp > 0) {
			saveIndex = first = i + 1;
		} else {
			if (_index)
				*_index = i;
			if (_next)
				*_next = BFS_ENDIAN_TO_HOST_INT64(values[i]);
			return B_OK;
		}
	}

	if (_index)
		*_index = saveIndex;
	if (_next) {
		if (saveIndex == node->NumKeys())
			*_next = node->OverflowLink();
		else
			*_next = BFS_ENDIAN_TO_HOST_INT64(values[saveIndex]);
	}
	return B_ENTRY_NOT_FOUND;
}


/*!	Prepares the stack to contain all nodes that were passed while
	following the key, from the root node to the leaf node that could
	or should contain that key.
*/
status_t
BPlusTree::_SeekDown(Stack<node_and_key>& stack, const uint8* key,
	uint16 keyLength)
{
	// set the root node to begin with
	node_and_key nodeAndKey;
	nodeAndKey.nodeOffset = fHeader.RootNode();

	CachedNode cached(this);
	const bplustree_node* node;
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
		status_t status = _FindKey(node, key, keyLength, &nodeAndKey.keyIndex,
			&nextOffset);

		if (status == B_ENTRY_NOT_FOUND && nextOffset == nodeAndKey.nodeOffset)
			RETURN_ERROR(B_ERROR);

		if ((uint32)stack.CountItems() > fHeader.MaxNumberOfLevels()) {
			dprintf("BPlusTree::_SeekDown() node walked too deep.\n");
			break;
		}

		// put the node offset & the correct keyIndex on the stack
		stack.Push(nodeAndKey);

		nodeAndKey.nodeOffset = nextOffset;
	}

	FATAL(("BPlusTree::_SeekDown() could not open node %" B_PRIdOFF ", inode %"
		B_PRIdOFF "\n", nodeAndKey.nodeOffset, fStream->ID()));
	return B_ERROR;
}


/*!	This will find a free duplicate fragment in the given bplustree_node.
	The CachedNode will be set to the writable fragment on success.
*/
status_t
BPlusTree::_FindFreeDuplicateFragment(Transaction& transaction,
	const bplustree_node* node, CachedNode& cached,
	off_t* _offset, bplustree_node** _fragment, uint32* _index)
{
	off_t* values = node->Values();
	for (int32 i = 0; i < node->NumKeys(); i++) {
		off_t value = BFS_ENDIAN_TO_HOST_INT64(values[i]);

		// does the value link to a duplicate fragment?
		if (bplustree_node::LinkType(value) != BPLUSTREE_DUPLICATE_FRAGMENT)
			continue;

		const bplustree_node* fragment = cached.SetTo(
			bplustree_node::FragmentOffset(value), false);
		if (fragment == NULL) {
			FATAL(("Could not get duplicate fragment at %" B_PRIdOFF ", inode %"
				B_PRIdOFF "\n", value, fStream->ID()));
			continue;
		}

		// see if there is some space left for us
		uint32 num = bplustree_node::MaxFragments(fNodeSize);
		for (uint32 j = 0; j < num; j++) {
			duplicate_array* array = fragment->FragmentAt(j);

			if (array->IsEmpty()) {
				// found an unused fragment
				*_fragment = cached.MakeWritable(transaction);
				if (*_fragment == NULL)
					return B_IO_ERROR;

				*_offset = bplustree_node::FragmentOffset(value);
				*_index = j;
				return B_OK;
			}
		}
	}
	return B_ENTRY_NOT_FOUND;
}


status_t
BPlusTree::_InsertDuplicate(Transaction& transaction, CachedNode& cached,
	const bplustree_node* node, uint16 index, off_t value)
{
	CachedNode cachedDuplicate(this);
	off_t* values = node->Values();
	off_t oldValue = BFS_ENDIAN_TO_HOST_INT64(values[index]);
	status_t status;
	off_t offset;

	if (bplustree_node::IsDuplicate(oldValue)) {
		// If it's a duplicate fragment, try to insert it into that, or if it
		// doesn't fit anymore, create a new duplicate node

		if (bplustree_node::LinkType(oldValue)
				== BPLUSTREE_DUPLICATE_FRAGMENT) {
			bplustree_node* duplicate = cachedDuplicate.SetToWritable(
				transaction, bplustree_node::FragmentOffset(oldValue), false);
			if (duplicate == NULL)
				return B_IO_ERROR;

			duplicate_array* array = duplicate->FragmentAt(
				bplustree_node::FragmentIndex(oldValue));
			int32 arrayCount = array->Count();
			if (arrayCount > NUM_FRAGMENT_VALUES || arrayCount < 1) {
				FATAL(("_InsertDuplicate: Invalid array[%d] size in fragment "
					"%" B_PRIdOFF " == %" B_PRId32 ", inode %" B_PRIdOFF "!\n",
					(int)bplustree_node::FragmentIndex(oldValue),
					bplustree_node::FragmentOffset(oldValue), arrayCount,
					fStream->ID()));
				return B_BAD_DATA;
			}

			if (arrayCount < NUM_FRAGMENT_VALUES) {
				array->Insert(value);
			} else {
				// Test if the fragment will be empty if we remove this key's
				// values
				if (duplicate->FragmentsUsed(fNodeSize) < 2) {
					// The node will be empty without our values, so let us
					// reuse it as a duplicate node
					offset = bplustree_node::FragmentOffset(oldValue);

					memmove(duplicate->DuplicateArray(), array,
						(NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));
					duplicate->left_link = duplicate->right_link
						= HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);

					array = duplicate->DuplicateArray();
					array->Insert(value);
				} else {
					// Create a new duplicate node

					cachedDuplicate.UnsetUnchanged(transaction);
						// The old duplicate has not been touched, so we can
						// reuse it

					bplustree_node* newDuplicate;
					status = cachedDuplicate.Allocate(transaction,
						&newDuplicate, &offset);
					if (status != B_OK)
						RETURN_ERROR(status);

					// Copy the array from the fragment node to the duplicate
					// node and free the old entry (by zero'ing all values)
					newDuplicate->overflow_link = array->count;
					memcpy(&newDuplicate->all_key_count, &array->values[0],
						array->Count() * sizeof(off_t));
					memset(array, 0, (NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));

					array = newDuplicate->DuplicateArray();
					array->Insert(value);
				}

				// Update the main pointer to link to a duplicate node
				if (cached.MakeWritable(transaction) == NULL)
					return B_IO_ERROR;

				values[index]
					= HOST_ENDIAN_TO_BFS_INT64(bplustree_node::MakeLink(
						BPLUSTREE_DUPLICATE_NODE, offset));
			}

			return B_OK;
		}

		// Put the value into a dedicated duplicate node

		// search for free space in the duplicate nodes of that key
		duplicate_array* array;
		int32 arrayCount;
		const bplustree_node* duplicate;
		off_t duplicateOffset;
		do {
			duplicateOffset = bplustree_node::FragmentOffset(oldValue);
			duplicate = cachedDuplicate.SetTo(duplicateOffset, false);
			if (duplicate == NULL)
				return B_IO_ERROR;

			array = duplicate->DuplicateArray();
			arrayCount =array->Count();
			if (arrayCount > NUM_DUPLICATE_VALUES || arrayCount < 0) {
				FATAL(("_InsertDuplicate: Invalid array size in duplicate %"
					B_PRIdOFF " == %" B_PRId32 ", inode %" B_PRIdOFF "!\n",
					duplicateOffset, arrayCount, fStream->ID()));
				return B_BAD_DATA;
			}
		} while (arrayCount >= NUM_DUPLICATE_VALUES
				&& (oldValue = duplicate->RightLink()) != BPLUSTREE_NULL);

		bplustree_node* writableDuplicate
			= cachedDuplicate.MakeWritable(transaction);
		if (writableDuplicate == NULL)
			return B_IO_ERROR;

		if (arrayCount < NUM_DUPLICATE_VALUES) {
			array = writableDuplicate->DuplicateArray();
			array->Insert(value);
		} else {
			// no space left - add a new duplicate node

			bplustree_node* newDuplicate;
			status = cachedDuplicate.Allocate(transaction, &newDuplicate,
				&offset);
			if (status != B_OK)
				RETURN_ERROR(status);

			// link the two nodes together
			writableDuplicate->right_link = HOST_ENDIAN_TO_BFS_INT64(offset);
			newDuplicate->left_link = HOST_ENDIAN_TO_BFS_INT64(duplicateOffset);

			array = newDuplicate->DuplicateArray();
			array->count = 0;
			array->Insert(value);
		}
		return B_OK;
	}

	// Search for a free duplicate fragment or create a new one
	// to insert the duplicate value into

	uint32 fragmentIndex = 0;
	bplustree_node* fragment;
	if (_FindFreeDuplicateFragment(transaction, node, cachedDuplicate,
			&offset, &fragment, &fragmentIndex) != B_OK) {
		// allocate a new duplicate fragment node
		status = cachedDuplicate.Allocate(transaction, &fragment, &offset);
		if (status != B_OK)
			RETURN_ERROR(status);

		memset(fragment, 0, fNodeSize);
	}

	duplicate_array* array = fragment->FragmentAt(fragmentIndex);
	array->Insert(oldValue);
	array->Insert(value);

	if (cached.MakeWritable(transaction) == NULL)
		return B_IO_ERROR;

	values[index] = HOST_ENDIAN_TO_BFS_INT64(bplustree_node::MakeLink(
		BPLUSTREE_DUPLICATE_FRAGMENT, offset, fragmentIndex));

	return B_OK;
}


void
BPlusTree::_InsertKey(bplustree_node* node, uint16 index, uint8* key,
	uint16 keyLength, off_t value)
{
	// should never happen, but who knows?
	if (index > node->NumKeys())
		return;

	off_t* values = node->Values();
	uint16* keyLengths = node->KeyLengths();
	uint8* keys = node->Keys();

	node->all_key_count = HOST_ENDIAN_TO_BFS_INT16(node->NumKeys() + 1);
	node->all_key_length = HOST_ENDIAN_TO_BFS_INT16(node->AllKeyLength()
		+ keyLength);

	off_t* newValues = node->Values();
	uint16* newKeyLengths = node->KeyLengths();

	// move values and copy new value into them
	memmove(newValues + index + 1, values + index,
		sizeof(off_t) * (node->NumKeys() - 1 - index));
	memmove(newValues, values, sizeof(off_t) * index);

	newValues[index] = HOST_ENDIAN_TO_BFS_INT64(value);

	// move and update key length index
	for (uint16 i = node->NumKeys(); i-- > index + 1;) {
		newKeyLengths[i] = HOST_ENDIAN_TO_BFS_INT16(
			BFS_ENDIAN_TO_HOST_INT16(keyLengths[i - 1]) + keyLength);
	}
	memmove(newKeyLengths, keyLengths, sizeof(uint16) * index);

	int32 keyStart;
	newKeyLengths[index] = HOST_ENDIAN_TO_BFS_INT16(keyLength
		+ (keyStart = index > 0
			? BFS_ENDIAN_TO_HOST_INT16(newKeyLengths[index - 1]) : 0));

	// move keys and copy new key into them
	uint16 length = BFS_ENDIAN_TO_HOST_INT16(newKeyLengths[index]);
	int32 size = node->AllKeyLength() - length;
	if (size > 0)
		memmove(keys + length, keys + length - keyLength, size);

	memcpy(keys + keyStart, key, keyLength);
}


/*!	Splits the \a node into two halves - the other half will be put into
	\a other. It also takes care to create a new overflow link if the node
	to split is an index node.
*/
status_t
BPlusTree::_SplitNode(bplustree_node* node, off_t nodeOffset,
	bplustree_node* other, off_t otherOffset, uint16* _keyIndex, uint8* key,
	uint16* _keyLength, off_t* _value)
{
	if (*_keyIndex > node->NumKeys() + 1)
		return B_BAD_VALUE;

	uint16* inKeyLengths = node->KeyLengths();
	off_t* inKeyValues = node->Values();
	uint8* inKeys = node->Keys();
	uint8* outKeys = other->Keys();
	int32 keyIndex = *_keyIndex;	// can become less than zero!

	if (keyIndex > node->NumKeys()) {
		FATAL(("key index out of bounds: %d, num keys: %u, inode %" B_PRIdOFF
			"\n", (int)keyIndex, node->NumKeys(), fStream->ID()));
		return B_BAD_VALUE;
	}

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
		if (!bytes) {
			bytesBefore = in > 0
				? BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in - 1]) : 0;
		}

		if (in == keyIndex && !bytes) {
			bytes = *_keyLength;
		} else {
			if (keyIndex < out) {
				bytesAfter = BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in])
					- bytesBefore;
			}

			in++;
		}
		out++;

		if (key_align(sizeof(bplustree_node) + bytesBefore + bytesAfter + bytes)
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
	other->all_key_length = HOST_ENDIAN_TO_BFS_INT16(bytes + bytesBefore
		+ bytesAfter);
	other->all_key_count = HOST_ENDIAN_TO_BFS_INT16(out);

	uint16* outKeyLengths = other->KeyLengths();
	off_t* outKeyValues = other->Values();
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
			memcpy(outKeys + bytesBefore + bytes, inKeys + bytesBefore,
				bytesAfter);
			keys = out - keyIndex - 1;
			for (int32 i = 0;i < keys;i++) {
				outKeyLengths[keyIndex + i + 1] = HOST_ENDIAN_TO_BFS_INT16(
					BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[keyIndex + i])
						+ bytes);
			}
			memcpy(outKeyValues + keyIndex + 1, inKeyValues + keyIndex,
				keys * sizeof(off_t));
		}
	}

	// if the new key was already inserted, we shouldn't use it again
	if (in != out)
		keyIndex--;

	int32 total = bytesBefore + bytesAfter;

	// these variables are for the key that will be returned
	// to the parent node
	uint8* newKey = NULL;
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
			uint8* droppedKey = node->KeyAt(in, &newLength);
			if (droppedKey + newLength + sizeof(off_t) + sizeof(uint16)
					> (uint8*)node + fNodeSize
				|| newLength > BPLUSTREE_MAX_KEY_LENGTH) {
				fStream->GetVolume()->Panic();
				RETURN_ERROR(B_BAD_DATA);
			}
			newKey = (uint8*)malloc(newLength);
			if (newKey == NULL)
				return B_NO_MEMORY;

			newAllocated = true;
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
			bytesBefore = in > skip
				? BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in - 1]) : 0;
			bytes = *_keyLength;
			out++;
		} else {
			if (in < node->NumKeys()) {
				inKeyLengths[in] = HOST_ENDIAN_TO_BFS_INT16(
					BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in]) - total);

				if (bytes) {
					inKeyLengths[in] = HOST_ENDIAN_TO_BFS_INT16(
						BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in]) + bytes);

					bytesAfter = BFS_ENDIAN_TO_HOST_INT16(inKeyLengths[in])
						- bytesBefore - bytes;
				}
				out++;
			}
			in++;
		}
	}

	// adjust the byte counts (since we were a bit lazy in the loop)
	if (keyIndex < skip)
		bytesBefore = node->AllKeyLength() - total;

	if (bytesBefore < 0 || bytesAfter < 0) {
		if (newAllocated)
			free(newKey);
		return B_BAD_DATA;
	}

	node->left_link = HOST_ENDIAN_TO_BFS_INT64(otherOffset);
		// right link, and overflow link can stay the same
	node->all_key_length = HOST_ENDIAN_TO_BFS_INT16(bytes + bytesBefore
		+ bytesAfter);
	node->all_key_count = HOST_ENDIAN_TO_BFS_INT16(out);

	// array positions have changed
	outKeyLengths = node->KeyLengths();
	outKeyValues = node->Values();

	// move the keys in the old node: the order is important here,
	// because we don't want to overwrite any contents

	keys = keyIndex <= skip ? out : keyIndex - skip;
	keyIndex -= skip;
	in = out - keyIndex - 1;
		// Note: keyIndex and in will contain invalid values when the new key
		// went to the other node. But in this case bytes and bytesAfter are
		// 0 and subsequently we never use keyIndex and in.

	if (bytesBefore)
		memmove(inKeys, inKeys + total, bytesBefore);
	if (bytesAfter) {
		memmove(inKeys + bytesBefore + bytes, inKeys + total + bytesBefore,
			bytesAfter);
	}

	if (bytesBefore)
		memmove(outKeyLengths, inKeyLengths + skip, keys * sizeof(uint16));
	if (bytesAfter) {
		// if byteAfter is > 0, keyIndex is larger than skip
		memmove(outKeyLengths + keyIndex + 1, inKeyLengths + skip + keyIndex,
			in * sizeof(uint16));
	}

	if (bytesBefore)
		memmove(outKeyValues, inKeyValues + skip, keys * sizeof(off_t));
	if (bytesAfter) {
		memmove(outKeyValues + keyIndex + 1, inKeyValues + skip + keyIndex,
			in * sizeof(off_t));
	}

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


/*!	This inserts a key into the tree. The changes made to the tree will
	all be part of the \a transaction.
	You need to have the inode write locked.
*/
status_t
BPlusTree::Insert(Transaction& transaction, const uint8* key, uint16 keyLength,
	off_t value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH
		|| keyLength > BPLUSTREE_MAX_KEY_LENGTH)
		RETURN_ERROR(B_BAD_VALUE);
#ifdef DEBUG
	if (value < 0)
		panic("tried to insert invalid value %Ld!\n", value);
#endif

	ASSERT_WRITE_LOCKED_INODE(fStream);

	Stack<node_and_key> stack;
	if (_SeekDown(stack, key, keyLength) != B_OK)
		RETURN_ERROR(B_ERROR);

	uint8 keyBuffer[BPLUSTREE_MAX_KEY_LENGTH + 1];

	memcpy(keyBuffer, key, keyLength);
	keyBuffer[keyLength] = 0;

	node_and_key nodeAndKey;
	const bplustree_node* node;

	CachedNode cached(this);
	while (stack.Pop(&nodeAndKey)
		&& (node = cached.SetTo(nodeAndKey.nodeOffset)) != NULL) {
#ifdef DEBUG
		NodeChecker checker(node, fNodeSize, "insert");
#endif
		if (node->IsLeaf()) {
			// first round, check for duplicate entries
			status_t status = _FindKey(node, key, keyLength,
				&nodeAndKey.keyIndex);

			// is this a duplicate entry?
			if (status == B_OK) {
				if (fAllowDuplicates) {
					status = _InsertDuplicate(transaction, cached, node,
						nodeAndKey.keyIndex, value);
					if (status != B_OK)
						RETURN_ERROR(status);
					return B_OK;
				}

				return B_NAME_IN_USE;
			}
		}

		bplustree_node* writableNode = cached.MakeWritable(transaction);
		if (writableNode == NULL)
			return B_IO_ERROR;

		// is the node big enough to hold the pair?
		if (int32(key_align(sizeof(bplustree_node)
				+ writableNode->AllKeyLength() + keyLength)
				+ (writableNode->NumKeys() + 1) * (sizeof(uint16)
				+ sizeof(off_t))) < fNodeSize) {
			_InsertKey(writableNode, nodeAndKey.keyIndex,
				keyBuffer, keyLength, value);
			_UpdateIterators(nodeAndKey.nodeOffset, BPLUSTREE_NULL,
				nodeAndKey.keyIndex, 0, 1);

			return B_OK;
		} else {
			CachedNode cachedNewRoot(this);
			CachedNode cachedOther(this);

			// do we need to allocate a new root node? if so, then do
			// it now
			off_t newRoot = BPLUSTREE_NULL;
			if (nodeAndKey.nodeOffset == fHeader.RootNode()) {
				bplustree_node* root;
				status_t status = cachedNewRoot.Allocate(transaction, &root,
					&newRoot);
				if (status != B_OK) {
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
			bplustree_node* other;
			off_t otherOffset;
			status_t status = cachedOther.Allocate(transaction, &other,
				&otherOffset);
			if (status != B_OK) {
				cachedNewRoot.Free(transaction, newRoot);
				RETURN_ERROR(status);
			}

			if (_SplitNode(writableNode, nodeAndKey.nodeOffset, other,
					otherOffset, &nodeAndKey.keyIndex, keyBuffer, &keyLength,
					&value) != B_OK) {
				// free root node & other node here
				cachedOther.Free(transaction, otherOffset);
				cachedNewRoot.Free(transaction, newRoot);

				RETURN_ERROR(B_ERROR);
			}
#ifdef DEBUG
			checker.Check("insert split");
			NodeChecker otherChecker(other, fNodeSize, "insert split other");
#endif

			_UpdateIterators(nodeAndKey.nodeOffset, otherOffset,
				nodeAndKey.keyIndex, writableNode->NumKeys(), 1);

			// update the right link of the node in the left of the new node
			if ((other = cachedOther.SetToWritable(transaction,
					other->LeftLink())) != NULL) {
				other->right_link = HOST_ENDIAN_TO_BFS_INT64(otherOffset);
			}

			// create a new root if necessary
			if (newRoot != BPLUSTREE_NULL) {
				bplustree_node* root = cachedNewRoot.Node();

				_InsertKey(root, 0, keyBuffer, keyLength,
					writableNode->LeftLink());
				root->overflow_link = HOST_ENDIAN_TO_BFS_INT64(
					nodeAndKey.nodeOffset);

				CachedNode cached(this);
				bplustree_header* header
					= cached.SetToWritableHeader(transaction);
				if (header == NULL)
					return B_IO_ERROR;

				// finally, update header to point to the new root
				header->root_node_pointer = HOST_ENDIAN_TO_BFS_INT64(newRoot);
				header->max_number_of_levels = HOST_ENDIAN_TO_BFS_INT32(
					header->MaxNumberOfLevels() + 1);

				return B_OK;
			}
		}
	}
	RETURN_ERROR(B_ERROR);
}


/*!	Removes the duplicate index/value pair from the tree.
	It's part of the private tree interface.
*/
status_t
BPlusTree::_RemoveDuplicate(Transaction& transaction,
	const bplustree_node* node, CachedNode& cached, uint16 index,
	off_t value)
{
	off_t* values = node->Values();
	off_t oldValue = BFS_ENDIAN_TO_HOST_INT64(values[index]);

	CachedNode cachedDuplicate(this);
	off_t duplicateOffset = bplustree_node::FragmentOffset(oldValue);
	bplustree_node* duplicate = cachedDuplicate.SetToWritable(transaction,
		duplicateOffset, false);
	if (duplicate == NULL)
		return B_IO_ERROR;

	// if it's a duplicate fragment, remove the entry from there
	if (bplustree_node::LinkType(oldValue) == BPLUSTREE_DUPLICATE_FRAGMENT) {
		duplicate_array* array = duplicate->FragmentAt(
			bplustree_node::FragmentIndex(oldValue));
		int32 arrayCount = array->Count();

		if (arrayCount > NUM_FRAGMENT_VALUES || arrayCount <= 1) {
			FATAL(("_RemoveDuplicate: Invalid array[%d] size in fragment %"
				B_PRIdOFF " == %" B_PRId32 ", inode %" B_PRIdOFF "!\n",
				(int)bplustree_node::FragmentIndex(oldValue), duplicateOffset,
				arrayCount, fStream->ID()));
			return B_BAD_DATA;
		}
		if (!array->Remove(value)) {
			FATAL(("Oh no, value %" B_PRIdOFF " not found in fragments of node "
				"%" B_PRIdOFF "..., inode %" B_PRIdOFF "\n", value,
				duplicateOffset, fStream->ID()));
			return B_ENTRY_NOT_FOUND;
		}

		// remove the array from the fragment node if it is empty
		if (--arrayCount == 1) {
			// set the link to the remaining value
			if (cached.MakeWritable(transaction) == NULL)
				return B_IO_ERROR;

			values[index] = array->values[0];

			// Remove the whole fragment node, if this was the only array,
			// otherwise free just the array
			if (duplicate->FragmentsUsed(fNodeSize) == 1) {
				status_t status = cachedDuplicate.Free(transaction,
					duplicateOffset);
				if (status != B_OK)
					return status;
			} else
				array->count = 0;
		}
		return B_OK;
	}

	// Remove value from a duplicate node!

	duplicate_array* array = NULL;
	int32 arrayCount = 0;

	if (duplicate->LeftLink() != BPLUSTREE_NULL) {
		FATAL(("invalid duplicate node: first left link points to %" B_PRIdOFF
			", inode %" B_PRIdOFF "!\n", duplicate->LeftLink(), fStream->ID()));
		return B_BAD_DATA;
	}

	// Search the duplicate nodes until the entry could be found (and removed)
	while (duplicate != NULL) {
		array = duplicate->DuplicateArray();
		arrayCount = array->Count();

		if (arrayCount > NUM_DUPLICATE_VALUES || arrayCount < 0) {
			FATAL(("_RemoveDuplicate: Invalid array size in duplicate %"
				B_PRIdOFF " == %" B_PRId32 ", inode %" B_PRIdOFF "!\n",
				duplicateOffset, arrayCount, fStream->ID()));
			return B_BAD_DATA;
		}

		if (array->Remove(value)) {
			arrayCount--;
			break;
		}

		if ((duplicateOffset = duplicate->RightLink()) == BPLUSTREE_NULL)
			RETURN_ERROR(B_ENTRY_NOT_FOUND);

		cachedDuplicate.UnsetUnchanged(transaction);
		duplicate = cachedDuplicate.SetToWritable(transaction, duplicateOffset,
			false);
	}
	if (duplicate == NULL)
		RETURN_ERROR(B_IO_ERROR);

	// The entry got removed from the duplicate node, but we might want to free
	// it now in case it's empty

	while (true) {
		off_t left = duplicate->LeftLink();
		off_t right = duplicate->RightLink();
		bool isLast = left == BPLUSTREE_NULL && right == BPLUSTREE_NULL;

		if ((isLast && arrayCount == 1) || arrayCount == 0) {
			// Free empty duplicate page, link their siblings together, and
			// update the duplicate link if needed (ie. when we either remove
			// the last duplicate node or have a new first one)

			if (left == BPLUSTREE_NULL) {
				// the duplicate link points to us
				if (cached.MakeWritable(transaction) == NULL)
					return B_IO_ERROR;

				if (arrayCount == 1) {
					// This is the last node, and there is only one value left;
					// replace the duplicate link with that value, it's no
					// duplicate anymore
					values[index] = array->values[0];
				} else {
					// Move the duplicate link to the next node
					values[index] = HOST_ENDIAN_TO_BFS_INT64(
						bplustree_node::MakeLink(
							BPLUSTREE_DUPLICATE_NODE, right));
				}
			}

			status_t status = cachedDuplicate.Free(transaction,
				duplicateOffset);
			if (status != B_OK)
				return status;

			if (left != BPLUSTREE_NULL
				&& (duplicate = cachedDuplicate.SetToWritable(transaction, left,
						false)) != NULL) {
				duplicate->right_link = HOST_ENDIAN_TO_BFS_INT64(right);

				// If the next node is the last node, we need to free that node
				// and convert the duplicate entry back into a normal entry
				array = duplicate->DuplicateArray();
				arrayCount = array->Count();
				if (right == BPLUSTREE_NULL
					&& duplicate->LeftLink() == BPLUSTREE_NULL
					&& arrayCount <= NUM_FRAGMENT_VALUES) {
					duplicateOffset = left;
					continue;
				}
			}
			if (right != BPLUSTREE_NULL
				&& (duplicate = cachedDuplicate.SetToWritable(transaction,
						right, false)) != NULL) {
				duplicate->left_link = HOST_ENDIAN_TO_BFS_INT64(left);

				// Again, we may need to turn the duplicate entry back into a
				// normal entry
				array = duplicate->DuplicateArray();
				arrayCount = array->Count();
				if (left == BPLUSTREE_NULL
					&& duplicate->RightLink() == BPLUSTREE_NULL
					&& arrayCount <= NUM_FRAGMENT_VALUES) {
					duplicateOffset = right;
					continue;
				}
			}
			return B_OK;
		}
		if (isLast && arrayCount <= NUM_FRAGMENT_VALUES) {
			// If the number of entries fits in a duplicate fragment, then
			// either find a free fragment node, or convert this node to a
			// fragment node.
			CachedNode cachedOther(this);

			bplustree_node* fragment = NULL;
			uint32 fragmentIndex = 0;
			off_t offset;
			if (_FindFreeDuplicateFragment(transaction, node, cachedOther,
					&offset, &fragment, &fragmentIndex) == B_OK) {
				// move to other node
				duplicate_array* target = fragment->FragmentAt(fragmentIndex);
				memcpy(target, array,
					(NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));

				cachedDuplicate.Free(transaction, duplicateOffset);
				duplicateOffset = offset;
			} else {
				// convert node
				memmove(duplicate, array,
					(NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));
				memset((off_t*)duplicate + NUM_FRAGMENT_VALUES + 1, 0,
					fNodeSize - (NUM_FRAGMENT_VALUES + 1) * sizeof(off_t));
			}

			if (cached.MakeWritable(transaction) == NULL)
				return B_IO_ERROR;

			values[index] = HOST_ENDIAN_TO_BFS_INT64(bplustree_node::MakeLink(
				BPLUSTREE_DUPLICATE_FRAGMENT, duplicateOffset, fragmentIndex));
		}
		return B_OK;
	}
}


/*!	Removes the key with the given index from the specified node.
	Since it has to get the key from the node anyway (to obtain it's
	pointer), it's not needed to pass the key & its length, although
	the calling method (BPlusTree::Remove()) have this data.
*/
void
BPlusTree::_RemoveKey(bplustree_node* node, uint16 index)
{
	// should never happen, but who knows?
	if (index > node->NumKeys() && node->NumKeys() > 0) {
		FATAL(("Asked me to remove key outer limits: %u, inode %" B_PRIdOFF
			"\n", index, fStream->ID()));
		return;
	}

	off_t* values = node->Values();

	// if we would have to drop the overflow link, drop
	// the last key instead and update the overflow link
	// to the value of that one
	if (!node->IsLeaf() && index == node->NumKeys())
		node->overflow_link = values[--index];

	uint16 length;
	uint8* key = node->KeyAt(index, &length);
	if (key + length + sizeof(off_t) + sizeof(uint16) > (uint8*)node + fNodeSize
		|| length > BPLUSTREE_MAX_KEY_LENGTH) {
		FATAL(("Key length to long: %s, %u inode %" B_PRIdOFF "\n", key, length,
			fStream->ID()));
		fStream->GetVolume()->Panic();
		return;
	}

	uint16* keyLengths = node->KeyLengths();
	uint8* keys = node->Keys();

	node->all_key_count = HOST_ENDIAN_TO_BFS_INT16(node->NumKeys() - 1);
	node->all_key_length = HOST_ENDIAN_TO_BFS_INT64(
		node->AllKeyLength() - length);

	off_t* newValues = node->Values();
	uint16* newKeyLengths = node->KeyLengths();

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
	if (node->NumKeys() > index) {
		memmove(newValues + index, values + index + 1,
			(node->NumKeys() - index) * sizeof(off_t));
	}
}


/*!	Removes the specified key from the tree. The "value" parameter is only used
	for trees which allow duplicates, so you may safely ignore it.
	It's not an optional parameter, so at least you have to think about it.
	You need to have the inode write locked.
*/
status_t
BPlusTree::Remove(Transaction& transaction, const uint8* key, uint16 keyLength,
	off_t value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH
		|| keyLength > BPLUSTREE_MAX_KEY_LENGTH)
		RETURN_ERROR(B_BAD_VALUE);

	ASSERT_WRITE_LOCKED_INODE(fStream);

	Stack<node_and_key> stack;
	if (_SeekDown(stack, key, keyLength) != B_OK)
		RETURN_ERROR(B_ERROR);

	node_and_key nodeAndKey;
	const bplustree_node* node;

	CachedNode cached(this);
	while (stack.Pop(&nodeAndKey)
		&& (node = cached.SetTo(nodeAndKey.nodeOffset)) != NULL) {
#ifdef DEBUG
		NodeChecker checker(node, fNodeSize, "remove");
#endif
		if (node->IsLeaf()) {
			// first round, check for duplicate entries
			status_t status = _FindKey(node, key, keyLength,
				&nodeAndKey.keyIndex);
			if (status != B_OK)
				RETURN_ERROR(status);

			// Is this a duplicate entry?
			if (bplustree_node::IsDuplicate(BFS_ENDIAN_TO_HOST_INT64(
					node->Values()[nodeAndKey.keyIndex]))) {
				if (fAllowDuplicates) {
					return _RemoveDuplicate(transaction, node, cached,
						nodeAndKey.keyIndex, value);
				}

				FATAL(("dupliate node found where no duplicates are "
					"allowed, inode %" B_PRIdOFF "!\n", fStream->ID()));
				RETURN_ERROR(B_ERROR);
			} else {
				if (node->Values()[nodeAndKey.keyIndex] != value)
					return B_ENTRY_NOT_FOUND;

				// If we will remove the last key, the iterator will be set
				// to the next node after the current - if there aren't any
				// more nodes, we need a way to prevent the TreeIterators to
				// touch the old node again, we use BPLUSTREE_FREE for this
				off_t next = node->RightLink() == BPLUSTREE_NULL
					? BPLUSTREE_FREE : node->RightLink();
				_UpdateIterators(nodeAndKey.nodeOffset, node->NumKeys() == 1
					? next : BPLUSTREE_NULL, nodeAndKey.keyIndex, 0 , -1);
			}
		}

		bplustree_node* writableNode = cached.MakeWritable(transaction);
		if (writableNode == NULL)
			return B_IO_ERROR;

		// if it's an empty root node, we have to convert it
		// to a leaf node by dropping the overflow link, or,
		// if it's already a leaf node, just empty it
		if (nodeAndKey.nodeOffset == fHeader.RootNode()
			&& (node->NumKeys() == 0
				|| (node->NumKeys() == 1 && node->IsLeaf()))) {
			writableNode->overflow_link
				= HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
			writableNode->all_key_count = 0;
			writableNode->all_key_length = 0;

			// if we've made a leaf node out of the root node, we need
			// to reset the maximum number of levels in the header
			if (fHeader.MaxNumberOfLevels() != 1) {
				CachedNode cached(this);
				bplustree_header* header
					= cached.SetToWritableHeader(transaction);
				if (header == NULL)
					return B_IO_ERROR;

				header->max_number_of_levels = HOST_ENDIAN_TO_BFS_INT32(1);
			}
			return B_OK;
		}

		// if there is only one key left, we don't have to remove
		// it, we can just dump the node (index nodes still have
		// the overflow link, so we have to drop the last key)
		if (writableNode->NumKeys() > 1
			|| (!writableNode->IsLeaf() && writableNode->NumKeys() == 1)) {
			_RemoveKey(writableNode, nodeAndKey.keyIndex);
			return B_OK;
		}

		// when we are here, we can just free the node, but
		// we have to update the right/left link of the
		// siblings first
		CachedNode otherCached(this);
		bplustree_node* other = otherCached.SetToWritable(transaction,
			writableNode->LeftLink());
		if (other != NULL)
			other->right_link = writableNode->right_link;

		if ((other = otherCached.SetToWritable(transaction, node->RightLink()))
				!= NULL) {
			other->left_link = writableNode->left_link;
		}

		cached.Free(transaction, nodeAndKey.nodeOffset);
	}
	RETURN_ERROR(B_ERROR);
}


/*!	Replaces the value for the key in the tree.
	Returns B_OK if the key could be found and its value replaced,
	B_ENTRY_NOT_FOUND if the key couldn't be found, and other errors
	to indicate that something went terribly wrong.
	Note that this doesn't work with duplicates - it will just
	return B_BAD_TYPE if you call this function on a tree where
	duplicates are allowed.
	You need to have the inode write locked.
*/
status_t
BPlusTree::Replace(Transaction& transaction, const uint8* key, uint16 keyLength,
	off_t value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH
		|| keyLength > BPLUSTREE_MAX_KEY_LENGTH
		|| key == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	if (fAllowDuplicates)
		RETURN_ERROR(B_BAD_TYPE);

	ASSERT_WRITE_LOCKED_INODE(fStream);

	off_t nodeOffset = fHeader.RootNode();
	CachedNode cached(this);
	const bplustree_node* node;

	while ((node = cached.SetTo(nodeOffset)) != NULL) {
		uint16 keyIndex = 0;
		off_t nextOffset;
		status_t status = _FindKey(node, key, keyLength, &keyIndex,
			&nextOffset);

		if (node->OverflowLink() == BPLUSTREE_NULL) {
			if (status == B_OK) {
				bplustree_node* writableNode = cached.MakeWritable(transaction);
				if (writableNode != NULL) {
					writableNode->Values()[keyIndex]
						= HOST_ENDIAN_TO_BFS_INT64(value);
				} else
					status = B_IO_ERROR;
			}

			return status;
		} else if (nextOffset == nodeOffset)
			RETURN_ERROR(B_ERROR);

		nodeOffset = nextOffset;
	}
	RETURN_ERROR(B_ERROR);
}


/*!	Searches the key in the tree, and stores the offset found in _value,
	if successful.
	It's very similar to BPlusTree::SeekDown(), but doesn't fill a stack
	while it descends the tree.
	Returns B_OK when the key could be found, B_ENTRY_NOT_FOUND if not.
	It can also return other errors to indicate that something went wrong.
	Note that this doesn't work with duplicates - it will just return
	B_BAD_TYPE if you call this function on a tree where duplicates are
	allowed.
	You need to have the inode read or write locked.
*/
status_t
BPlusTree::Find(const uint8* key, uint16 keyLength, off_t* _value)
{
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH
		|| keyLength > BPLUSTREE_MAX_KEY_LENGTH
		|| key == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	if (fAllowDuplicates)
		RETURN_ERROR(B_BAD_TYPE);

	ASSERT_READ_LOCKED_INODE(fStream);

	off_t nodeOffset = fHeader.RootNode();
	CachedNode cached(this);
	const bplustree_node* node;

#ifdef DEBUG
	int32 levels = 0;
#endif

	while ((node = cached.SetTo(nodeOffset)) != NULL) {
		uint16 keyIndex = 0;
		off_t nextOffset;
		status_t status = _FindKey(node, key, keyLength, &keyIndex,
			&nextOffset);

#ifdef DEBUG
		levels++;
#endif
		if (node->OverflowLink() == BPLUSTREE_NULL) {
			if (status == B_OK && _value != NULL)
				*_value = BFS_ENDIAN_TO_HOST_INT64(node->Values()[keyIndex]);

#ifdef DEBUG
			if (levels != (int32)fHeader.MaxNumberOfLevels())
				DEBUGGER(("levels don't match"));
#endif
			return status;
		} else if (nextOffset == nodeOffset)
			RETURN_ERROR(B_ERROR);

		nodeOffset = nextOffset;
	}
	FATAL(("b+tree node at %" B_PRIdOFF " could not be loaded, inode %"
		B_PRIdOFF "\n", nodeOffset, fStream->ID()));
	RETURN_ERROR(B_ERROR);
}


status_t
BPlusTree::_ValidateChildren(TreeCheck& check, uint32 level, off_t offset,
	const uint8* largestKey, uint16 largestKeyLength,
	const bplustree_node* parent)
{
	if (parent->CheckIntegrity(fNodeSize) != B_OK) {
		dprintf("inode %" B_PRIdOFF ": node %" B_PRIdOFF " integrity check "
			"failed!\n", fStream->ID(), offset);
		check.FoundError();
		return B_OK;
	}
	if (level >= fHeader.MaxNumberOfLevels()) {
		dprintf("inode %" B_PRIdOFF ": maximum level surpassed at %" B_PRIdOFF
			"!\n", fStream->ID(), offset);
		check.FoundError();
		return B_OK;
	}

	check.SetLevel(level);

	if (check.Visited(offset)) {
		dprintf("inode %" B_PRIdOFF ": node %" B_PRIdOFF " already visited!\n",
			fStream->ID(), offset);
		check.FoundError();
		return B_OK;
	}

	check.SetVisited(offset);

	uint32 count = parent->NumKeys();
	off_t* values = parent->Values();
	off_t lastOffset = check.PreviousOffset(level);
	CachedNode cached(this);

	for (uint32 i = 0; i < count; i++) {
		uint16 keyLength;
		uint8* key = parent->KeyAt(i, &keyLength);
		if (largestKey != NULL) {
			int result = compareKeys(fHeader.DataType(), key, keyLength,
				largestKey, largestKeyLength);
			if (result >= 0) {
				dprintf("inode %" B_PRIdOFF ": node %" B_PRIdOFF " key %"
					B_PRIu32 " larger than it should!\n",
					fStream->ID(), offset, i);
				check.FoundError();
			}
		}

		off_t childOffset = BFS_ENDIAN_TO_HOST_INT64(values[i]);
		if (bplustree_node::IsDuplicate(childOffset)) {
			// Walk the duplicate nodes
			off_t duplicateOffset = bplustree_node::FragmentOffset(childOffset);
			off_t lastDuplicateOffset = BPLUSTREE_NULL;

			while (duplicateOffset != BPLUSTREE_NULL) {
				const bplustree_node* node;
				status_t status = cached.SetTo(duplicateOffset, &node, false);
				if (status != B_OK) {
					if (status == B_IO_ERROR)
						return B_IO_ERROR;

					dprintf("inode %" B_PRIdOFF ": duplicate node at %"
						B_PRIdOFF " could not be read: %s\n", fStream->ID(),
						duplicateOffset, strerror(status));
					check.FoundError();
					break;
				}

				bool isFragmentNode = bplustree_node::LinkType(childOffset)
					== BPLUSTREE_DUPLICATE_FRAGMENT;
				bool isKnownFragment = isFragmentNode
					&& check.VisitedFragment(duplicateOffset);

				if (!isKnownFragment && check.Visited(duplicateOffset)) {
					dprintf("inode %" B_PRIdOFF ": %s node at %"
						B_PRIdOFF " already visited, referenced from %"
						B_PRIdOFF "!\n", fStream->ID(),
						isFragmentNode ? "fragment" : "duplicate",
						duplicateOffset, offset);
					check.FoundError();
					break;
				}

				// Fragment nodes may be visited more than once from different
				// places
				if (!check.Visited(duplicateOffset))
					check.SetVisited(duplicateOffset);
				if (!isKnownFragment && isFragmentNode)
					check.SetVisitedFragment(duplicateOffset);

				duplicate_array* array;
				int32 maxSize;
				if (isFragmentNode) {
					array = node->FragmentAt(
						bplustree_node::FragmentIndex(childOffset));
					maxSize = NUM_FRAGMENT_VALUES;
				} else {
					array = node->DuplicateArray();
					maxSize = NUM_DUPLICATE_VALUES;
				}
				int32 arrayCount = array->Count();

				if (arrayCount <= 1 || arrayCount > maxSize) {
					dprintf("inode %" B_PRIdOFF ": duplicate at %" B_PRIdOFF
						" has invalid array size %" B_PRId32 "!\n",
						fStream->ID(), duplicateOffset, arrayCount);
					check.FoundError();
				} else {
					// Simple check if the values in the array may be valid
					for (int32 j = 0; j < arrayCount; j++) {
						if (!fStream->GetVolume()->IsValidInodeBlock(
								array->ValueAt(j))) {
							dprintf("inode %" B_PRIdOFF ": duplicate at %"
								B_PRIdOFF " contains invalid block %" B_PRIdOFF
								" at %" B_PRId32 "!\n", fStream->ID(),
								duplicateOffset, array->ValueAt(j), j);
							check.FoundError();
							break;
						}
					}
				}

				// A fragment node is not linked (and does not have valid links)
				if (isFragmentNode)
					break;

				if (node->LeftLink() != lastDuplicateOffset) {
					dprintf("inode %" B_PRIdOFF ": duplicate at %" B_PRIdOFF
						" has wrong left link %" B_PRIdOFF ", expected %"
						B_PRIdOFF "!\n", fStream->ID(), duplicateOffset,
						node->LeftLink(), lastDuplicateOffset);
					check.FoundError();
				}

				lastDuplicateOffset = duplicateOffset;
				duplicateOffset = node->RightLink();
			}
		} else if (!parent->IsLeaf()) {
			// Test a regular child node recursively
			off_t nextOffset = parent->OverflowLink();
			if (i < count - 1)
				nextOffset = BFS_ENDIAN_TO_HOST_INT64(values[i + 1]);

			if (i == 0 && lastOffset != BPLUSTREE_NULL) {
				// Test right link of the previous node
				const bplustree_node* previous = cached.SetTo(lastOffset, true);
				if (previous == NULL)
					return B_IO_ERROR;

				if (previous->RightLink() != childOffset) {
					dprintf("inode %" B_PRIdOFF ": node at %" B_PRIdOFF " has "
						"wrong right link %" B_PRIdOFF ", expected %" B_PRIdOFF
						"!\n", fStream->ID(), lastOffset, previous->RightLink(),
						childOffset);
					check.FoundError();
				}
			}

			status_t status = _ValidateChild(check, cached, level, childOffset,
				lastOffset, nextOffset, key, keyLength);
			if (status != B_OK)
				return status;
		} else if (!fStream->GetVolume()->IsValidInodeBlock(childOffset)) {
			dprintf("inode %" B_PRIdOFF ": node at %" B_PRIdOFF " contains "
				"invalid block %" B_PRIdOFF " at %" B_PRId32 "!\n",
				fStream->ID(), offset, childOffset, i);
			check.FoundError();
		}

		lastOffset = childOffset;
	}

	if (parent->OverflowLink() != BPLUSTREE_NULL) {
		off_t childOffset = parent->OverflowLink();
		status_t status = _ValidateChild(check, cached, level, childOffset,
			lastOffset, 0, NULL, 0);
		if (status != B_OK)
			return status;

		lastOffset = childOffset;
	}

	check.SetPreviousOffset(level, lastOffset);
	return B_OK;
}


status_t
BPlusTree::_ValidateChild(TreeCheck& check, CachedNode& cached, uint32 level,
	off_t offset, off_t lastOffset, off_t nextOffset,
	const uint8* key, uint16 keyLength)
{
	const bplustree_node* node;
	status_t status = cached.SetTo(offset, &node, true);
	if (status != B_OK) {
		if (status == B_IO_ERROR)
			return B_IO_ERROR;

		dprintf("inode %" B_PRIdOFF ": node at %" B_PRIdOFF " could not be "
			"read: %s\n", fStream->ID(), offset, strerror(status));
		check.FoundError();
		return B_OK;
	}

	if (node->LeftLink() != lastOffset) {
		dprintf("inode %" B_PRIdOFF ": node at %" B_PRIdOFF " has "
			"wrong left link %" B_PRIdOFF ", expected %" B_PRIdOFF
			"!\n", fStream->ID(), offset, node->LeftLink(), lastOffset);
		check.FoundError();
	}

	if (nextOffset != 0 && node->RightLink() != nextOffset) {
		dprintf("inode %" B_PRIdOFF ": node at %" B_PRIdOFF " has "
			"wrong right link %" B_PRIdOFF ", expected %" B_PRIdOFF
			"!\n", fStream->ID(), offset, node->RightLink(), nextOffset);
		check.FoundError();
	}

	return _ValidateChildren(check, level + 1, offset, key, keyLength, node);
}


//	#pragma mark -


TreeIterator::TreeIterator(BPlusTree* tree)
	:
	fTree(tree),
	fCurrentNodeOffset(BPLUSTREE_NULL)
{
	tree->_AddIterator(this);
}


TreeIterator::~TreeIterator()
{
	if (fTree)
		fTree->_RemoveIterator(this);
}


status_t
TreeIterator::Goto(int8 to)
{
	if (fTree == NULL || fTree->fStream == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// lock access to stream
	InodeReadLocker locker(fTree->fStream);

	off_t nodeOffset = fTree->fHeader.RootNode();
	CachedNode cached(fTree);
	const bplustree_node* node;

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
				|| (addr_t)node->Values() > (addr_t)node + fTree->fNodeSize
					- 8 * node->NumKeys())
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


/*!	Iterates through the tree in the specified direction.
	When it iterates through duplicates, the "key" is only updated for the
	first entry - if you need to know when this happens, use the "duplicate"
	parameter which is 0 for no duplicate, 1 for the first, and 2 for all
	the other duplicates.
	That's not too nice, but saves the 256 bytes that would be needed to
	store the last key - if this will ever become an issue, it will be
	easy to change.
	The other advantage of this is, that the queries can skip all duplicates
	at once when they are not relevant to them.
*/
status_t
TreeIterator::Traverse(int8 direction, void* key, uint16* keyLength,
	uint16 maxLength, off_t* value, uint16* duplicate)
{
	if (fTree == NULL)
		return B_INTERRUPTED;

	bool forward = direction == BPLUSTREE_FORWARD;

	if (fCurrentNodeOffset == BPLUSTREE_NULL
		&& Goto(forward ? BPLUSTREE_BEGIN : BPLUSTREE_END) != B_OK)
		RETURN_ERROR(B_ERROR);

	// if the tree was emptied since the last call
	if (fCurrentNodeOffset == BPLUSTREE_FREE)
		return B_ENTRY_NOT_FOUND;

	// lock access to stream
	InodeReadLocker locker(fTree->fStream);

	CachedNode cached(fTree);
	const bplustree_node* node;

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
				// If the node is out of duplicates, we go directly to the next
				// one
				fDuplicateNode = node->RightLink();
				if (fDuplicateNode != BPLUSTREE_NULL
					&& (node = cached.SetTo(fDuplicateNode, false)) != NULL) {
					fNumDuplicates = node->CountDuplicates(fDuplicateNode,
						false);
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

	off_t savedNodeOffset = fCurrentNodeOffset;
	int32 savedKey = fCurrentKey;

	if ((node = cached.SetTo(fCurrentNodeOffset)) == NULL)
		RETURN_ERROR(B_ERROR);

	if (duplicate)
		*duplicate = 0;

	fCurrentKey += direction;

	// is the current key in the current node?
	while ((forward && fCurrentKey >= node->NumKeys())
			|| (!forward && fCurrentKey < 0)) {
		fCurrentNodeOffset = forward ? node->RightLink() : node->LeftLink();

		// are there any more nodes?
		if (fCurrentNodeOffset != BPLUSTREE_NULL) {
			node = cached.SetTo(fCurrentNodeOffset);
			if (!node)
				RETURN_ERROR(B_ERROR);

			// reset current key
			fCurrentKey = forward ? 0 : node->NumKeys();
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
	uint8* keyStart = node->KeyAt(fCurrentKey, &length);
	if (keyStart + length + sizeof(off_t) + sizeof(uint16)
			> (uint8*)node + fTree->fNodeSize
		|| length > BPLUSTREE_MAX_KEY_LENGTH) {
		fTree->fStream->GetVolume()->Panic();
		RETURN_ERROR(B_BAD_DATA);
	}

	length = min_c(length, maxLength);
	memcpy(key, keyStart, length);

	if (fTree->fHeader.DataType() == BPLUSTREE_STRING_TYPE)	{
		// terminate string type
		if (length == maxLength)
			length--;
		((char*)key)[length] = '\0';
	}
	*keyLength = length;

	off_t offset = BFS_ENDIAN_TO_HOST_INT64(node->Values()[fCurrentKey]);

	// duplicate fragments?
	uint8 type = bplustree_node::LinkType(offset);
	if (type == BPLUSTREE_DUPLICATE_FRAGMENT
		|| type == BPLUSTREE_DUPLICATE_NODE) {
		fDuplicateNode = offset;

		node = cached.SetTo(bplustree_node::FragmentOffset(fDuplicateNode),
			false);
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
			// Shouldn't happen, but we're dealing here with potentially
			// corrupt disks...
			fDuplicateNode = BPLUSTREE_NULL;
			offset = 0;
		}
	}
	*value = offset;

	return B_OK;
}


/*!	This is more or less a copy of BPlusTree::Find() - but it just
	sets the current position in the iterator, regardless of if the
	key could be found or not.
*/
status_t
TreeIterator::Find(const uint8* key, uint16 keyLength)
{
	if (fTree == NULL)
		return B_INTERRUPTED;
	if (keyLength < BPLUSTREE_MIN_KEY_LENGTH
		|| keyLength > BPLUSTREE_MAX_KEY_LENGTH
		|| key == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	// lock access to stream
	InodeReadLocker locker(fTree->fStream);

	off_t nodeOffset = fTree->fHeader.RootNode();

	CachedNode cached(fTree);
	const bplustree_node* node;
	while ((node = cached.SetTo(nodeOffset)) != NULL) {
		uint16 keyIndex = 0;
		off_t nextOffset;
		status_t status = fTree->_FindKey(node, key, keyLength, &keyIndex,
			&nextOffset);

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
TreeIterator::Update(off_t offset, off_t nextOffset, uint16 keyIndex,
	uint16 splitAt, int8 change)
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

	// TODO: duplicate handling!
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
	__out("\tfCurrentKey = %d\n", (int)fCurrentKey);
	__out("\tfDuplicateNode = %Ld (%Ld, 0x%Lx)\n",
		bplustree_node::FragmentOffset(fDuplicateNode), fDuplicateNode,
		fDuplicateNode);
	__out("\tfDuplicate = %u\n", fDuplicate);
	__out("\tfNumDuplicates = %u\n", fNumDuplicates);
	__out("\tfIsFragment = %s\n", fIsFragment ? "true" : "false");
}
#endif


// #pragma mark -


bool
bplustree_header::IsValid() const
{
	return Magic() == BPLUSTREE_MAGIC
		&& (RootNode() % NodeSize()) == 0
		&& IsValidLink(RootNode())
		&& IsValidLink(FreeNode());
}


// #pragma mark -


void
bplustree_node::Initialize()
{
	left_link = right_link = overflow_link
		= HOST_ENDIAN_TO_BFS_INT64((uint64)BPLUSTREE_NULL);
	all_key_count = 0;
	all_key_length = 0;
}


uint8*
bplustree_node::KeyAt(int32 index, uint16* keyLength) const
{
	if (index < 0 || index > NumKeys())
		return NULL;

	uint8* keyStart = Keys();
	uint16* keyLengths = KeyLengths();

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

		return ((off_t*)this)[fragment];
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

	return ((off_t*)this)[start + 1 + index];
}


/*!	Although the name suggests it, this function doesn't return the real
	used fragment count; at least, it can only count to two: it returns
	0, if there is no fragment used, 1 if there is only one fragment
	used, and 2 if there are at least 2 fragments used.
*/
uint32
bplustree_node::FragmentsUsed(uint32 nodeSize) const
{
	uint32 used = 0;
	for (uint32 i = 0; i < MaxFragments(nodeSize); i++) {
		duplicate_array* array = FragmentAt(i);
		if (array->Count() > 0 && ++used > 1)
			return used;
	}
	return used;
}


status_t
bplustree_node::CheckIntegrity(uint32 nodeSize) const
{
	if (NumKeys() > nodeSize || AllKeyLength() > nodeSize)
		DEBUGGER(("invalid node: key/length count"));

	for (int32 i = 0; i < NumKeys(); i++) {
		uint16 length;
		uint8* key = KeyAt(i, &length);
		if (key + length + sizeof(off_t) + sizeof(uint16)
				> (uint8*)this + nodeSize
			|| length > BPLUSTREE_MAX_KEY_LENGTH) {
			dprintf("invalid node %p, key %d: keys corrupted\n", this, (int)i);
			return B_BAD_DATA;
		}
		if (Values()[i] == -1) {
			dprintf("invalid node %p, value %d: %lld: values corrupted\n",
				this, (int)i, Values()[i]);
			return B_BAD_DATA;
		}
	}
	return B_OK;
}


// #pragma mark -


BitmapArray::BitmapArray(size_t numBits)
{
	fSize = (numBits + 7) / 8;
	fBitmap = (uint8*)calloc(fSize, 1);
	fCountSet = 0;
}


BitmapArray::~BitmapArray()
{
	free(fBitmap);
}


status_t
BitmapArray::InitCheck() const
{
	return fBitmap != NULL ? B_OK : B_NO_MEMORY;
}


bool
BitmapArray::IsSet(size_t index) const
{
	uint32 byteIndex = index / 8;
	if (byteIndex >= fSize)
		return false;

	return (fBitmap[byteIndex] & (1UL << (index & 0x7))) != 0;
}


void
BitmapArray::Set(size_t index, bool set)
{
	uint32 byteIndex = index / 8;
	if (byteIndex >= fSize)
		return;

	if (set) {
		fBitmap[byteIndex] |= 1UL << (index & 0x7);
		fCountSet++;
	} else {
		fBitmap[byteIndex] &= ~(1UL << (index & 0x7));
		fCountSet--;
	}
}


// #pragma mark -


bool
duplicate_array::_FindInternal(off_t value, int32& index) const
{
	int32 min = 0, max = Count() - 1;
	off_t cmp;
	while (min <= max) {
		index = (min + max) / 2;

		cmp = ValueAt(index) - value;
		if (cmp < 0)
			min = index + 1;
		else if (cmp > 0)
			max = index - 1;
		else
			return true;
	}
	return false;
}


void
duplicate_array::Insert(off_t value)
{
	// if there are more than 8 values in this array, use a
	// binary search, if not, just iterate linearly to find
	// the insertion point
	int32 size = Count();
	int32 i;
	if (size > 8 ) {
		if (!_FindInternal(value, i) && ValueAt(i) <= value)
			i++;
	} else {
		for (i = 0; i < size; i++) {
			if (ValueAt(i) > value)
				break;
		}
	}

	memmove(&values[i + 1], &values[i], (size - i) * sizeof(off_t));
	values[i] = HOST_ENDIAN_TO_BFS_INT64(value);
	count = HOST_ENDIAN_TO_BFS_INT64(size + 1);
}


bool
duplicate_array::Remove(off_t value)
{
	int32 index = Find(value);
	if (index == -1)
		return false;

	int32 newSize = Count() - 1;
	memmove(&values[index], &values[index + 1],
		(newSize - index) * sizeof(off_t));
	count = HOST_ENDIAN_TO_BFS_INT64(newSize);

	return true;
}


// #pragma mark -


int32
compareKeys(type_code type, const void* key1, int keyLength1,
	const void* key2, int keyLength2)
{
	// if one of the keys is NULL, bail out gracefully
	if (key1 == NULL || key2 == NULL) {
		// even if it's most probably a bug in the calling code, we try to
		// give a meaningful result
		if (key1 == NULL && key2 != NULL)
			return 1;
		if (key2 == NULL && key1 != NULL)
			return -1;

		return 0;
	}

	switch (type) {
	    case B_STRING_TYPE:
    	{
			int result = memcmp(key1, key2, min_c(keyLength1, keyLength2));
			if (result == 0) {
				// ignore trailing null bytes
				if ((keyLength1 == keyLength2 + 1
						&& ((uint8*)key1)[keyLength2] == '\0')
					|| (keyLength2 == keyLength1 + 1
						&& ((uint8*)key2)[keyLength1] == '\0'))
					return 0;

				result = keyLength1 - keyLength2;
			}

			return result;
		}

		case B_SSIZE_T_TYPE:
		case B_INT32_TYPE:
			return *(int32*)key1 - *(int32*)key2;

		case B_SIZE_T_TYPE:
		case B_UINT32_TYPE:
			if (*(uint32*)key1 == *(uint32*)key2)
				return 0;
			if (*(uint32*)key1 > *(uint32*)key2)
				return 1;

			return -1;

		case B_OFF_T_TYPE:
		case B_INT64_TYPE:
			if (*(int64*)key1 == *(int64*)key2)
				return 0;
			if (*(int64*)key1 > *(int64*)key2)
				return 1;

			return -1;

		case B_UINT64_TYPE:
			if (*(uint64*)key1 == *(uint64*)key2)
				return 0;
			if (*(uint64*)key1 > *(uint64*)key2)
				return 1;

			return -1;

		case B_FLOAT_TYPE:
		{
			float result = *(float*)key1 - *(float*)key2;
			if (result == 0.0f)
				return 0;

			return (result < 0.0f) ? -1 : 1;
		}

		case B_DOUBLE_TYPE:
		{
			double result = *(double*)key1 - *(double*)key2;
			if (result == 0.0)
				return 0;

			return (result < 0.0) ? -1 : 1;
		}
	}

	// if the type is unknown, the entries don't match...
	return -1;
}

