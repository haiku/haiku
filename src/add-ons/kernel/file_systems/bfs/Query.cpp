/*
 * Copyright 2001-2020, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "Query.h"

#include "BPlusTree.h"
#include "bfs.h"
#include "Debug.h"
#include "Index.h"
#include "Inode.h"
#include "Volume.h"

#include <query_private.h>
#include <file_systems/QueryParser.h>


// #pragma mark - QueryPolicy


struct Query::QueryPolicy {
	typedef Query Context;
	typedef ::Inode Entry;
	typedef ::Inode Node;

	struct Index : ::Index {
		bool isSpecialTime;

		Index(Context* context)
			:
			::Index(context->fVolume),
			isSpecialTime(false)
		{
		}
	};

	struct IndexIterator : TreeIterator {
		off_t offset;
		bool isSpecialTime;

		IndexIterator(BPlusTree* tree)
			:
			TreeIterator(tree),
			offset(0),
			isSpecialTime(false)
		{
		}
	};

	struct NodeHolder {
		Vnode vnode;
		NodeGetter nodeGetter;
		RecursiveLocker smallDataLocker;
	};

	static const int32 kMaxFileNameLength = INODE_FILE_NAME_LENGTH;

	// Entry interface

	static ino_t EntryGetParentID(Inode* inode)
	{
		return inode->ParentID();
	}

	static Node* EntryGetNode(Entry* entry)
	{
		return entry;
	}

	static ino_t EntryGetNodeID(Entry* entry)
	{
		return entry->ID();
	}

	static ssize_t EntryGetName(Inode* inode, void* buffer, size_t bufferSize)
	{
		status_t status = inode->GetName((char*)buffer, bufferSize);
		if (status != B_OK)
			return status;
		return strlen((const char*)buffer) + 1;
	}

	static const char* EntryGetNameNoCopy(NodeHolder& holder, Inode* inode)
	{
		// we need to lock before accessing Inode::Name()
		status_t status = holder.nodeGetter.SetTo(inode);
		if (status != B_OK)
			return NULL;

		holder.smallDataLocker.SetTo(inode->SmallDataLock(), false);
		uint8* buffer = (uint8*)inode->Name(holder.nodeGetter.Node());
		if (buffer == NULL) {
			holder.smallDataLocker.Unlock();
			return NULL;
		}

		return (const char*)buffer;
	}

	// Index interface

	static status_t IndexSetTo(Index& index, const char* attribute)
	{
		status_t status = index.SetTo(attribute);
		if (status == B_OK) {
			// The special time flag is set if the time values are shifted
			// 64-bit values to reduce the number of duplicates.
			// We have to be able to compare them against unshifted values
			// later. The only index which needs this is the last_modified
			// index, but we may want to open that feature for other indices,
			// too one day.
			index.isSpecialTime = (strcmp(attribute, "last_modified") == 0);
		}
		return status;
	}

	static void IndexUnset(Index& index)
	{
		index.Unset();
	}

	static int32 IndexGetWeightedScore(Index& index, int32 score)
	{
		// take index size into account (1024 is the current node size
		// in our B+trees)
		// 2048 * 2048 == 4194304 is the maximum score (for an empty
		// tree, since the header + 1 node are already 2048 bytes)
		return score * ((2048 * 1024LL) / index.Node()->Size());
	}

	static type_code IndexGetType(Index& index)
	{
		return index.Type();
	}

	static int32 IndexGetKeySize(Index& index)
	{
		return index.KeySize();
	}

	static IndexIterator* IndexCreateIterator(Index& index)
	{
		IndexIterator* iterator = new(std::nothrow) IndexIterator(index.Node()->Tree());
		if (iterator == NULL)
			return NULL;

		iterator->isSpecialTime = index.isSpecialTime;
		return iterator;
	}

	// IndexIterator interface

	static void IndexIteratorDelete(IndexIterator* iterator)
	{
		delete iterator;
	}

	static status_t IndexIteratorFind(IndexIterator* iterator,
		const void* value, size_t size)
	{
		int64 shiftedTime;
		if (iterator->isSpecialTime) {
			// int64 time index; convert value.
			shiftedTime = *(int64*)value << INODE_TIME_SHIFT;
			value = &shiftedTime;
		}

		return iterator->Find((const uint8*)value, size);
	}

	static status_t IndexIteratorFetchNextEntry(IndexIterator* iterator,
		void* indexValue, size_t* _keyLength, size_t bufferSize, size_t* _duplicate)
	{
		uint16 keyLength;
		uint16 duplicate;
		status_t status = iterator->GetNextEntry((uint8*)indexValue, &keyLength,
			bufferSize, &iterator->offset, &duplicate);
		if (status != B_OK)
			return status;

		if (iterator->isSpecialTime) {
			// int64 time index; convert value.
			*(int64*)indexValue >>= INODE_TIME_SHIFT;
		}

		*_keyLength = keyLength;
		*_duplicate = duplicate;
		return B_OK;
	}

	static status_t IndexIteratorGetEntry(Context* context, IndexIterator* iterator,
		NodeHolder& holder, Inode** _entry)
	{
		holder.vnode.SetTo(context->fVolume, iterator->offset);
		Inode* inode;
		status_t status = holder.vnode.Get(&inode);
		if (status != B_OK) {
			REPORT_ERROR(status);
			FATAL(("could not get inode %" B_PRIdOFF " in index!\n", iterator->offset));
			return status;
		}

		*_entry = inode;
		return B_OK;
	}

	static void IndexIteratorSkipDuplicates(IndexIterator* iterator)
	{
		iterator->SkipDuplicates();
	}

	static void IndexIteratorSuspend(IndexIterator* indexIterator)
	{
		// Nothing to do.
	}

	static void IndexIteratorResume(IndexIterator* indexIterator)
	{
		// Nothing to do.
	}

	// Node interface

	static const off_t NodeGetSize(Inode* inode)
	{
		return inode->Size();
	}

	static time_t NodeGetLastModifiedTime(Inode* inode)
	{
		return bfs_inode::ToSecs(inode->Node().LastModifiedTime());
	}

	static status_t NodeGetAttribute(NodeHolder& holder, Inode* inode,
		const char* attributeName, uint8*& buffer, size_t* size, int32* type)
	{
		status_t status = holder.nodeGetter.SetTo(inode);
		if (status != B_OK)
			return status;

		Inode* attribute;

		holder.smallDataLocker.SetTo(inode->SmallDataLock(), false);
		small_data* smallData = inode->FindSmallData(holder.nodeGetter.Node(),
			attributeName);
		if (smallData != NULL) {
			buffer = smallData->Data();
			*type = smallData->type;
			*size = smallData->data_size;
		} else {
			// needed to unlock the small_data section as fast as possible
			holder.smallDataLocker.Unlock();
			holder.nodeGetter.Unset();

			if (inode->GetAttribute(attributeName, &attribute) == B_OK) {
				*type = attribute->Type();
				if (*size > (size_t)attribute->Size())
					*size = attribute->Size();

				if (*size > MAX_INDEX_KEY_LENGTH)
					*size = MAX_INDEX_KEY_LENGTH;

				if (attribute->ReadAt(0, (uint8*)buffer, size) < B_OK) {
					inode->ReleaseAttribute(attribute);
					return B_IO_ERROR;
				}
				inode->ReleaseAttribute(attribute);
			} else
				return B_ENTRY_NOT_FOUND;
		}
		return B_OK;
	}

	static Entry* NodeGetFirstReferrer(Node* node)
	{
		return node;
	}

	static Entry* NodeGetNextReferrer(Node* node, Entry* entry)
	{
		return NULL;
	}

	// Volume interface

	static dev_t ContextGetVolumeID(Context* context)
	{
		return context->fVolume->ID();
	}
};


// #pragma mark - Query


Query::Query(Volume* volume)
	:
	fVolume(volume),
	fImpl(NULL)
{
}


Query::~Query()
{
	if (fImpl != NULL) {
		if ((fImpl->Flags() & B_LIVE_QUERY) != 0)
			fVolume->RemoveQuery(this);

		delete fImpl;
	}
}


/*static*/ status_t
Query::Create(Volume* volume, const char* queryString, uint32 flags,
	port_id port, uint32 token, Query*& _query)
{
	Query* query = new(std::nothrow) Query(volume);
	if (query == NULL)
		return B_NO_MEMORY;

	status_t error = query->_Init(queryString, flags, port, token);
	if (error != B_OK) {
		delete query;
		return error;
	}

	_query = query;
	return B_OK;
}


status_t
Query::Rewind()
{
	return fImpl->Rewind();
}


status_t
Query::GetNextEntry(struct dirent* entry, size_t size)
{
	return fImpl->GetNextEntry(entry, size);
}


void
Query::LiveUpdate(Inode* inode, const char* attribute, int32 type,
	const void* oldKey, size_t oldLength, const void* newKey, size_t newLength)
{
	fImpl->LiveUpdate(inode, inode, attribute, type,
		(const uint8*)oldKey, oldLength,
		(const uint8*)newKey, newLength);
}


void
Query::LiveUpdateRenameMove(Inode* inode,
	ino_t oldDirectoryID, const char* oldName, size_t oldLength,
	ino_t newDirectoryID, const char* newName, size_t newLength)
{
	fImpl->LiveUpdateRenameMove(inode, inode,
		oldDirectoryID, oldName, oldLength,
		newDirectoryID, newName, newLength);
}


status_t
Query::_Init(const char* queryString, uint32 flags, port_id port, uint32 token)
{
	status_t error = QueryImpl::Create(this, queryString, flags, port, token,
		fImpl);
	if (error != B_OK)
		return error;

	if ((fImpl->Flags() & B_LIVE_QUERY) != 0)
		fVolume->AddQuery(this);

	return B_OK;
}
