/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#define write_pos	block_cache_write_pos
#define read_pos	block_cache_read_pos

#include "block_cache.cpp"

#undef write_pos
#undef read_pos


#define MAX_BLOCKS	100

#define TEST_BLOCK_DATA(block, number, type) \
	if ((block)->type ## _data != NULL && gBlocks[(number)]. type == 0) \
		error("Block %Ld: " #type " should be NULL!", (number)); \
	if ((block)->type ## _data != NULL && gBlocks[(number)]. type != 0 \
		&& *(int32*)(block)->type ## _data != gBlocks[(number)]. type) { \
		error("Block %Ld: " #type " wrong (%ld should be %ld)!", (number), \
			*(int32*)(block)->type ## _data, gBlocks[(number)]. type); \
	}

struct test_block {
	int32	current;
	int32	original;
	int32	parent;
	int32	previous_transaction;
	int32	transaction;
	bool	unused;
	bool	is_dirty;
	bool	discard;

	bool	write;

	bool	read;
	bool	written;
	bool	present;
};

test_block gBlocks[MAX_BLOCKS];
block_cache* gCache;
size_t gBlockSize;
int32 gTest;
const char* gTestName;


ssize_t
block_cache_write_pos(int fd, off_t offset, const void* buffer, size_t size)
{
	//printf("write: %Ld, %p, %lu\n", offset, buffer, size);
	gBlocks[offset / gBlockSize].written = true;
	return size;
}


ssize_t
block_cache_read_pos(int fd, off_t offset, void* buffer, size_t size)
{
	//printf("read: %Ld, %p, %lu\n", offset, buffer, size);
	memset(buffer, 0xcc, size);
	int32* value = (int32*)buffer;
	*value = offset / gBlockSize + 1;
	gBlocks[offset / gBlockSize].read = true;

	return size;
}


void
init_test_blocks()
{
	memset(gBlocks, 0, sizeof(test_block) * MAX_BLOCKS);

	for (uint32 i = 0; i < MAX_BLOCKS; i++) {
		gBlocks[i].current = i + 1;
		gBlocks[i].unused = true;
	}
}


void
error(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");

	va_end(args);

	char cacheString[32];
	sprintf(cacheString, "%p", gCache);
	char* argv[4];
	argv[0] = "dump";
	argv[1] = "-bt";
	argv[2] = cacheString;
	argv[3] = NULL;
	dump_cache(3, argv);

	exit(1);
}


void
test_blocks(off_t number, int32 count)
{
	for (int32 i = 0; i < count; i++, number++) {
		MutexLocker locker(&gCache->lock);

		cached_block* block = (cached_block*)hash_lookup(gCache->hash, &number);
		if (block == NULL) {
			if (gBlocks[number].present)
				error("Block %Ld not found!", number);
			continue;
		}
		if (!gBlocks[number].present)
			error("Block %Ld is present, but should not!", number);

		if (block->is_dirty != gBlocks[number].is_dirty) {
			error("Block %Ld: dirty bit differs (%d should be %d)!", number,
				block->is_dirty, gBlocks[number].is_dirty);
		}
#if 0
		if (block->unused != gBlocks[number].unused) {
			error("Block %ld: discard bit differs (%d should be %d)!", number,
				block->unused, gBlocks[number].unused);
		}
#endif
		if (block->discard != gBlocks[number].discard) {
			error("Block %Ld: discard bit differs (%d should be %d)!", number,
				block->discard, gBlocks[number].discard);
		}
		if (gBlocks[number].write && !gBlocks[number].written)
			error("Block %Ld: has not been written yet!", number);

		TEST_BLOCK_DATA(block, number, current);
		TEST_BLOCK_DATA(block, number, original);
		TEST_BLOCK_DATA(block, number, parent);
	}
}


void
test_block(off_t block)
{
	test_blocks(block, 1);
}


void
stop_test(void)
{
	if (gCache == NULL)
		return;
	test_blocks(0, MAX_BLOCKS);

	block_cache_delete(gCache, true);
}


void
start_test(int32 test, const char* name, bool init = false)
{
	if (init) {
		stop_test();

		gBlockSize = 2048;
		gCache = (block_cache*)block_cache_create(-1, MAX_BLOCKS, gBlockSize,
			false);

		init_test_blocks();
	}

	gTest = test;
	gTestName = name;
	printf("----------- Test %ld%s%s -----------\n", gTest,
		gTestName[0] ? " - " : "", gTestName);
}


int
main(int argc, char** argv)
{
	block_cache_init();

	const void* block;

	// TODO: test transaction-less block caches
	// TODO: test read-only block caches

	// Test transactions and block caches

	start_test(1, "Discard simple", true);

	int32 id = cache_start_transaction(gCache);

	gBlocks[0].present = true;
	gBlocks[0].read = true;

	block = block_cache_get(gCache, 0);
	block_cache_put(gCache, 0);

	gBlocks[1].present = true;
	gBlocks[1].read = true;
	gBlocks[1].write = true;

	block = block_cache_get_writable(gCache, 1, id);
	block_cache_put(gCache, 1);

	gBlocks[2].present = false;

	block = block_cache_get_empty(gCache, 2, id);
	block_cache_discard(gCache, 2, 1);
	block_cache_put(gCache, 2);

	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);

	stop_test();
	return 0;
}
