/*
 * Copyright 2011, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Chế Vũ Gia Hy
 */
#ifndef CHUNK_H
#define CHUNK_H


#include "btrfs.h"


//! Used to translate logical addresses to physical addresses
class Chunk {
public:
								Chunk(btrfs_chunk* chunk,
									fsblock_t offset);
								~Chunk();
			//! \return Value of current physical size
			uint32				Size() const;
			//! Used to convert logical addresses into physical addresses
			status_t			FindBlock(off_t logical, off_t& physical);
			fsblock_t			Offset() const { return fChunkOffset; }
			fsblock_t			End() const
									{ return fChunkOffset + fChunk->Length(); }
private:
			btrfs_chunk*	fChunk;
			fsblock_t			fChunkOffset;
			status_t			fInitStatus;
};

#endif	// CHUNK_H
