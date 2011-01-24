/*
 * Copyright 2011, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */


#include "Chunk.h"

#include <stdlib.h>
#include <string.h>


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#	define FATAL(x...) dprintf("\33[34mbtrfs:\33[0m " x)


Chunk::Chunk(struct btrfs_chunk* chunk, fsblock_t offset)
	:
	fChunk(NULL),
	fInitStatus(B_OK)
{
	fChunkOffset = offset;
	fChunk = (struct btrfs_chunk*)malloc(sizeof(struct btrfs_chunk)
		+ chunk->StripeCount() * sizeof(struct btrfs_stripe));
	if (fChunk == NULL)
		fInitStatus = B_NO_MEMORY;
	memcpy(fChunk, chunk, sizeof(struct btrfs_chunk)
		+ chunk->StripeCount() * sizeof(struct btrfs_stripe));

	TRACE("chunk[0] length %llu owner %llu stripe_length %llu type %llu "
			"stripe_count %u sub_stripes %u sector_size %lu\n", chunk->Length(),
			chunk->Owner(), chunk->StripeLength(), chunk->Type(),
			chunk->StripeCount(), chunk->SubStripes(), chunk->SectorSize());
	for(int32 i = 0; i < chunk->StripeCount(); i++) {
		TRACE("chunk.stripe[%ld].physical %lld deviceid %lld\n", i,
			chunk->stripes[i].Offset(), chunk->stripes[i].DeviceID());
	}
}


Chunk::~Chunk()
{
	free(fChunk);
}


uint32
Chunk::Size() const
{
	return sizeof(struct btrfs_chunk) 
		+ fChunk->StripeCount() * sizeof(struct btrfs_stripe);
}


status_t
Chunk::FindBlock(off_t logical, off_t &physical)
{
	if (fChunk == NULL)
		return B_NO_INIT;

	if (logical < fChunkOffset
		|| logical > (fChunkOffset + fChunk->Length()))
			return B_BAD_VALUE;
	
	// only one stripe
	physical = logical + fChunk->stripes[0].Offset() - fChunkOffset;
	return B_OK;
}

