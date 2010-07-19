/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUPER_BLOCK_H
#define SUPER_BLOCK_H


#include "checksumfs.h"


class Volume;


struct SuperBlock : private checksumfs_super_block {
public:
			uint64				TotalBlocks() const		{ return totalBlocks; }
			uint64				FreeBlocks() const		{ return freeBlocks; }
			uint32				Version() const			{ return version; }
			const char*			Name() const			{ return name; }
			uint64				BlockBitmap() const		{ return blockBitmap; }
			uint64				RootDirectory() const	{ return rootDir; }

			bool				Check(uint64 totalBlocks) const;
			void				Initialize(Volume* volume);

			void				SetFreeBlocks(uint64 count);
			void				SetName(const char* name);
};


#endif	// SUPER_BLOCK_H
