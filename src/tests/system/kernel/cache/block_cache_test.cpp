/*
 * Copyright 2008-2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#define write_pos	block_cache_write_pos
#define read_pos	block_cache_read_pos

#include "block_cache.cpp"

#undef write_pos
#undef read_pos


#define MAX_BLOCKS					100
#define BLOCK_CHANGED_IN_MAIN		(1L << 16)
#define BLOCK_CHANGED_IN_SUB		(2L << 16)
#define BLOCK_CHANGED_IN_PREVIOUS	(4L << 16)

#define TEST_BLOCKS(number, count) \
	test_blocks(number, count, __LINE__)

#define TEST_BLOCK_DATA(block, number, type) \
	if ((block)->type ## _data != NULL && gBlocks[(number)]. type == 0) \
		error(line, "Block %Ld: " #type " should be NULL!", (number)); \
	if ((block)->type ## _data != NULL && gBlocks[(number)]. type != 0 \
		&& *(int32*)(block)->type ## _data != gBlocks[(number)]. type) { \
		error(line, "Block %Ld: " #type " wrong (0x%lx should be 0x%lx)!", \
			(number), *(int32*)(block)->type ## _data, \
			gBlocks[(number)]. type); \
	}

#define TEST_ASSERT(statement) \
	if (!(statement)) { \
		error(__LINE__, "Assertion failed: " #statement); \
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
int32 gSubTest;
const char* gTestName;


void
dump_cache()
{
	char cacheString[32];
	sprintf(cacheString, "%p", gCache);
	char* argv[4];
	argv[0] = "dump";
	argv[1] = "-bt";
	argv[2] = cacheString;
	argv[3] = NULL;
	dump_cache(3, argv);
}


void
error(int32 line, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	fprintf(stderr, "ERROR IN TEST LINE %ld: ", line);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");

	va_end(args);

	dump_cache();

	exit(1);
}


void
or_block(void* block, int32 value)
{
	int32* data = (int32*)block;
	*data |= value;
}


void
set_block(void* block, int32 value)
{
	int32* data = (int32*)block;
	*data = value;
}


void
reset_block(void* block, int32 index)
{
	int32* data = (int32*)block;
	*data = index + 1;
}


ssize_t
block_cache_write_pos(int fd, off_t offset, const void* buffer, size_t size)
{
	int32 index = offset / gBlockSize;

	gBlocks[index].written = true;
	if (!gBlocks[index].write)
		error(__LINE__, "Block %ld should not be written!\n", index);

	return size;
}


ssize_t
block_cache_read_pos(int fd, off_t offset, void* buffer, size_t size)
{
	int32 index = offset / gBlockSize;

	memset(buffer, 0xcc, size);
	reset_block(buffer, index);
	if (!gBlocks[index].read)
		error(__LINE__, "Block %ld should not be read!\n", index);

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
test_blocks(off_t number, int32 count, int32 line)
{
	printf("  %ld\n", gSubTest++);

	for (int32 i = 0; i < count; i++, number++) {
		MutexLocker locker(&gCache->lock);

		cached_block* block = (cached_block*)hash_lookup(gCache->hash, &number);
		if (block == NULL) {
			if (gBlocks[number].present)
				error(line, "Block %Ld not found!", number);
			continue;
		}
		if (!gBlocks[number].present)
			error(line, "Block %Ld is present, but should not!", number);

		if (block->is_dirty != gBlocks[number].is_dirty) {
			error(line, "Block %Ld: dirty bit differs (is %d should be %d)!",
				number, block->is_dirty, gBlocks[number].is_dirty);
		}
#if 0
		if (block->unused != gBlocks[number].unused) {
			error("Block %ld: unused bit differs (%d should be %d)!", number,
				block->unused, gBlocks[number].unused);
		}
#endif
		if (block->discard != gBlocks[number].discard) {
			error(line, "Block %Ld: discard bit differs (is %d should be %d)!",
				number, block->discard, gBlocks[number].discard);
		}
		if (gBlocks[number].write && !gBlocks[number].written)
			error(line, "Block %Ld: has not been written yet!", number);

		TEST_BLOCK_DATA(block, number, current);
		TEST_BLOCK_DATA(block, number, original);
		TEST_BLOCK_DATA(block, number, parent);
	}
}


void
stop_test(void)
{
	if (gCache == NULL)
		return;
	TEST_BLOCKS(0, MAX_BLOCKS);

//	dump_cache();
	block_cache_delete(gCache, true);
}


void
start_test(const char* name, bool init = true)
{
	if (init) {
		stop_test();

		gBlockSize = 2048;
		gCache = (block_cache*)block_cache_create(-1, MAX_BLOCKS, gBlockSize,
			false);

		init_test_blocks();
	}

	gTest++;
	gTestName = name;
	gSubTest = 1;

	printf("----------- Test %ld%s%s -----------\n", gTest,
		gTestName[0] ? " - " : "", gTestName);
}


/*!	Changes block 1 in main, block 2 if touchedInMain is \c true.
	Changes block 0 in sub, discards block 2.
	Performs two block tests.
*/
void
basic_test_discard_in_sub(int32 id, bool touchedInMain)
{
	gBlocks[1].present = true;
	gBlocks[1].read = true;
	gBlocks[1].is_dirty = true;
	gBlocks[1].original = gBlocks[1].current;
	gBlocks[1].current |= BLOCK_CHANGED_IN_MAIN;

	void* block = block_cache_get_writable(gCache, 1, id);
	or_block(block, BLOCK_CHANGED_IN_MAIN);
	block_cache_put(gCache, 1);

	TEST_BLOCKS(0, 2);

	if (touchedInMain) {
		gBlocks[2].present = true;
		gBlocks[2].is_dirty = true;
		gBlocks[2].current |= BLOCK_CHANGED_IN_MAIN;

		block = block_cache_get_empty(gCache, 2, id);
		reset_block(block, 2);
		or_block(block, BLOCK_CHANGED_IN_MAIN);
		block_cache_put(gCache, 2);
	}

	cache_start_sub_transaction(gCache, id);

	gBlocks[0].present = true;
	gBlocks[0].read = true;
	gBlocks[0].is_dirty = true;
	if ((gBlocks[0].current & BLOCK_CHANGED_IN_MAIN) != 0)
		gBlocks[0].parent = gBlocks[0].current;
	else
		gBlocks[0].original = gBlocks[0].current;
	gBlocks[0].current |= BLOCK_CHANGED_IN_SUB;

	gBlocks[1].parent = gBlocks[1].current;
	if (touchedInMain)
		gBlocks[2].parent = gBlocks[2].current;

	block = block_cache_get_writable(gCache, 0, id);
	or_block(block, BLOCK_CHANGED_IN_SUB);
	block_cache_put(gCache, 0);

	gBlocks[2].discard = true;

	block_cache_discard(gCache, 2, 1);

	TEST_BLOCKS(0, 2);

	gBlocks[0].is_dirty = false;
	gBlocks[0].write = true;
	gBlocks[1].is_dirty = false;
	gBlocks[1].write = true;
	gBlocks[2].present = false;
	gBlocks[2].write = touchedInMain;
	gBlocks[2].is_dirty = false;
}


// #pragma mark - Tests


void
test_abort_transaction()
{
	start_test("Abort main");

	int32 id = cache_start_transaction(gCache);

	gBlocks[0].present = true;
	gBlocks[0].read = false;
	gBlocks[0].write = false;
	gBlocks[0].is_dirty = true;
	gBlocks[1].present = true;
	gBlocks[1].read = true;
	gBlocks[1].write = false;
	gBlocks[1].is_dirty = true;

	void* block = block_cache_get_empty(gCache, 0, id);
	or_block(block, BLOCK_CHANGED_IN_PREVIOUS);
	gBlocks[0].current = BLOCK_CHANGED_IN_PREVIOUS;

	block = block_cache_get_writable(gCache, 1, id);
	or_block(block, BLOCK_CHANGED_IN_PREVIOUS);
	gBlocks[1].original = gBlocks[1].current;
	gBlocks[1].current |= BLOCK_CHANGED_IN_PREVIOUS;

	block_cache_put(gCache, 0);
	block_cache_put(gCache, 1);

	cache_end_transaction(gCache, id, NULL, NULL);
	TEST_BLOCKS(0, 2);

	id = cache_start_transaction(gCache);

	block = block_cache_get_writable(gCache, 0, id);
	or_block(block, BLOCK_CHANGED_IN_MAIN);

	block = block_cache_get_writable(gCache, 1, id);
	or_block(block, BLOCK_CHANGED_IN_MAIN);

	block_cache_put(gCache, 0);
	block_cache_put(gCache, 1);

	cache_abort_transaction(gCache, id);

	gBlocks[0].write = true;
	gBlocks[0].is_dirty = false;
	gBlocks[1].write = true;
	gBlocks[1].is_dirty = false;
	cache_sync_transaction(gCache, id);
}


void
test_abort_sub_transaction()
{
	start_test("Abort sub");

	int32 id = cache_start_transaction(gCache);

	gBlocks[0].present = true;
	gBlocks[0].read = false;
	gBlocks[0].write = false;
	gBlocks[0].is_dirty = true;
	gBlocks[1].present = true;
	gBlocks[1].read = true;
	gBlocks[1].write = false;
	gBlocks[1].is_dirty = true;

	void* block = block_cache_get_empty(gCache, 0, id);
	or_block(block, BLOCK_CHANGED_IN_PREVIOUS);
	gBlocks[0].current = BLOCK_CHANGED_IN_PREVIOUS;

	block = block_cache_get_writable(gCache, 1, id);
	or_block(block, BLOCK_CHANGED_IN_PREVIOUS);
	gBlocks[1].original = gBlocks[1].current;
	gBlocks[1].current |= BLOCK_CHANGED_IN_PREVIOUS;

	block_cache_put(gCache, 0);
	block_cache_put(gCache, 1);

	cache_start_sub_transaction(gCache, id);

	gBlocks[0].parent = gBlocks[0].current;
	gBlocks[1].parent = gBlocks[1].current;
	TEST_BLOCKS(0, 2);

	block = block_cache_get_writable(gCache, 1, id);
	or_block(block, BLOCK_CHANGED_IN_MAIN);

	block_cache_put(gCache, 1);

	cache_abort_sub_transaction(gCache, id);

	gBlocks[0].write = true;
	gBlocks[0].is_dirty = false;
	gBlocks[1].write = true;
	gBlocks[1].is_dirty = false;

	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);

	start_test("Abort sub with empty block");
	id = cache_start_transaction(gCache);

	gBlocks[1].present = true;
	gBlocks[1].read = true;

	block = block_cache_get_writable(gCache, 1, id);
	or_block(block, BLOCK_CHANGED_IN_PREVIOUS);
	gBlocks[1].original = gBlocks[1].current;
	gBlocks[1].current |= BLOCK_CHANGED_IN_PREVIOUS;

	block_cache_put(gCache, 1);

	gBlocks[1].is_dirty = true;
	TEST_BLOCKS(1, 1);

	cache_start_sub_transaction(gCache, id);

	gBlocks[0].present = true;

	block = block_cache_get_empty(gCache, 0, id);
	or_block(block, BLOCK_CHANGED_IN_SUB);
	gBlocks[0].current = BLOCK_CHANGED_IN_SUB;

	block_cache_put(gCache, 0);

	cache_abort_sub_transaction(gCache, id);

	gBlocks[0].write = false;
	gBlocks[0].is_dirty = false;
	gBlocks[0].parent = 0;
	gBlocks[0].original = 0;
	TEST_BLOCKS(0, 1);

	gBlocks[1].write = true;
	gBlocks[1].is_dirty = false;
	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);
}


void
test_block_cache_discard()
{
	// TODO: test transaction-less block caches
	// TODO: test read-only block caches

	// Test transactions and block caches

	start_test("Discard in main");

	int32 id = cache_start_transaction(gCache);

	gBlocks[0].present = true;
	gBlocks[0].read = true;

	block_cache_get(gCache, 0);
	block_cache_put(gCache, 0);

	gBlocks[1].present = true;
	gBlocks[1].read = true;
	gBlocks[1].write = true;

	void* block = block_cache_get_writable(gCache, 1, id);
	block_cache_put(gCache, 1);

	gBlocks[2].present = false;

	block = block_cache_get_empty(gCache, 2, id);
	block_cache_discard(gCache, 2, 1);
	block_cache_put(gCache, 2);

	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);

	start_test("Discard in sub");

	id = cache_start_transaction(gCache);

	basic_test_discard_in_sub(id, false);
	TEST_ASSERT(cache_blocks_in_sub_transaction(gCache, id) == 1);

	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);

	gBlocks[0].is_dirty = false;
	gBlocks[1].is_dirty = false;

	start_test("Discard in sub, present in main");

	id = cache_start_transaction(gCache);

	basic_test_discard_in_sub(id, true);

	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);

	gBlocks[0].is_dirty = false;
	gBlocks[1].is_dirty = false;

	start_test("Discard in sub, changed in main, abort sub");

	id = cache_start_transaction(gCache);

	basic_test_discard_in_sub(id, true);
	TEST_ASSERT(cache_blocks_in_sub_transaction(gCache, id) == 1);

	gBlocks[0].current &= ~BLOCK_CHANGED_IN_SUB;
	gBlocks[0].is_dirty = false;
	gBlocks[0].write = false;
	gBlocks[1].write = false;
	gBlocks[1].is_dirty = true;
	gBlocks[2].present = true;
	gBlocks[2].is_dirty = true;
	gBlocks[2].write = false;
	gBlocks[2].discard = false;
	cache_abort_sub_transaction(gCache, id);
	TEST_BLOCKS(0, 2);

	gBlocks[1].is_dirty = false;
	gBlocks[1].write = true;
	gBlocks[2].is_dirty = false;
	gBlocks[2].write = true;
	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);

	start_test("Discard in sub, changed in main, new sub");

	id = cache_start_transaction(gCache);

	basic_test_discard_in_sub(id, true);

	cache_start_sub_transaction(gCache, id);
	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);

	start_test("Discard in sub, changed in main, detach sub");

	id = cache_start_transaction(gCache);

	basic_test_discard_in_sub(id, true);

	id = cache_detach_sub_transaction(gCache, id, NULL, NULL);

	gBlocks[0].is_dirty = true;
	gBlocks[0].write = false;
	gBlocks[1].is_dirty = true;
	gBlocks[1].write = false;

	TEST_BLOCKS(0, 2);

	gBlocks[0].is_dirty = false;
	gBlocks[0].write = true;
	gBlocks[1].is_dirty = false;
	gBlocks[1].write = true;

	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);

	start_test("Discard in sub, all changed in main, detach sub");

	id = cache_start_transaction(gCache);

	gBlocks[0].present = true;
	gBlocks[0].read = true;
	gBlocks[0].is_dirty = true;
	gBlocks[0].original = gBlocks[0].current;
	gBlocks[0].current |= BLOCK_CHANGED_IN_MAIN;

	block = block_cache_get_writable(gCache, 0, id);
	or_block(block, BLOCK_CHANGED_IN_MAIN);
	block_cache_put(gCache, 0);

	basic_test_discard_in_sub(id, true);

	id = cache_detach_sub_transaction(gCache, id, NULL, NULL);

	gBlocks[0].is_dirty = true;
	gBlocks[0].write = false;
	gBlocks[0].original |= BLOCK_CHANGED_IN_MAIN;
	gBlocks[1].is_dirty = true;
	gBlocks[1].write = false;

	TEST_BLOCKS(0, 2);

	gBlocks[0].is_dirty = false;
	gBlocks[0].write = true;
	gBlocks[1].is_dirty = false;
	gBlocks[1].write = true;

	cache_end_transaction(gCache, id, NULL, NULL);
	cache_sync_transaction(gCache, id);

	stop_test();
}


// #pragma mark -


int
main(int argc, char** argv)
{
	block_cache_init();

	test_abort_transaction();
	test_abort_sub_transaction();
	test_block_cache_discard();
	return 0;
}
