/*
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//! BTree implementation


#include "BTree.h"
#include "Journal.h"


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
	// binary search for item slot in a node
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
			return B_OK;		// if key is in node
		}
	}

	// |--item1--|--item2--|--item3--|--etc--|
	//     m-1        m        m+1
	//           k          		: comp < 0
	//                     k		: comp > 0
	if (type == BTREE_BACKWARD && comp < 0)
		mid--;
	else if (type == BTREE_FORWARD && comp > 0)
		mid++;

	if (type == BTREE_EXACT || mid < 0)
		return B_ENTRY_NOT_FOUND;

	*slot = mid;
	TRACE("SearchSlot() found slot %" B_PRId32 " comp %" B_PRId32 "\n",
		*slot, comp);
	return B_OK;
}


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


void
BTree::Node::_Copy(const Node* origin, uint32 at, uint32 from, uint32 to,
	int length) const
{
	TRACE("Node::_Copy() at: %d from: %d to: %d length: %d\n",
		at, from, to, length);

	if (Level() == 0) {
		memcpy(Item(at), origin->Item(from), origin->_CalculateSpace(from, to));
		// Item offset of copied node must be changed to get the
		// item data offset correctly. length can be zero
		// but let it there doesn't harm anything.
		for (uint32 i = at; i - at <= to - from; ++i)
			Item(i)->SetOffset(Item(i)->Offset() - length);

		memcpy(ItemData(at + to - from), origin->ItemData(to),
			origin->_CalculateSpace(from, to, 2));
	} else {
		memcpy(Index(at), origin->Index(from),
			origin->_CalculateSpace(from, to));
	}
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


status_t
BTree::Node::Copy(const Node* origin, uint32 start, uint32 end, int length)
	const
{
	status_t status = origin->_SpaceCheck(length);
	if (status != B_OK)
		return status;

	memcpy(fNode, origin->fNode, sizeof(btrfs_header));
	if (length == 0) {
		// like removing [0, start - 1] and keeping [start, end]
		length = -origin->_CalculateSpace(0, start - 1, 2);
		_Copy(origin, 0, start, end, length);
	} else if (length < 0) {
		// removing all items in [start, end]
		if (start > 0)
			_Copy(origin, 0, 0, start - 1, 0);	// <-- [start,...
		if (end + 1 < origin->ItemCount()) {
			// ..., end] -->
			// we only care data size
			length += origin->_CalculateSpace(start, end);
			_Copy(origin, start, end + 1, origin->ItemCount() - 1, length);
		}
	} else {
		// inserting in [start, end] - make a hole for later
		if (start > 0)
			_Copy(origin, 0, 0, start - 1, 0);
		if (start < origin->ItemCount()) {
			length -= origin->_CalculateSpace(start, end);
			_Copy(origin, end + 1, start, origin->ItemCount() - 1, length);
		}
	}

	return B_OK;
}


status_t
BTree::Node::MoveEntries(uint32 start, uint32 end, int length) const
{
	status_t status = _SpaceCheck(length);
	if (status != B_OK || length == 0/*B_OK*/)
		return status;

	int entrySize = sizeof(btrfs_entry);
	if (Level() != 0)
		entrySize = sizeof(btrfs_index);

	uint8* base = (uint8*)fNode + sizeof(btrfs_header);
	end++;
	if (length < 0) {
		// removing [start, end]
		TRACE("Node::MoveEntries() removing ... start %" B_PRIu32 " end %"
			B_PRIu32 " length %i\n", start, end, length);
		length += _CalculateSpace(start, end - 1);
	} else {
		// length > 0
		// inserting into [start, end] - make room for later
		TRACE("Node::MoveEntries() inserting ... start %" B_PRIu32 " end %"
			B_PRIu32 " length %i\n", start, end, length);
		length -= _CalculateSpace(start, end - 1);
		uint32 tmp = start;
		start = end;
		end = tmp;
	}

	if (end >= ItemCount())
		return B_OK;

	int dataSize = _CalculateSpace(end, ItemCount() - 1, 2);
	// moving items/block pointers
	memmove(base + start * entrySize, base + end * entrySize,
		_CalculateSpace(end, ItemCount() - 1));
	if (Level() == 0) {
		// moving item data
		int num = start - end;
		for (uint32 i = start; i < ItemCount() + num; ++i)
			Item(i)->SetOffset(Item(i)->Offset() - length);

		memmove(ItemData(ItemCount() - 1) - length, ItemData(ItemCount() - 1),
			dataSize);
	}

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


status_t
BTree::Path::SetEntry(int slot, const btrfs_entry& entry, void* value)
{
	if (slot < 0)
		return B_ENTRY_NOT_FOUND;

	memcpy(fNodes[0]->Item(slot), &entry, sizeof(btrfs_entry));
	memcpy(fNodes[0]->ItemData(slot), value, entry.Size());
	return B_OK;
}


status_t
BTree::Path::CopyOnWrite(Transaction& transaction, int level, uint32 start,
	int num, int length)
{
	Node* node = fNodes[level];
	if (node == NULL)
		return B_BAD_VALUE;

	status_t status;
	if (transaction.HasBlock(node->BlockNum())) {
		// cow-ed block can not be cow-ed again
		status = node->MoveEntries(start, start + num - 1, length);
		if (status != B_OK)
			return status;

		node->SetGeneration(transaction.SystemID());
		if (length < 0)
			node->SetItemCount(node->ItemCount() - num);
		else if (length > 0)
			node->SetItemCount(node->ItemCount() + num);

		return B_OK;
	}

	uint64 address;
	fsblock_t block;
	status = fTree->SystemVolume()->GetNewBlock(address, block);
	if (status != B_OK)
		return status;

	fNodes[level] = new(std::nothrow) BTree::Node(fTree->SystemVolume());
	if (fNodes[level] == NULL)
		return B_NO_MEMORY;

	fNodes[level]->SetToWritable(block, transaction.ID(), true);

	status = fNodes[level]->Copy(node, start, start + num - 1, length);
	if (status != B_OK)
		return status;

	fNodes[level]->SetGeneration(transaction.SystemID());
	fNodes[level]->SetLogicalAddress(address);
	if (length < 0)
		fNodes[level]->SetItemCount(node->ItemCount() - num);
	else if (length > 0)
		fNodes[level]->SetItemCount(node->ItemCount() + num);
	else
		fNodes[level]->SetItemCount(num);

	// change pointer of this node in parent
	int parentSlot;
	Node* parentNode = GetNode(level + 1, &parentSlot);
	if (parentNode != NULL)
		parentNode->Index(parentSlot)->SetLogicalAddress(address);

	if (level == fTree->RootLevel())
		fTree->SetRoot(fNodes[level]);

	delete node;
	return B_OK;
}


status_t
BTree::Path::InternalCopy(Transaction& transaction, int level)
{
	if (std::abs(level) >= fTree->RootLevel())
		return B_OK;

	TRACE("Path::InternalCopy() level %i\n", level);
	int from, to;
	if (level > 0) {
		from = level;
		to = fTree->RootLevel();
	} else {


		from = 0;
		to = std::abs(level);
	}

	Node* node = NULL;
	status_t status;
	while (from <= to) {
		node = fNodes[from];
		status = CopyOnWrite(transaction, from, 0, node->ItemCount(), 0);
		from++;
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


//	#pragma mark - BTree implementation


BTree::BTree(Volume* volume)
	:
	fRootBlock(0),
	fRootLevel(0),
	fVolume(volume)
{
	mutex_init(&fIteratorLock, "btrfs b+tree iterator");
}


BTree::BTree(Volume* volume, btrfs_stream* stream)
	:
	fRootBlock(0),
	fRootLevel(0),
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

	if (found.Type() != wanted.Type() && wanted.Type() != BTRFS_KEY_TYPE_ANY) {
		ERROR("Find() not found wanted: %" B_PRIu64 " %" B_PRIu8 " %"
			B_PRIu64 " found: %" B_PRIu64 " %" B_PRIu8 " %" B_PRIu64 "\n",
			wanted.ObjectID(), wanted.Type(), wanted.Offset(), found.ObjectID(),
			found.Type(), found.Offset());
		return B_ENTRY_NOT_FOUND;
	}

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
BTree::MakeEntries(Transaction& transaction, Path* path,
	const btrfs_key& startKey, int num, int length)
{
	TRACE("BTree::MakeEntries() num %i key (% " B_PRIu64 " %" B_PRIu8 " %"
		B_PRIu64 ")\n", num, startKey.ObjectID(), startKey.Type(),
		startKey.Offset());

	status_t status = Traverse(BTREE_FORWARD, path, startKey);
	if (status < B_OK)
		return status;

	int slot = status;
	status = path->InternalCopy(transaction, 1);
	if (status != B_OK)
		return status;

	status = path->CopyOnWrite(transaction, 0, slot, num, length);
	if (status == B_DEVICE_FULL) {
		// TODO: push data or split node
		return status;
	}

	if (status != B_OK)
		return status;
	return slot;
}


status_t
BTree::InsertEntries(Transaction& transaction, Path* path,
	btrfs_entry* entries, void** data, int num)
{
	int totalLength = sizeof(btrfs_entry) * num;
	for (int i = 0; i < num; i++)
		totalLength += entries[i].Size();

	status_t slot = MakeEntries(transaction, path, entries[0].key, num,
		totalLength);
	if (slot < B_OK)
		return slot;

	uint32 upperLimit;
	if (slot > 0) {
		path->GetEntry(slot - 1, NULL, NULL, NULL, &upperLimit);
	} else
		upperLimit = fVolume->BlockSize() - sizeof(btrfs_header);

	TRACE("BTree::InsertEntries() num: %i upper limit %" B_PRIu32 "\n", num,
		upperLimit);
	for (int i = 0; i < num; i++) {
		upperLimit -= entries[i].Size();
		entries[i].SetOffset(upperLimit);
		path->SetEntry(slot + i, entries[i], data[i]);
	}

	return B_OK;
}


status_t
BTree::RemoveEntries(Transaction& transaction, Path* path,
	const btrfs_key& startKey, void** _data, int num)
{
	TRACE("BTree::RemoveEntries() num %i key (% " B_PRIu64 " %" B_PRIu8 " %"
		B_PRIu64 ")\n", num, startKey.ObjectID(), startKey.Type(),
		startKey.Offset());

	status_t status = Traverse(BTREE_EXACT, path, startKey);
	if (status < B_OK)
		return status;

	int slot = status;
	int length = -sizeof(btrfs_entry) * num;
	for (int i = 0; i < num; i++) {
		uint32 itemSize;
		path->GetEntry(slot + i, NULL, &_data[i], &itemSize);
		length -= itemSize;
	}

	status = path->InternalCopy(transaction, 1);
	if (status != B_OK)
		return status;

	status = path->CopyOnWrite(transaction, 0, slot, num, length);
	if (status == B_DIRECTORY_NOT_EMPTY) {
		// TODO: merge node or push data
	}

	return status;
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
	} while (level != 0);

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
	} while (level != 0);

	return B_OK;
}


status_t
BTree::SetRoot(off_t logical, fsblock_t* block)
{
	if (block != NULL) {
		fRootBlock = *block;
	} else {
		if (fVolume->FindBlock(logical, fRootBlock) != B_OK) {
			ERROR("SetRoot() unmapped block %" B_PRId64 " %" B_PRId64 "\n",
				logical, fRootBlock);
			return B_ERROR;
		}
	}

	btrfs_header header;
	read_pos(fVolume->Device(), fRootBlock * fVolume->BlockSize(), &header,
		sizeof(btrfs_header));
	fRootLevel = header.Level();
	fLogicalRoot = header.LogicalAddress();
	return B_OK;
}


void
BTree::SetRoot(Node* root)
{
	fRootBlock = root->BlockNum();
	fLogicalRoot = root->LogicalAddress();
	fRootLevel = root->Level();
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


status_t
TreeIterator::_GetEntry(btree_traversing type, void** _value, uint32* _size,
	uint32* _offset)
{
	status_t status = B_OK;
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
