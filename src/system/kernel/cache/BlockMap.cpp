/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


/** The BlockMap stores offset:address pairs; you can map an offset to a specific address.
 *	It has been designed to contain few and mostly contiguous offset mappings - it is used
 *	by the file cache to keep track about which blocks of the file are already in memory.
 *	The offsets may spread over a very large amount.
 *
 *	Internally, it stores small and contiguous address arrays of a certain size, and
 *	accesses those using a hash table. Address values of NULL are equal to non existing
 *	mappings; that value cannot be stored. At the current size, each hash entry can
 *	map 60 addresses which corresponds to a file range of 240 kB.
 *	It currently does not do any locking; it assumes a safe environment which you are
 *	responsible for to create when you call its functions.
 */


#include "BlockMap.h"

#include <KernelExport.h>
#include <util/kernel_cpp.h>

#include <stdlib.h>
#include <string.h>


//#define TRACE_BLOCK_MAP
#ifdef TRACE_BLOCK_MAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


// ToDo: when we have a better allocator, change the number of addresses to a power of two
// currently, this structure takes 256 bytes total

struct BlockMap::block_entry {
	block_entry	*next;
	uint32		used;
	off_t		offset;
	addr_t		address[60];
};

#define BLOCK_ARRAY_SIZE (sizeof(BlockMap::block_entry::address) / sizeof(addr_t))

static inline off_t
to_block_entry_offset(off_t offset, uint32 &index)
{
	// ToDo: improve this once we have a power of two array size
	off_t baseOffset = (offset / BLOCK_ARRAY_SIZE) * BLOCK_ARRAY_SIZE;
	index = uint32(offset - baseOffset);

	return baseOffset;
}


static int
block_entry_compare(void *_entry, const void *_offset)
{
	BlockMap::block_entry *entry = (BlockMap::block_entry *)_entry;
	const off_t *offset = (const off_t *)_offset;

	return entry->offset - *offset;
}


static uint32
block_entry_hash(void *_entry, const void *_offset, uint32 range)
{
	BlockMap::block_entry *entry = (BlockMap::block_entry *)_entry;
	const off_t *offset = (const off_t *)_offset;

	if (entry != NULL)
		return entry->offset % range;

	return *offset % range;
}


//	#pragma mark -


BlockMap::BlockMap(off_t size)
	:
	fSize(size)
{
	fHashTable = hash_init(16, 0, &block_entry_compare, &block_entry_hash);
}


BlockMap::~BlockMap()
{
}


/** Checks whether or not the construction of the BlockMap were successful.
 */

status_t
BlockMap::InitCheck() const
{
	return fHashTable != NULL ? B_OK : B_NO_MEMORY;
}


/**	Sets the size of the block map - all existing entries beyond this size will be
 *	removed from the map, and their memory is freed.
 */

void
BlockMap::SetSize(off_t size)
{
	TRACE(("BlockMap::SetSize(%Ld)\n", size));

	if (size >= fSize) {
		// nothing to do
		fSize = size;
		return;
	}
	
	// ToDo: remove all mappings beyond the file size
}


/**	Upon successful exit which is indicated by a return value of B_OK, the "_entry"
 *	argument points to a block_entry structure containing the data for the given
 *	offset.
 *	The offset must have been normalized to the base offset values of a block entry
 *	already.
 */

status_t
BlockMap::GetBlockEntry(off_t baseOffset, block_entry **_entry)
{
	block_entry *entry = (block_entry *)hash_lookup(fHashTable, &baseOffset);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	*_entry = entry;
	return B_OK;
}


status_t
BlockMap::Remove(off_t offset, off_t count)
{
	TRACE(("BlockMap::Remove(offset = %Ld, count = %Ld)\n", offset, count));

	uint32 index;
	off_t baseOffset = to_block_entry_offset(offset, index);
	block_entry *entry;

	while (count > 0) {
		int32 max = min_c(BLOCK_ARRAY_SIZE, index + count);
		int32 blocks = max - index;

		if (GetBlockEntry(baseOffset, &entry) == B_OK) {
			for (int32 i = index; i < max; i++) {
				if (entry->address[i] != NULL)
					entry->used--;

				entry->address[i] = NULL;
			}

			if (entry->used == 0) {
				// release entry if it's no longer used
				hash_remove(fHashTable, entry);
				free(entry);
			}
		}

		baseOffset += BLOCK_ARRAY_SIZE;
		count -= blocks;
		index = 0;
	}

	return B_OK;
}


status_t
BlockMap::Set(off_t offset, addr_t address)
{
	TRACE(("BlockMap::Set(offset = %Ld, address = %08lx)\n", offset, address));

	uint32 index;
	off_t baseOffset = to_block_entry_offset(offset, index);

	block_entry *entry;
	if (GetBlockEntry(baseOffset, &entry) == B_OK) {
		// the block already exists, we just need to fill in our new address
		if (entry->address[index] == NULL && address != NULL)
			entry->used++;
		else if (entry->address[index] != NULL && address == NULL)
			entry->used--;

		entry->address[index] = address;
		return B_OK;
	}
	
	// allocate new block and fill it

	entry = (block_entry *)malloc(sizeof(struct block_entry));
	if (entry == NULL)
		return B_NO_MEMORY;

	memset(entry->address, 0, sizeof(entry->address));

	entry->used = 1;
	entry->offset = baseOffset;

	hash_insert(fHashTable, entry);

	entry->address[index] = address;
	return B_OK;
}


status_t
BlockMap::Get(off_t offset, addr_t &address)
{
	TRACE(("BlockMap::Get(offset = %Ld)\n", offset));

	uint32 index;
	off_t baseOffset = to_block_entry_offset(offset, index);

	block_entry *entry;
	if (GetBlockEntry(baseOffset, &entry) == B_OK
		&& entry->address[index] != NULL) {
		address = entry->address[index];
		return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}

