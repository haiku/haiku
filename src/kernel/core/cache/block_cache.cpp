/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>

#include <cache.h>
#include <khash.h>
#include <lock.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>


/* Note, this is just a dump and simple cache implementation targeted to be
 * compatible with the one found in BeOS.
 * This won't be the cache we end up using in R1, seriously :-)
 */


struct cache_entry {
	cache_entry	*next;			// next in hash
	off_t		block_number;
	void		*data;
	int32		ref_count;
	int32		lock;
	bool		is_dirty;
};

struct cache {
	hash_table	*hash;
	mutex		lock;
	off_t		max_blocks;
	size_t		block_size;
};

static const int32 kNumCaches = 16;
struct cache sCaches[kNumCaches];
	// we can cache the first 16 fds (I said we were dumb, right?)


class MutexLocker {
	public:
		MutexLocker(int fd)
			: fMutex(NULL)
		{
			if (fd < 0 || fd >= kNumCaches)
				return;

			fMutex = &sCaches[fd].lock;
			mutex_lock(fMutex);
		}

		~MutexLocker()
		{
			if (fMutex != NULL)
				mutex_unlock(fMutex);
		}

		status_t InitCheck()
		{
			return fMutex != NULL ? B_OK : B_ERROR;
		}

	private:
		mutex	*fMutex;
};


static int
cache_entry_compare(void *_cacheEntry, const void *_block)
{
	cache_entry *cacheEntry = (cache_entry *)_cacheEntry;
	const off_t *block = (const off_t *)_block;

	return cacheEntry->block_number - *block;
}


static uint32
cache_entry_hash(void *_cacheEntry, const void *_block, uint32 range)
{
	cache_entry *cacheEntry = (cache_entry *)_cacheEntry;
	const off_t *block = (const off_t *)_block;

	if (cacheEntry != NULL)
		return cacheEntry->block_number % range;

	return *block % range;
}


static status_t
write_cache_entry(int fd, cache_entry *entry)
{
	ssize_t blockSize = sCaches[fd].block_size;

	return write_pos(fd, entry->block_number * blockSize,
				entry->data, blockSize) < blockSize ? B_ERROR : B_OK;
}


void
put_cache_entry(int fd, cache_entry *entry)
{
	if (entry == NULL)
		return;

	if (--entry->ref_count > 0)
		return;

	// write out entry if necessary
	if (entry->is_dirty
		&& write_cache_entry(fd, entry) != B_OK)
		panic("could not write back entry\n");

	hash_remove(sCaches[fd].hash, entry);
	free(entry->data);
	free(entry);
}


cache_entry *
new_cache_entry(int fd, off_t blockNumber, size_t blockSize)
{
	cache_entry *entry = (cache_entry *)malloc(sizeof(cache_entry));
	if (entry == NULL)
		return NULL;

	entry->data = malloc(blockSize);
	if (entry->data == NULL) {
		free(entry);
		return NULL;
	}

	sCaches[fd].block_size = blockSize;
		// store block size (once would be sufficient, though)

	entry->block_number = blockNumber;
	entry->ref_count = 1;
	entry->lock = 0;
	entry->is_dirty = false;

	hash_insert(sCaches[fd].hash, entry);

	return entry;
}


cache_entry *
lookup_cache_entry(int fd, off_t blockNumber)
{
	cache_entry *entry = (cache_entry *)hash_lookup(sCaches[fd].hash, &blockNumber);
	if (entry == NULL)
		return NULL;

	entry->ref_count++;
	return entry;
}


//	#pragma mark -
//	public BeOS compatible interface


extern "C" void
force_cache_flush(int dev, int prefer_log_blocks)
{
}


extern "C" int
flush_blocks(int dev, off_t bnum, int nblocks)
{
	return 0;
}


extern "C" int
flush_device(int dev, int warn_locked)
{
	return 0;
}


extern "C" int
init_cache_for_device(int fd, off_t max_blocks)
{
	if (fd < 0 || fd >= kNumCaches)
		return B_ERROR;
	if (sCaches[fd].hash != NULL)
		return B_BUSY;

	sCaches[fd].hash = hash_init(32, 0, &cache_entry_compare, &cache_entry_hash);
	return mutex_init(&sCaches[fd].lock, "block cache");
}


extern "C" int
remove_cached_device_blocks(int fd, int allowWrite)
{
	if (fd < 0 || fd >= kNumCaches || sCaches[fd].hash == NULL)
		return B_ERROR;

	if (allowWrite) {
		hash_iterator iterator;
		hash_open(sCaches[fd].hash, &iterator);
		
		cache_entry *entry;
		while ((entry = (cache_entry *)hash_next(sCaches[fd].hash, &iterator)) != NULL) {
			// write out entry if necessary
			if (entry->is_dirty
				&& write_cache_entry(fd, entry) != B_OK)
				panic("write failed!\n");
		}
		hash_close(sCaches[fd].hash, &iterator, false);
	}

	hash_uninit(sCaches[fd].hash);
	mutex_destroy(&sCaches[fd].lock);

	return 0;
}


extern "C" void *
get_block(int fd, off_t blockNumber, int blockSize)
{
	MutexLocker locker(fd);
	if (locker.InitCheck() != B_OK)
		return NULL;

	cache_entry *entry = lookup_cache_entry(fd, blockNumber);
	if (entry == NULL) {
		// read entry into cache
		entry = new_cache_entry(fd, blockNumber, blockSize);
		if (entry == NULL)
			return NULL;

		if (read_pos(fd, blockNumber * blockSize, entry->data, blockSize) < blockSize) {
			free(entry->data);
			free(entry);
			return NULL;
		}
	}

	entry->lock++;

	return entry->data;
}


extern "C" void *
get_empty_block(int fd, off_t blockNumber, int blockSize)
{
	MutexLocker locker(fd);
	if (locker.InitCheck() != B_OK)
		return NULL;

	cache_entry *entry = lookup_cache_entry(fd, blockNumber);
	if (entry == NULL) {
		// create new cache entry
		entry = new_cache_entry(fd, blockNumber, blockSize);
		if (entry == NULL)
			return NULL;
	}
	memset(entry->data, 0, blockSize);

	entry->lock++;

	return entry->data;
}


extern "C" int
release_block(int fd, off_t blockNumber)
{
	MutexLocker locker(fd);
	if (locker.InitCheck() != B_OK)
		return B_ERROR;

	cache_entry *entry = lookup_cache_entry(fd, blockNumber);
	if (entry == NULL)
		panic("release_block() called on block %Ld that was not cached\n", blockNumber);

	entry->lock--;
	put_cache_entry(fd, entry);
	return 0;
}


extern "C" int
mark_blocks_dirty(int fd, off_t blockNumber, int numBlocks)
{
	return -1;
}


extern "C" int
cached_read(int fd, off_t blockNumber, void *data, off_t numBlocks, int blockSize)
{
	MutexLocker locker(fd);
	if (locker.InitCheck() != B_OK)
		return B_ERROR;

	ssize_t bytes = numBlocks * blockSize;
	if (read_pos(fd, blockNumber * blockSize, data, bytes) < bytes)
		return -1;

	// check if there are any cached blocks in that range shadowing the device contents

	for (off_t blockOffset = 0; blockOffset < numBlocks; blockOffset++) {
		cache_entry *entry = lookup_cache_entry(fd, blockNumber + blockOffset);
		if (entry != NULL) {
			if (entry->is_dirty)
				memcpy((void *)((addr_t)data + blockOffset * blockSize), entry->data, blockSize);
			put_cache_entry(fd, entry);
		}
	}
	return 0;
}


extern "C" int
cached_write(int fd, off_t blockNumber, const void *data, off_t numBlocks, int blockSize)
{
	return -1;
}


extern "C" int
cached_write_locked(int fd, off_t blockNumber, const void *data, off_t numBlocks, int blockSize)
{
	return -1;
}


extern "C" int
set_blocks_info(int fd, off_t *blocks, int numBlocks,
	void (*func)(off_t bnum, size_t nblocks, void *arg),
	void *arg)
{
	return -1;
}

