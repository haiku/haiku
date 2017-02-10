/* cache - emulation for the B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "cache.h"

#include "BPlusTree.h"
	// To get at the right debug helpers

#include <File.h>
#include <List.h>

#include <malloc.h>
#include <stdio.h>


/*	A note from the author: this cache implementation can only be used
 *	with the test program, it really suites no other needs.
 *	It's very simple and not that efficient, and simple holds the whole
 *	file in memory, all the time.
 */


#define TRACE(x)	/*printf x*/


static size_t sBlockSize;


BList gBlocks;


void
init_cache(BFile* /*file*/, int32 blockSize)
{
	sBlockSize = blockSize;
}


void
shutdown_cache(BFile* file, int32 blockSize)
{
	for (int32 i = 0; i < gBlocks.CountItems(); i++) {
		void* buffer = gBlocks.ItemAt(i);
		if (buffer == NULL) {
			debugger("cache is corrupt!");
			exit(-1);
		}

		file->WriteAt(i * blockSize, buffer, blockSize);
		free(buffer);
	}
}


status_t
cached_write(void* cache, off_t num, const void* _data, off_t numBlocks)
{
	if (num + numBlocks > gBlocks.CountItems()) {
		debugger("cached write beyond loaded blocks");
		exit(1);
	}

	for (off_t i = 0; i < numBlocks; i++) {
		void* buffer = gBlocks.ItemAt(num + i);
		const void* data = (uint8*)_data + i * sBlockSize;
		if (buffer != data)
			memcpy(buffer, data, sBlockSize);
	}

	return B_OK;
}


static status_t
read_blocks(void* cache, off_t num)
{
	BFile* file = (BFile*)cache;
	for (uint32 i = gBlocks.CountItems(); i <= num; i++) {
		void* buffer = malloc(sBlockSize);
		if (buffer == NULL)
			return B_NO_MEMORY;

		gBlocks.AddItem(buffer);
		if (file->ReadAt(i * sBlockSize, buffer, sBlockSize) < 0)
			return B_IO_ERROR;
	}

	return B_OK;
}


static void*
get_block(void* cache, off_t num)
{
	//TRACE(("get_block(num = %" B_PRIdOFF ")\n", num);
	if (num >= gBlocks.CountItems())
		read_blocks(cache, num);

	return gBlocks.ItemAt(num);
}


static void
release_block(void* cache, off_t num)
{
	//TRACE(("release_block(num = %" B_PRIdOFF ")\n", num));
}


// #pragma mark - Block Cache API


const void*
block_cache_get(void* _cache, off_t blockNumber)
{
	TRACE(("block_cache_get(block = %" B_PRIdOFF ")\n", blockNumber));
	return get_block(_cache, blockNumber);
}


status_t
block_cache_make_writable(void* _cache, off_t blockNumber, int32 transaction)
{
	TRACE(("block_cache_make_writable(block = %" B_PRIdOFF ", transaction = %"
		B_PRId32 ")\n", blockNumber, transaction));

	// We're always writable...
	return B_OK;
}


void*
block_cache_get_writable(void* _cache, off_t blockNumber, int32 transaction)
{
	TRACE(("block_cache_get_writable(block = %" B_PRIdOFF
		", transaction = %" B_PRId32 ")\n", blockNumber, transaction));
	return get_block(_cache, blockNumber);
}



status_t
block_cache_set_dirty(void* _cache, off_t blockNumber, bool dirty,
	int32 transaction)
{
	TRACE(("block_cache_set_dirty(block = %" B_PRIdOFF
		", dirty = %s, transaction = %" B_PRId32 ")\n", blockNumber,
		dirty ? "yes" : "no", transaction));

	if (dirty)
		debugger("setting to dirty not implemented\n");

	return B_OK;
}


void
block_cache_put(void* _cache, off_t blockNumber)
{
	TRACE(("block_cache_put(block = %" B_PRIdOFF ")\n", blockNumber));
	release_block(_cache, blockNumber);
}
