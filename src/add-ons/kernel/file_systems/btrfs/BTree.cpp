/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//! BTree implementation


#include "BTree.h"
#include "CachedBlock.h"


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#	define ERROR(x...) dprintf("\33[34mbtrfs:\33[0m " x)


BTree::BTree(Volume* volume, btrfs_stream* stream)
	:
	fStream(stream),
	fRootBlock(0),
	fVolume(volume)
{
	mutex_init(&fIteratorLock, "btrfs b+tree iterator");
}


BTree::BTree(Volume* volume, fsblock_t rootBlock)
	:
	fStream(NULL),
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
BTree::_CompareKeys(btrfs_key& key1, btrfs_key& key2)
{
	if (key1.ObjectID() > key2.ObjectID())
		return 1;
	if (key1.ObjectID() < key2.ObjectID())
		return -1;
	if (key1.Type() > key2.Type())
		return 1;
	if (key1.Type() < key2.Type())
		return -1;
	if (key1.Offset() > key2.Offset())
		return 1;
	if (key1.Offset() < key2.Offset())
		return -1;
	return 0;
}


/*!	Searches the key in the tree, and stores the allocated found item in
	_value, if successful.
	Returns B_OK when the key could be found, B_ENTRY_NOT_FOUND if not.
	It can also return other errors to indicate that something went wrong.
*/
status_t
BTree::_Find(btrfs_key& key, void** _value, size_t* _size,
	btree_traversing type)
{
	TRACE("Find() objectid %" B_PRId64 " type %d offset %" B_PRId64 " \n",
		key.ObjectID(),	key.Type(), key.Offset());
	btrfs_stream* stream = fStream;
	CachedBlock cached(fVolume);
	fsblock_t physical;
	if (stream == NULL) {
		if (fVolume->FindBlock(fRootBlock, physical) != B_OK) {
			ERROR("Find() unmapped block %" B_PRId64 "\n", fRootBlock);
			return B_ERROR;
		}
		stream = (btrfs_stream*)cached.SetTo(physical);
	}

	while (stream->header.Level() != 0) {
		TRACE("Find() level %d\n", stream->header.Level());
		uint32 i = 1;
		for (; i < stream->header.ItemCount(); i++) {
			int32 comp = _CompareKeys(stream->index[i].key, key);
			TRACE("Find() found index %" B_PRIu32 " at %" B_PRId64 " comp %"
				B_PRId32 "\n", i, stream->index[i].BlockNum(), comp);
			if (comp < 0)
				continue;
			if (comp > 0 || type == BTREE_BACKWARD)
				break;
		}
		TRACE("Find() getting index %" B_PRIu32 " at %" B_PRId64 "\n", i - 1,
			stream->index[i - 1].BlockNum());

		if (fVolume->FindBlock(stream->index[i - 1].BlockNum(), physical)
			!= B_OK) {
			ERROR("Find() unmapped block %" B_PRId64 "\n",
				stream->index[i - 1].BlockNum());
			return B_ERROR;
		}
		stream = (btrfs_stream*)cached.SetTo(physical);
	}

	uint32 i;
#ifdef TRACE_BTRFS
	TRACE("Find() dump count %" B_PRId32 "\n", stream->header.ItemCount());
	for (i = 0; i < stream->header.ItemCount(); i++) {
		int32 comp = _CompareKeys(key, stream->entries[i].key);
		TRACE("Find() dump %" B_PRIu32 " %" B_PRIu32 " offset %" B_PRId64
			" comp %" B_PRId32 "\n", stream->entries[i].Offset(),
			stream->entries[i].Size(), stream->entries[i].key.Offset(), comp);
	}
#endif

	for (i = 0; i < stream->header.ItemCount(); i++) {
		int32 comp = _CompareKeys(key, stream->entries[i].key);
		TRACE("Find() found %" B_PRIu32 " %" B_PRIu32 " oid %" B_PRId64
			" type %d offset %" B_PRId64 " comp %" B_PRId32 "\n",
			stream->entries[i].Offset(), stream->entries[i].Size(),
			stream->entries[i].key.ObjectID(), stream->entries[i].key.Type(),
			stream->entries[i].key.Offset(), comp);
		if (comp == 0)
			break;
		if (comp < 0 && i > 0) {
			if (type == BTREE_EXACT)
				return B_ENTRY_NOT_FOUND;
			if (type == BTREE_BACKWARD)
				i--;
			break;
		}
	}

	if (i == stream->header.ItemCount()) {
		if (type == BTREE_BACKWARD)
			i--;
		else
			return B_ENTRY_NOT_FOUND;
	}

	if (i < stream->header.ItemCount()
		&& stream->entries[i].key.Type() == key.Type()) {
		TRACE("Find() found %" B_PRIu32 " %" B_PRIu32 "\n",
			stream->entries[i].Offset(), stream->entries[i].Size());

		if (_value != NULL) {
			*_value = malloc(stream->entries[i].Size());
			memcpy(*_value, ((uint8 *)&stream->entries[0]
				+ stream->entries[i].Offset()),
				stream->entries[i].Size());
			key.SetOffset(stream->entries[i].key.Offset());
			if (_size != NULL)
				*_size = stream->entries[i].Size();
		}
		return B_OK;
	}


	TRACE("Find() not found %" B_PRId64 " %" B_PRId64 "\n", key.Offset(),
		key.ObjectID());

	return B_ENTRY_NOT_FOUND;
}


status_t
BTree::FindNext(btrfs_key& key, void** _value, size_t* _size)
{
	return _Find(key, _value, _size, BTREE_FORWARD);
}


status_t
BTree::FindPrevious(btrfs_key& key, void** _value, size_t* _size)
{
	return _Find(key, _value, _size, BTREE_BACKWARD);
}


status_t
BTree::FindExact(btrfs_key& key, void** _value, size_t* _size)
{
	return _Find(key, _value, _size, BTREE_EXACT);
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


TreeIterator::TreeIterator(BTree* tree, btrfs_key& key)
	:
	fTree(tree),
	fCurrentKey(key)
{
	Rewind();
	tree->_AddIterator(this);
}


TreeIterator::~TreeIterator()
{
	if (fTree)
		fTree->_RemoveIterator(this);
}


/*!	Iterates through the tree in the specified direction.
*/
status_t
TreeIterator::Traverse(btree_traversing direction, btrfs_key& key,
	void** value, size_t* size)
{
	if (fTree == NULL)
		return B_INTERRUPTED;

	fCurrentKey.SetOffset(fCurrentKey.Offset() + direction);
	status_t status = fTree->_Find(fCurrentKey, value, size,
		direction);
	if (status != B_OK) {
		TRACE("TreeIterator::Traverse() Find failed\n");
		return B_ENTRY_NOT_FOUND;
	}

	return B_OK;
}


/*!	just sets the current key in the iterator.
*/
status_t
TreeIterator::Find(btrfs_key& key)
{
	if (fTree == NULL)
		return B_INTERRUPTED;
	fCurrentKey = key;
	return B_OK;
}


void
TreeIterator::Stop()
{
	fTree = NULL;
}
