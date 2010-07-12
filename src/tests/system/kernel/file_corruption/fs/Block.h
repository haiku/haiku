/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_H
#define BLOCK_H


#include <fs_cache.h>

#include "Transaction.h"
#include "Volume.h"


class Block {
public:
	Block()
		:
		fVolume(NULL),
		fData(NULL)
	{
	}

	~Block()
	{
		Put();
	}

	void TransferFrom(Block& other)
	{
		Put();

		fVolume = other.fVolume;
		fData = other.fData;
		fIndex = other.fIndex;
		fWritable = other.fWritable;

		other.fVolume = NULL;
		other.fData = NULL;
	}

	bool GetReadable(Volume* volume, uint64 blockIndex)
	{
		Put();

		return _Init(volume, blockIndex,
			block_cache_get(volume->BlockCache(), blockIndex), false);
	}

	bool GetWritable(Volume* volume, uint64 blockIndex,
		Transaction& transaction)
	{
		Put();

		return _Init(volume, blockIndex,
			block_cache_get_writable(volume->BlockCache(), blockIndex,
				transaction.ID()),
			true);
	}

	bool GetZero(Volume* volume, uint64 blockIndex, Transaction& transaction)
	{
		Put();

		return _Init(volume, blockIndex,
			block_cache_get_empty(volume->BlockCache(), blockIndex,
				transaction.ID()),
			true);
	}

	status_t MakeWritable(Transaction& transaction)
	{
		if (fVolume == NULL)
			return B_BAD_VALUE;
		if (fWritable)
			return B_OK;

		status_t error = block_cache_make_writable(fVolume->BlockCache(),
			fIndex, transaction.ID());
		if (error != B_OK)
			return error;

		fWritable = true;
		return B_OK;
	}

	void Put()
	{
		if (fVolume != NULL) {
			block_cache_put(fVolume->BlockCache(), fIndex);
			fVolume = NULL;
			fData = NULL;
		}
	}

	void Discard()
	{
		if (fVolume != NULL) {
			block_cache_discard(fVolume->BlockCache(), fIndex, 1);
			fVolume = NULL;
			fData = NULL;
		}
	}

	void* Data() const
	{
		return fData;
	}

	uint64 Index() const
	{
		return fIndex;
	}

private:
	bool _Init(Volume* volume, uint64 blockIndex, const void* data,
		bool writable)
	{
		if (data == NULL)
			return false;

		fVolume = volume;
		fData = const_cast<void*>(data);
		fIndex = blockIndex;
		fWritable = writable;

		return true;
	}


private:
	Volume*	fVolume;
	void*	fData;
	uint64	fIndex;
	bool	fWritable;
};


#endif	// BLOCK_H
