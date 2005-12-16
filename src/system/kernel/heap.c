/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>
#include <kernel.h>
#include <vm.h>
#include <lock.h>
#include <int.h>
#include <memheap.h>
#include <malloc.h>
#include <debug.h>
#include <arch/cpu.h>
#include <boot/kernel_args.h>

#include <string.h>

//#define TRACE_HEAP
#ifdef TRACE_HEAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

/* prevent freeing pointers that were not allocated by kmalloc or are already freeed */
#ifdef DEBUG
#	define PARANOID_POINTER_CHECK 1
#else
#	define PARANOID_POINTER_CHECK 0
#endif
/* initialize newly allocated memory with something non zero */  
#define PARANOID_KMALLOC 1
/* check if freed pointers are already freed, and fill freed memory with 0xdeadbeef */  
#define PARANOID_KFREE 1
/* use a back and front wall around each allocation */
#define USE_WALL 0
#define USE_CHECKING_WALL 0

#define WALL_SIZE 8
	// must be a multiple of 4 

#if USE_CHECKING_WALL
#	define WALL_CHECK_FREQUENCY	1 /* every tenth second */
#endif

#if USE_WALL
struct front_wall {
#	if USE_CHECKING_WALL
	struct list_link link;
#	endif
	size_t	alignment;
	size_t	size;
	uint32	wall[WALL_SIZE / 4];
};

struct back_wall {
	uint32	wall[WALL_SIZE / 4];
};
#endif	// USE_WALL

// heap stuff
// ripped mostly from nujeffos

struct heap_page {
	uint16	bin_index : 5;
	uint16	free_count : 9;
	uint16	cleaning : 1;
	uint16	in_use : 1;
} PACKED;

static struct heap_page *heap_alloc_table;
static addr_t heap_base_ptr;
static addr_t heap_base;
static addr_t heap_size;

struct heap_bin {
	uint32	element_size;
	uint32	grow_size;
	uint32	alloc_count;
	void	*free_list;
	uint32	free_count;
	char	*raw_list;
	uint32	raw_count;
};

static struct heap_bin bins[] = {
	{16, B_PAGE_SIZE, 0, 0, 0, 0, 0},
	{32, B_PAGE_SIZE, 0, 0, 0, 0, 0},
	{64, B_PAGE_SIZE, 0, 0, 0, 0, 0},
	{128, B_PAGE_SIZE, 0, 0, 0, 0, 0},
	{256, B_PAGE_SIZE, 0, 0, 0, 0, 0},
	{512, B_PAGE_SIZE, 0, 0, 0, 0, 0},
	{1024, B_PAGE_SIZE, 0, 0, 0, 0, 0},
	{2048, B_PAGE_SIZE, 0, 0, 0, 0, 0},
	{0x1000, 0x1000, 0, 0, 0, 0, 0},
	{0x2000, 0x2000, 0, 0, 0, 0, 0},
	{0x3000, 0x3000, 0, 0, 0, 0, 0},
	{0x4000, 0x4000, 0, 0, 0, 0, 0},
	{0x5000, 0x5000, 0, 0, 0, 0, 0},
	{0x6000, 0x6000, 0, 0, 0, 0, 0},
	{0x7000, 0x7000, 0, 0, 0, 0, 0},
	{0x8000, 0x8000, 0, 0, 0, 0, 0},
	{0x9000, 0x9000, 0, 0, 0, 0, 0},
	{0xa000, 0xa000, 0, 0, 0, 0, 0},
	{0xb000, 0xb000, 0, 0, 0, 0, 0},
	{0xc000, 0xc000, 0, 0, 0, 0, 0},
	{0xd000, 0xd000, 0, 0, 0, 0, 0},
	{0xe000, 0xe000, 0, 0, 0, 0, 0},
	{0xf000, 0xf000, 0, 0, 0, 0, 0},
	{0x10000, 0x10000, 0, 0, 0, 0, 0} // 64k
};

static const int bin_count = sizeof(bins) / sizeof(struct heap_bin);
static mutex heap_lock;

#if USE_CHECKING_WALL
sem_id sWallCheckLock = -1;
struct list sWalls;
#endif

#if PARANOID_POINTER_CHECK

#define PTRCHECKLIST_ENTRIES 8192
static void *ptrchecklist[PTRCHECKLIST_ENTRIES]; /* automatically initialized to zero */

static void
ptrchecklist_store(void *ptr)
{
	int i;
	mutex_lock(&heap_lock);
	for (i = 0; i != PTRCHECKLIST_ENTRIES; i++)
		if (ptrchecklist[i] == NULL) {
			ptrchecklist[i] = ptr;
			break;
		}
	mutex_unlock(&heap_lock);
	if (i == PTRCHECKLIST_ENTRIES)
		panic("Sorry, out of entries, increase PTRCHECKLIST_ENTRIES in heap.c\n");
}


static bool
ptrchecklist_remove(void *ptr)
{
	int i;
	bool found = false;
	mutex_lock(&heap_lock);
	for (i = 0; i != PTRCHECKLIST_ENTRIES; i++)
		if (ptrchecklist[i] == ptr) {
			ptrchecklist[i] = NULL;
			found = true;
			break;
		}
	mutex_unlock(&heap_lock);
	return found;
}

#endif /* PARANOID_POINTER_CHECK */

#if USE_WALL

static size_t
wall_size(size_t alignment)
{
	if (alignment == 0)
		return sizeof(struct front_wall) + sizeof(struct back_wall);

	return 2 * alignment;
}


static void *
get_base_address_from_wall(struct front_wall *wall)
{
	if (wall->alignment == 0)
		return (void *)wall;

	return (void *)((addr_t)wall + sizeof(struct front_wall) - wall->alignment);
}


static struct front_wall *
get_wall(void *address)
{
	return (struct front_wall *)((addr_t)address - sizeof(struct front_wall));
}


static void
set_wall(struct front_wall *wall, size_t size, size_t alignment)
{
	struct back_wall *backWall;
	uint32 i;

	size -= wall_size(alignment);

	wall->alignment = alignment;
	wall->size = size;

	for (i = 0; i < WALL_SIZE / sizeof(uint32); i++) {
		wall->wall[i] = 0xabadcafe;
	}

	backWall = (struct back_wall *)((addr_t)wall + sizeof(struct front_wall) + size);
	for (i = 0; i < WALL_SIZE / sizeof(uint32); i++) {
		backWall->wall[i] = 0xabadcafe;
	}
}


static void *
add_wall(void *address, size_t size, size_t alignment)
{
	struct front_wall *wall;

	if (alignment == 0)
		address = (uint8 *)address + sizeof(struct front_wall);
	else
		address = (uint8 *)address + alignment;

	wall = get_wall(address);
	set_wall(wall, size, alignment);

#if USE_CHECKING_WALL
	acquire_sem(sWallCheckLock);
	list_add_link_to_tail(&sWalls, &wall->link);
	release_sem(sWallCheckLock);
#endif
	return address;
}


void check_wall(void *address);
void
check_wall(void *address)
{
	struct front_wall *frontWall = get_wall(address);
	struct back_wall *backWall;
	uint32 i;

	for (i = 0; i < WALL_SIZE / 4; i++) {
		if (frontWall->wall[i] != 0xabadcafe) {
			panic("free: front wall %i (%p) was overwritten (allocation at %p, %lu bytes): %08lx\n",
				i, frontWall, address, frontWall->size, frontWall->wall[i]);
		}
	}

	backWall = (struct back_wall *)((uint8 *)address + frontWall->size);

	for (i = 0; i < WALL_SIZE / 4; i++) {
		if (backWall->wall[i] != 0xabadcafe) {
			panic("free: back wall %i (%p) was overwritten (allocation at %p, %lu bytes): %08lx\n",
				i, backWall, address, frontWall->size, backWall->wall[i]);
		}
	}
}

#endif	/* USE_WALL */

#if USE_CHECKING_WALL

static void
check_wall_daemon(void *arg, int iteration)
{
	struct front_wall *wall = NULL;

	acquire_sem(sWallCheckLock);

	while ((wall = list_get_next_item(&sWalls, wall)) != NULL) {
		check_wall((uint8 *)wall + sizeof(struct front_wall));
	}

	release_sem(sWallCheckLock);
}

#endif	/* USE_CHECKING_WALL */

static void
dump_bin(int bin_index)
{
	struct heap_bin *bin = &bins[bin_index];
	unsigned int *temp;

	dprintf("%d:\tesize %lu\tgrow_size %lu\talloc_count %lu\tfree_count %lu\traw_count %lu\traw_list %p\n",
		bin_index, bin->element_size, bin->grow_size, bin->alloc_count, bin->free_count, bin->raw_count, bin->raw_list);
	dprintf("free_list: ");
	for(temp = bin->free_list; temp != NULL; temp = (unsigned int *)*temp) {
		dprintf("%p ", temp);
	}
	dprintf("NULL\n");
}


static int
dump_bin_list(int argc, char **argv)
{
	int i;

	dprintf("%d heap bins at %p:\n", bin_count, bins);

	for (i = 0; i < bin_count; i++) {
		dump_bin(i);
	}
	return 0;
}


/* called from vm_init. The heap should already be mapped in at this point, we just
 * do a little housekeeping to set up the data structure.
 */

status_t
heap_init(addr_t heapBase)
{
	const unsigned int page_entries = B_PAGE_SIZE / sizeof(struct heap_page);
	// set some global pointers
	heap_alloc_table = (struct heap_page *)heapBase;
	heap_size = ((uint64)HEAP_SIZE * page_entries / (page_entries + 1)) & ~(B_PAGE_SIZE-1);
	heap_base = (unsigned int)heap_alloc_table + PAGE_ALIGN(heap_size / page_entries);
	heap_base_ptr = heap_base;
	TRACE(("heap_alloc_table = %p, heap_base = 0x%lx, heap_size = 0x%lx\n",
		heap_alloc_table, heap_base, heap_size));

	// zero out the heap alloc table at the base of the heap
	memset((void *)heap_alloc_table, 0, (heap_size / B_PAGE_SIZE) * sizeof(struct heap_page));

	// pre-init the mutex to at least fall through any semaphore calls
	heap_lock.sem = -1;
	heap_lock.holder = -1;

#if USE_CHECKING_WALL
	list_init(&sWalls);
#endif

	// set up some debug commands
	add_debugger_command("heap_bindump", &dump_bin_list, "dump stats about bin usage");

	return B_OK;
}


status_t
heap_init_post_sem(kernel_args *args)
{
	if (mutex_init(&heap_lock, "heap_mutex") < 0)
		panic("error creating heap mutex\n");

#if USE_CHECKING_WALL
	sWallCheckLock = create_sem(1, "check wall");
#endif
	return B_OK;
}


status_t
heap_init_post_thread(kernel_args *args)
{
#if USE_CHECKING_WALL
	register_kernel_daemon(check_wall_daemon, NULL, WALL_CHECK_FREQUENCY);
#endif
	return B_OK;
}


static char *
raw_alloc(unsigned int size, int bin_index)
{
	unsigned int new_heap_ptr;
	char *retval;
	struct heap_page *page;
	addr_t addr;

	new_heap_ptr = heap_base_ptr + PAGE_ALIGN(size);
	if (new_heap_ptr > heap_base + heap_size) {
		panic("heap overgrew itself!\n");
		return NULL;
	}

	for (addr = heap_base_ptr; addr < new_heap_ptr; addr += B_PAGE_SIZE) {
		page = &heap_alloc_table[(addr - heap_base) / B_PAGE_SIZE];
		page->in_use = 1;
		page->cleaning = 0;
		page->bin_index = bin_index;
		if (bin_index < bin_count && bins[bin_index].element_size < B_PAGE_SIZE)
			page->free_count = B_PAGE_SIZE / bins[bin_index].element_size;
		else
			page->free_count = 1;
	}

	retval = (char *)heap_base_ptr;
	heap_base_ptr = new_heap_ptr;
	return retval;
}


#if DEBUG
static bool
is_valid_alignment(size_t number)
{
	// this cryptic line accepts zero and all powers of two
	return ((~number + 1) | ((number << 1) - 1)) == ~0UL;
}
#endif


//	#pragma mark -


void *
memalign(size_t alignment, size_t size)
{
	void *address = NULL;
	int bin_index;
	unsigned int i;
	struct heap_page *page;

	TRACE(("memalign(alignment = %lu, size = %lu\n", alignment, size));

#if DEBUG
	if (!is_valid_alignment(alignment))
		panic("memalign() with an alignment which is not a power of 2\n");
#endif

	if (!kernel_startup && !are_interrupts_enabled())
		panic("malloc: called with interrupts disabled\n");

	mutex_lock(&heap_lock);

#if USE_WALL
	if (alignment > 0) {
		// make the alignment big enough to contain the front wall
		while (alignment < sizeof(struct front_wall))
			alignment *= 2;
	}

	size += wall_size(alignment);
#else
	// ToDo: that code "aligns" the buffer because the bins are always
	//	aligned on their bin size
	if (size < alignment)
		size = alignment;
#endif

	for (bin_index = 0; bin_index < bin_count; bin_index++)
		if (size <= bins[bin_index].element_size)
			break;

	if (bin_index == bin_count) {
		address = raw_alloc(size, bin_index);
		dprintf("heap: allocated big chunk (%ld bytes), it will never be freed!\n",
			size);
	} else {
		if (bins[bin_index].free_list != NULL) {
			address = bins[bin_index].free_list;
			bins[bin_index].free_list = (void *)(*(unsigned int *)bins[bin_index].free_list);
			bins[bin_index].free_count--;
		} else {
			if (bins[bin_index].raw_count == 0) {
				bins[bin_index].raw_list = raw_alloc(bins[bin_index].grow_size, bin_index);
				bins[bin_index].raw_count = bins[bin_index].grow_size / bins[bin_index].element_size;
			}

			bins[bin_index].raw_count--;
			address = bins[bin_index].raw_list;
			bins[bin_index].raw_list += bins[bin_index].element_size;
		}

		bins[bin_index].alloc_count++;
		page = &heap_alloc_table[((unsigned int)address - heap_base) / B_PAGE_SIZE];
		page[0].free_count--;

		TRACE(("kmalloc0: page %p: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count));

		for(i = 1; i < bins[bin_index].element_size / B_PAGE_SIZE; i++) {
			page[i].free_count--;
			TRACE(("kmalloc1: page 0x%x: bin_index %d, free_count %d\n", page[i], page[i].bin_index, page[i].free_count));
		}
	}

	mutex_unlock(&heap_lock);

	TRACE(("kmalloc: asked to allocate size %d, returning ptr = %p\n", size, address));

#if PARANOID_KMALLOC
	memset(address, 0xcc, size);
#endif

#if PARANOID_POINTER_CHECK
	ptrchecklist_store(address);
#endif

#if USE_WALL
	address = add_wall(address, size, alignment);
#endif

	return address;
}


void *
malloc(size_t size)
{
	return memalign(0, size);
}


void
free(void *address)
{
	struct heap_page *page;
	struct heap_bin *bin;
	unsigned int i;

	if (!kernel_startup && !are_interrupts_enabled())
		panic("free: called with interrupts disabled\n");

	if (address == NULL)
		return;

	if ((addr_t)address < heap_base || (addr_t)address >= (heap_base + heap_size))
		panic("free: asked to free invalid address %p\n", address);

#if USE_WALL
	{
		struct front_wall *wall = get_wall(address);

#if USE_CHECKING_WALL
		acquire_sem(sWallCheckLock);
		list_remove_link(&wall->link);
		release_sem(sWallCheckLock);
#endif
		check_wall(address);
		address = get_base_address_from_wall(wall);
	}
#endif

#if PARANOID_POINTER_CHECK
	if (!ptrchecklist_remove(address))
		panic("free(): asked to free invalid pointer %p\n", address);
#endif

	mutex_lock(&heap_lock);

	TRACE(("free(): asked to free at ptr = %p\n", address));

	page = &heap_alloc_table[((unsigned)address - heap_base) / B_PAGE_SIZE];

	TRACE(("free(): page %p: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count));

	if (page[0].bin_index > bin_count)
		panic("free(): page %p: invalid bin_index %d\n", page, page->bin_index);

	if (page[0].bin_index < bin_count) {
		bin = &bins[page[0].bin_index];

		if (bin->element_size <= B_PAGE_SIZE && (addr_t)address % bin->element_size != 0)
			panic("kfree: passed invalid pointer %p! Supposed to be in bin for esize 0x%x\n", address, bin->element_size);

#if PARANOID_KFREE
		// mark the free space as freed
		{
			uint32 deadbeef = 0xdeadbeef;
			uint8 *dead = (uint8 *)address;
			uint32 i;

			// the first 4 bytes are overwritten with the next free list pointer later
			for (i = 4; i < bin->element_size; i++)
				dead[i] = ((uint8 *)&deadbeef)[i % 4];
		}
#endif

		for (i = 0; i < bin->element_size / B_PAGE_SIZE; i++) {
			if (page[i].bin_index != page[0].bin_index)
				panic("free(): not all pages in allocation match bin_index\n");
			page[i].free_count++;
		}

#if PARANOID_KFREE
		// walk the free list on this bin to make sure this address doesn't exist already
		{
			unsigned int *temp;
			for (temp = bin->free_list; temp != NULL; temp = (unsigned int *)*temp) {
				if (temp == (unsigned int *)address)
					panic("free(): address %p already exists in bin free list\n", address);
			}
		}
#endif

		*(unsigned int *)address = (unsigned int)bin->free_list;
		bin->free_list = address;
		bin->alloc_count--;
		bin->free_count++;
	} else {
		// this chunk has been allocated via raw_alloc() directly
		// TODO: since the heap must be replaced anyway, we don't
		//	free this allocation anymore... (tracking them would
		//	require some extra stuff)
	}

	mutex_unlock(&heap_lock);
}


/** Naive implementation of realloc() - it's very simple but
 *	it's there and working.
 *	It takes the bin of the current allocation if the new size
 *	fits in and is larger than the size of the next smaller bin.
 *	If not, it allocates a new chunk of memory, and copies and
 *	frees the old buffer.
 */

void *
realloc(void *address, size_t newSize)
{
	void *newAddress = NULL;
	size_t maxSize = 0, minSize;

	if (!kernel_startup && !are_interrupts_enabled())
		panic("realloc(): called with interrupts disabled\n");

	if (address != NULL && ((addr_t)address < heap_base || (addr_t)address >= (heap_base + heap_size)))
		panic("realloc(): asked to realloc invalid address %p\n", address);

	if (newSize == 0) {
		free(address);
		return NULL;
	}

	// find out the size of the old allocation first

	if (address != NULL) {
		struct heap_page *page;
#if USE_WALL
		struct front_wall *wall = get_wall(address);
#endif

		mutex_lock(&heap_lock);
		page = &heap_alloc_table[((unsigned)address - heap_base) / B_PAGE_SIZE];

		TRACE(("realloc(): page %p: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count));

		if (page[0].bin_index >= bin_count)
			panic("realloc(): page %p: invalid bin_index %d\n", page, page->bin_index);

		maxSize = bins[page[0].bin_index].element_size;
		minSize = page[0].bin_index > 0 ? bins[page[0].bin_index - 1].element_size : 0;

		mutex_unlock(&heap_lock);

#if USE_WALL
		newSize += wall_size(wall->alignment);
#endif

		// does the new allocation simply fit in the bin?
		if (newSize > minSize && newSize <= maxSize) {
#if USE_WALL
			check_wall(address);

			// we need to move the back wall, and honour the
			// alignment so that the address stays the same
			set_wall(wall, newSize, wall->alignment);
#endif
			return address;
		}
	}

	// if not, allocate a new chunk of memory
	newAddress = malloc(newSize);
	if (newAddress == NULL)
		return NULL;

	// copy the old data and free the old allocation
	if (address) {
		// we do have the maxSize of the bin at this point
		memcpy(newAddress, address, min(maxSize, newSize));
		free(address);
	}

	return newAddress;
}


void *
calloc(size_t numElements, size_t size)
{
	void *address = malloc(numElements * size);
	if (address != NULL)
		memset(address, 0, numElements * size);

	return address;
}

