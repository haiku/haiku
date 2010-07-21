/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Block.h"

#include <fs_cache.h>

#include "Transaction.h"
#include "Volume.h"


void
Block::TransferFrom(Block& other)
{
	Put();

	fVolume = other.fVolume;
	fData = other.fData;
	fIndex = other.fIndex;
	fTransaction = other.fTransaction;

	other.fVolume = NULL;
	other.fData = NULL;
}


bool
Block::GetReadable(Volume* volume, uint64 blockIndex)
{
	Put();

	return _Init(volume, blockIndex,
		block_cache_get(volume->BlockCache(), blockIndex), NULL);
}


bool
Block::GetWritable(Volume* volume, uint64 blockIndex, Transaction& transaction)
{
	Put();

	status_t error = transaction.RegisterBlock(blockIndex);
	if (error != B_OK)
		return false;

	return _Init(volume, blockIndex,
		block_cache_get_writable(volume->BlockCache(), blockIndex,
			transaction.ID()),
		&transaction);
}


bool
Block::GetZero(Volume* volume, uint64 blockIndex, Transaction& transaction)
{
	Put();

	status_t error = transaction.RegisterBlock(blockIndex);
	if (error != B_OK)
		return false;

	return _Init(volume, blockIndex,
		block_cache_get_empty(volume->BlockCache(), blockIndex,
			transaction.ID()),
		&transaction);
}


status_t
Block::MakeWritable(Transaction& transaction)
{
	if (fVolume == NULL)
		return B_BAD_VALUE;
	if (fTransaction != NULL)
		return B_OK;

	status_t error = transaction.RegisterBlock(fIndex);
	if (error != B_OK)
		return error;

	error = block_cache_make_writable(fVolume->BlockCache(), fIndex,
		transaction.ID());
	if (error != B_OK) {
		transaction.PutBlock(fIndex, NULL);
		return error;
	}

	fTransaction = &transaction;
	return B_OK;
}


void
Block::Put()
{
	if (fVolume != NULL) {
		if (fTransaction != NULL)
			fTransaction->PutBlock(fIndex, fData);

		block_cache_put(fVolume->BlockCache(), fIndex);
		fVolume = NULL;
		fData = NULL;
	}
}


bool
Block::_Init(Volume* volume, uint64 blockIndex, const void* data,
	Transaction* transaction)
{
	if (data == NULL)
		return false;

	fVolume = volume;
	fData = const_cast<void*>(data);
	fIndex = blockIndex;
	fTransaction = transaction;

	return true;
}
