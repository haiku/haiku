/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//! BTree implementation


#include "BTree.h"


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#	define ERROR(x...) dprintf("\33[34mbtrfs:\33[0m " x)


BTree::Node::Node(Volume* volume)
	:
	fNode(NULL),
	fVolume(volume),
	fBlockNumber(0),
	fWritable(false)
{
}


BTree::Node::Node(Volume* volume, off_t block)
	:
	fNode(NULL),
	fVolume(volume),
	fBlockNumber(0),
	fWritable(false)
{
	SetTo(block);
}


BTree::Node::~Node()
{
	Unset();
}


void
BTree::Node::Keep()
{
	fNode = NULL;
}

void
BTree::Node::Unset()
{
	if (fNode != NULL) {
		block_cache_put(fVolume->BlockCache(), fBlockNumber);
		fNode = NULL;
	}
}


void
BTree::Node::SetTo(off_t block)
{
	Unset();
	fBlockNumber = block;
	fNode = (btrfs_stream*)block_cache_get(fVolume->BlockCache(), block);
}


void
BTree::Node::SetToWritable(off_t block, int32 transactionId, bool empty)
{
	Unset();
	fBlockNumber = block;
	fWritable = true;
	if (empty) {
		fNode = (btrfs_stream*)block_cache_get_empty(fVolume->BlockCache(),
			block, transactionId);
	} else {
		fNode = (btrfs_stream*)block_cache_get_writable(fVolume->BlockCache(),
			block, transactionId);
	}
}


status_t
BTree::Node::SearchSlot(const btrfs_key& key, int* slot, btree_traversing type)
	const
{
	//binary search for item slot in a node
	int entrySize = sizeof(btrfs_entry);
	if (Level() != 0) {
		// internal node
		entrySize = sizeof(btrfs_index);
	}

	int high = ItemCount();
	int low = 0, mid = 0, comp = 0;
	uint8* base = (uint8*)fNode + sizeof(btrfs_header);
	const btrfs_key* other;
	while (low < high) {
		mid = (low + high) / 2;
		other = (const btrfs_key*)(base + mid * entrySize);
		comp = key.Compare(*other);
		if (comp < 0)
			high = mid;
		else if (comp > 0)
			low = mid + 1;
		else {
			*slot = mid;
			return B_OK; 		//if key is in node
		}
	}

	// |--item1--|--item2--|--item3--|--etc--|
	//     m-1        m        m+1
	//           k          		: comp < 0
	//                     k		: comp > 0
	if (type == BTREE_BACKWARD) {
		*slot = mid - 1;
		if (comp > 0)
			*slot = mid;
	} else if (type == BTREE_FORWARD) {
		*slot = mid;
		if (comp > 0)
			*slot = mid + 1;
	}

	if (type == BTREE_EXACT || *slot < 0)
		return B_ENTRY_NOT_FOUND;

	TRACE("SearchSlot() found slot %" B_PRId32 " comp %" B_PRId32 "\n",
		*slot, comp);
	return B_OK;
}


/*
 * calculate used space except the header.
 * type is only for leaf node
 * type 1: only item space
 * type 2: only item data space
 * type 3: both type 1 and 2
 */
int
BTree::Node::_CalculateSpace(uint32 from, uint32 to, uint8 type) const
{
	if (to < from || to >= ItemCount())
		return 0;

	if (Level() != 0)
		return sizeof(btrfs_index) * (to - from + 1);

	uint32 result = 0;
	if ((type & 1) == 1) {
		result += sizeof(btrfs_entry) * (to - from + 1);
	}
	if ((type & 2) == 2) {
		result += Item(from)->Offset() + Item(from)->Size()
			- Item(to)->Offset();
	}

	return result;
}


int
BTree::Node::SpaceUsed() const
{
	if (Level() == 0)
		return _CalculateSpace(0, ItemCount() - 1, 3);
	return _CalculateSpace(0, ItemCount() - 1, 0);
}


int
BTree::Node::SpaceLeft() const
{
	return fVolume->BlockSize() - SpaceUsed() - sizeof(btrfs_header);
}


status_t
BTree::Node::_SpaceCheck(int length) const
{
	// this is a little bit weird here because we can't find
	// any suitable error code
	if (length < 0 && std::abs(length) >= SpaceUsed())
		return B_DIRECTORY_NOT_EMPTY;	// not enough data to delete
	if (length > 0 && length >= SpaceLeft())
		return B_DEVICE_FULL;			// no spare space
	return B_OK;
}


//	#pragma mark - BTree::Path implementation


BTree::Path::Path(BTree* tree)
	:
	fTree(tree)
{
	for (int i = 0; i < BTRFS_MAX_TREE_DEPTH; ++i) {
		fNodes[i] = NULL;
		fSlots[i] = 0;
	}
}


BTree::Path::~Path()
{
	for (int i = 0; i < BTRFS_MAX_TREE_DEPTH; ++i) {
		delete fNodes[i];
		fNodes[i] = NULL;
		fSlots[i] = 0;
	}
}


BTree::Node*
BTree::Path::GetNode(int level, int* _slot) const
{
	if (_slot != NULL)
		*_slot = fSlots[level];
	return fNodes[level];
}


BTree::Node*
BTree::Path::SetNode(off_t block, int slot)
{
	Node node(fTree->SystemVolume(), block);
	return SetNode(&node, slot);
}


BTree::Node*
BTree::Path::SetNode(const Node* node, int slot)
{
	uint8 level = node->Level();
	if (fNodes[level] == NULL) {
		fNodes[level] = new Node(fTree->SystemVolume(), node->BlockNum());
		if (fNodes[level] == NULL)
			return NULL;
	} else
		fNodes[level]->SetTo(node->BlockNum());

	if (slot == -1)
		fSlots[level] = fNodes[level]->ItemCount() - 1;
	else
		fSlots[level] = slot;
	return fNodes[level];
}


int
BTree::Path::Move(int level, int step)
{
	fSlots[level] += step;
	if (fSlots[level] < 0)
		return -1;
	if (fSlots[level] >= fNodes[level]->ItemCount())
		return 1;
	return 0;
}


status_t
BTree::Path::GetEntry(int slot, btrfs_key* _key, void** _value, uint32* _size,
	uint32* _offset)
{
	BTree::Node* leaf = fNodes[0];
	if (slot < 0 || slot >= leaf->ItemCount())
		return B_ENTRY_NOT_FOUND;

	if (_key != NULL)
		*_key = leaf->Item(slot)->key;

	uint32 itemSize = leaf->Item(slot)->Size();
	if (_value != NULL) {
		*_value = malloc(itemSize);
		if (*_value == NULL)
			return B_NO_MEMORY;

		memcpy(*_value, leaf->ItemData(slot), itemSize);
	}

	if (_size != NULL)
		*_size = itemSize;

	if (_offset != NULL)
		*_offset = leaf->Item(slot)->Offset();

	return B_OK;
}


//	#pragma mark - BTree implementation


BTree::BTree(Volume* volume)
	:
	fRootBlock(0),
	fVolume(volume)
{
	mutex_init(&fIteratorLock, "btrfs b+tree iterator");
}


BTree::BTree(Volume* volume, btrfs_stream* stream)
	:
	fRootBlock(0),
	fVolume(volume)
{
	mutex_init(&fIteratorLock, "btrfs b+tree iterator");
}


BTree::BTree(Volume* volume, fsblock_t rootBlock)
	:
	fRootBlock(rootBlock),
	fVolume(volume)
{
	mutex_init(&fIteratorLock, "btrfs b+tree iterator");
}


BTree::~BTree()
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
}


int32
btrfs_key::Compare(const btrfs_key& key) const
{
	if (ObjectID() > key.ObjectID())
		return 1;
	if (ObjectID() < key.ObjectID())
		return -1;
	if (Type() > key.Type())
		return 1;
	if (Type() < key.Type())
		return -1;
	if (Offset() > key.Offset())
		return 1;
	if (Offset() < key.Offset())
		return -1;
	return 0;
}


/* Traverse from root to fill in the path along way its finding.
 * Return current slot at leaf if successful.
 */
status_t
BTree::Traverse(btree_traversing type, Path* path, const btrfs_key& key)
	const
{
	TRACE("BTree::Traverse() objectid %" B_PRId64 " type %d offset %"
		B_PRId64 " \n", key.ObjectID(),	key.Type(), key.Offset());
	fsblock_t physicalBlock = fRootBlock;
	Node node(fVolume, physicalBlock);
	int slot;
	status_t status = B_OK;

	while (node.Level() != 0) {
		TRACE("BTree::Traverse() level %d count %d\n", node.Level(),
			node.ItemCount());
		status = node.SearchSlot(key, &slot, BTREE_BACKWARD);
		if (status != B_OK)
			return status;
		if (path->SetNode(&node, slot) == NULL)
			return B_NO_MEMORY;

		TRACE("BTree::Traverse() getting index %" B_PRIu32 "\n", slot);

		status = fVolume->FindBlock(node.Index(slot)->LogicalAddress(),
				physicalBlock);
		if (status != B_OK) {
			ERROR("BTree::Traverse() unmapped block %" B_PRId64 "\n",
				node.Index(slot)->LogicalAddress());
			return status;
		}
		node.SetTo(physicalBlock);
	}

	TRACE("BTree::Traverse() dump count %" B_PRId32 "\n", node.ItemCount());
	status = node.SearchSlot(key, &slot, type);
	if (status != B_OK)
		return status;
	if (path->SetNode(&node, slot) == NULL)
		return B_NO_MEMORY;

	TRACE("BTree::Traverse() found %" B_PRIu32 " %" B_PRIu32 "\n",
		node.Item(slot)->Offset(), node.Item(slot)->Size());
	return slot;
}


/*!	Searches the key in the tree, and stores the allocated found item in
	_value, if successful.
	Returns B_OK when the key could be found, B_ENTRY_NOT_FOUND if not.
	It can also return other errors to indicate that something went wrong.
*/
status_t
BTree::_Find(Path* path, btrfs_key& wanted, void** _value, uint32* _size,
	uint32* _offset, btree_traversing type) const
{
	status_t status = Traverse(type, path, wanted);
	if (status < B_OK)
		return status;

	btrfs_key found;
	status = path->GetCurrentEntry(&found, _value, _size, _offset);
	if (status != B_OK)
		return status;

	if (found.Type() != wanted.Type() && wanted.Type() != BTRFS_KEY_TYPE_ANY)
		return B_ENTRY_NOT_FOUND;

	wanted = found;
	return B_OK;
}


status_t
BTree::FindNext(Path* path, btrfs_key& key, void** _value, uint32* _size,
	uint32* _offset) const
{
	return _Find(path, key, _value, _size, _offset, BTREE_FORWARD);
}


status_t
BTree::FindPrevious(Path* path, btrfs_key& key, void** _value, uint32* _size,
	uint32* _offset) const
{
	return _Find(path, key, _value, _size, _offset, BTREE_BACKWARD);
}


status_t
BTree::FindExact(Path* path, btrfs_key& key, void** _value, uint32* _size,
	uint32* _offset) const
{
	return _Find(path, key, _value, _size, _offset, BTREE_EXACT);
}


status_t
BTree::PreviousLeaf(Path* path) const
{
	// TODO: use Traverse() ???
	int level = 0;
	int slot;
	Node* node = NULL;
	// iterate to the root until satisfy the condition
	while (true) {
		node = path->GetNode(level, &slot);
		if (node == NULL || slot != 0)
			break;
		level++;
	}

	// the current leaf is already the left most leaf or
	// path was not initialized
	if (node == NULL)
		return B_ENTRY_NOT_FOUND;

	path->Move(level, BTREE_BACKWARD);
	fsblock_t physicalBlock;
	// change all nodes below this level and slot to the ending
	do {
		status_t status = fVolume->FindBlock(
			node->Index(slot)->LogicalAddress(), physicalBlock);
		if (status != B_OK)
			return status;

		node = path->SetNode(physicalBlock, -1);
		if (node == NULL)
			return B_NO_MEMORY;
		slot = node->ItemCount() - 1;
		level--;
	} while(level != 0);

	return B_OK;
}


status_t
BTree::NextLeaf(Path* path) const
{
	int level = 0;
	int slot;
	Node* node = NULL;
	// iterate to the root until satisfy the condition
	while (true) {
		node = path->GetNode(level, &slot);
		if (node == NULL || slot < node->ItemCount() - 1)
			break;
		level++;
	}

	// the current leaf is already the right most leaf or
	// path was not initialized
	if (node == NULL)
		return B_ENTRY_NOT_FOUND;

	path->Move(level, BTREE_FORWARD);
	fsblock_t physicalBlock;
	// change all nodes below this level and slot to the beginning
	do {
		status_t status = fVolume->FindBlock(
			node->Index(slot)->LogicalAddress(), physicalBlock);
		if (status != B_OK)
			return status;

		node = path->SetNode(physicalBlock, 0);
		if (node == NULL)
			return B_NO_MEMORY;
		slot = 0;
		level--;
	} while(level != 0);

	return B_OK;
}


status_t
BTree::SetRoot(off_t logical, fsblock_t* block)
{
	if (block != NULL) {
		fRootBlock = *block;
		//TODO: mapping physical block -> logical address
	} else {
		fLogicalRoot = logical;
		if (fVolume->FindBlock(logical, fRootBlock) != B_OK) {
			ERROR("Find() unmapped block %" B_PRId64 "\n", fRootBlock);
			return B_ERROR;
		}
	}
	return B_OK;
}


void
BTree::_AddIterator(TreeIterator* iterator)
{
	MutexLocker _(fIteratorLock);
	fIterators.Add(iterator);
}


void
BTree::_RemoveIterator(TreeIterator* iterator)
{
	MutexLocker _(fIteratorLock);
	fIterators.Remove(iterator);
}


//	#pragma mark -


TreeIterator::TreeIterator(BTree* tree, const btrfs_key& key)
	:
	fTree(tree),
	fKey(key),
	fIteratorStatus(B_NO_INIT)
{
	tree->_AddIterator(this);
	fPath = new(std::nothrow) BTree::Path(tree);
	if (fPath == NULL)
		fIteratorStatus = B_NO_MEMORY;
}


TreeIterator::~TreeIterator()
{
	if (fTree)
		fTree->_RemoveIterator(this);

	delete fPath;
	fPath = NULL;
}


void
TreeIterator::Rewind(bool inverse)
{
	if (inverse)
		fKey.SetOffset(BTREE_END);
	else
		fKey.SetOffset(BTREE_BEGIN);
	fIteratorStatus = B_NO_INIT;
}


/*!	Iterates through the tree in the specified direction.
*/
status_t
TreeIterator::_Traverse(btree_traversing direction)
{
	status_t status = fTree->Traverse(direction, fPath, fKey);
	if (status < B_OK) {
		ERROR("TreeIterator::Traverse() Find failed\n");
		return status;
	}

	return (fIteratorStatus = B_OK);
}


// Like GetEntry in BTree::Path but here we check the type and moving.
status_t
TreeIterator::_GetEntry(btree_traversing type, void** _value, uint32* _size,
	uint32* _offset)
{
	status_t status;
	if (fIteratorStatus == B_NO_INIT) {
		status = _Traverse(type);
		if (status != B_OK)
			return status;
		type = BTREE_EXACT;
	}

	if (fIteratorStatus != B_OK)
		return fIteratorStatus;

	int move = fPath->Move(0, type);
	if (move > 0)
		status = fTree->NextLeaf(fPath);
	else if (move < 0)
		status = fTree->PreviousLeaf(fPath);
	if (status != B_OK)
		return status;

	btrfs_key found;
	status = fPath->GetCurrentEntry(&found, _value, _size, _offset);
	if (status != B_OK)
		return status;

	fKey.SetObjectID(found.ObjectID());
	fKey.SetOffset(found.Offset());
	if (fKey.Type() != found.Type() && fKey.Type() != BTRFS_KEY_TYPE_ANY)
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}


/*!	just sets the current key in the iterator.
*/
status_t
TreeIterator::Find(const btrfs_key& key)
{
	if (fIteratorStatus == B_INTERRUPTED)
		return fIteratorStatus;

	fKey = key;
	fIteratorStatus = B_NO_INIT;
	return B_OK;
}


void
TreeIterator::Stop()
{
	fTree = NULL;
	fIteratorStatus = B_INTERRUPTED;
}
