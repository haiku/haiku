/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_H
#define BLOCK_H


#include <fs_cache.h>

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

	bool GetReadable(Volume* volume, uint64 blockIndex)
	{
		Put();

		return _Init(volume, blockIndex,
			block_cache_get(volume->BlockCache(), blockIndex));
	}

	bool GetWritable(Volume* volume, uint64 blockIndex)
	{
		Put();

		return _Init(volume, blockIndex,
			block_cache_get_writable(volume->BlockCache(), blockIndex, -1));
	}

	bool GetZero(Volume* volume, uint64 blockIndex)
	{
		Put();

		return _Init(volume, blockIndex,
			block_cache_get_empty(volume->BlockCache(), blockIndex, -1));
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
	bool _Init(Volume* volume, uint64 blockIndex, const void* data)
	{
		if (data == NULL)
			return false;

		fVolume = volume;
		fData = const_cast<void*>(data);
		fIndex = blockIndex;

		return true;
	}


private:
	Volume*	fVolume;
	void*	fData;
	uint64	fIndex;
};


#endif	// BLOCK_H
