/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
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

#define TRACE_HEAP 0
#if TRACE_HEAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

/* prevent freeing pointers that were not allocated by kmalloc or are already freeed */
#define PARANOID_POINTER_CHECK 1
/* initialize newly allocated memory with something non zero */  
#define PARANOID_KMALLOC 1
/* check if freed pointers are already freed, and fill freed memory with 0xdeadbeef */  
#define PARANOID_KFREE 1
/* use a back and front wall around each allocation */
#define USE_WALL 0
#define USE_CHECKING_WALL 0

#define WALL_SIZE 8
	/* must be a multiple of 4 */

#if USE_CHECKING_WALL
	/* Change to allow (struct list_link) + 2 * uint32 + WALL_SIZE */
#	define WALL_MIN_ALIGN 32
#	define WALL_CHECK_FREQUENCY	1 /* every tenth second */
#else
#	define WALL_MIN_ALIGN 16
#endif


// heap stuff
// ripped mostly from nujeffos

struct heap_page {
	unsigned short bin_index : 5;
	unsigned short free_count : 9;
	unsigned short cleaning : 1;
	unsigned short in_use : 1;
} PACKED;

static struct heap_page *heap_alloc_table;
static addr_t heap_base_ptr;
static addr_t heap_base;
static addr_t heap_size;

struct heap_bin {
	unsigned int element_size;
	unsigned int grow_size;
	unsigned int alloc_count;
	void *free_list;
	unsigned int free_count;
	char *raw_list;
	unsigned int raw_count;
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

static void
check_wall(void *address)
{
	uint32 *wall = (uint32 *)((uint8 *)address - WALL_SIZE - 8);
	uint32 size = wall[1];

	if (wall[2] != 0xabadcafe || wall[3] != 0xabadcafe)
		panic("free: front wall was overwritten (allocation at %p, %lu bytes): %08lx %08lx\n", address, size, wall[1], wall[2]);

	wall = (uint32 *)((uint8 *)address + size);
	if (wall[0] != 0xabadcafe || wall[1] != 0xabadcafe)
		panic("free: back wall was overwritten (allocation at %p, %lu bytes): %08lx %08lx\n", address, size, wall[0], wall[1]);
}

#endif	/* USE_WALL */

#if USE_CHECKING_WALL

sem_id sWallCheckLock = -1;
struct list sWalls;

static void
check_wall_daemon(void *arg, int iteration)
{
	struct list_link *link = NULL;
	uint32 *wall;

dprintf("checking walls...\n");
	acquire_sem(sWallCheckLock);

	while ((link = list_get_next_item(&sWalls, link)) != NULL) {
		wall = (uint32 *)((addr_t)link + WALL_SIZE + 8);
		check_wall(wall);
	}

	release_sem(sWallCheckLock);
}

#endif	/* USE_CHECKING_WALL */

static void dump_bin(int bin_index)
{
	struct heap_bin *bin = &bins[bin_index];
	unsigned int *temp;

	dprintf("%d:\tesize %d\tgrow_size %d\talloc_count %d\tfree_count %d\traw_count %d\traw_list %p\n",
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

	for (i = 0; i<bin_count; i++) {
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

	// set up some debug commands
	add_debugger_command("heap_bindump", &dump_bin_list, "dump stats about bin usage");

	return B_OK;
}


status_t
heap_init_postsem(kernel_args *ka)
{
	if (mutex_init(&heap_lock, "heap_mutex") < 0)
		panic("error creating heap mutex\n");

#if USE_CHECKING_WALL
	sWallCheckLock = create_sem(1, "check wall");
if (sWallCheckLock == 0)
	panic("AAAaaaaargl.\n");
	list_init(&sWalls);
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
	if (new_heap_ptr > heap_base + heap_size)
		panic("heap overgrew itself!\n");

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
	// The wall uses 4 bytes to store the actual length of the requested
	// block, 4 bytes for the alignment offset, WALL_SIZE, and eventually
	// a list_link in case USE_CHECKING_WALL is defined
	if (alignment < WALL_MIN_ALIGN)
		alignment = WALL_MIN_ALIGN;

#if USE_CHECKING_WALL
	size += sizeof(struct list_link);
#endif
	size += 2*WALL_SIZE + 8 + 2*alignment;
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
		// XXX fix the raw alloc later.
		//address = raw_alloc(size, bin_index);
		panic("malloc: asked to allocate too much for now (%lu bytes)!\n", size);
		goto out;
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

out:
	mutex_unlock(&heap_lock);

	TRACE(("kmalloc: asked to allocate size %d, returning ptr = %p\n", size, address));

#if PARANOID_KMALLOC
	memset(address, 0xcc, size);
#endif

#if PARANOID_POINTER_CHECK
	ptrchecklist_store(address);
#endif

#if USE_WALL
	{
		uint32 *wall = (uint32 *)((addr_t)address + alignment - WALL_SIZE - 8);
#if USE_CECKING_WALL
		struct list_link *link = (struct list_link)wall - 1;
		
		acquire_sem(sCheckWallLock);
		list_add_link_to_tail(&sWalls, link);
		list_remove_link(link);
		release_sem(sCheckWallLock);
		size -= sizeof(struct list_link);
#endif
		size -= 8 + 2*WALL_SIZE + 2*alignment;

		wall[0] = alignment;
		wall[1] = size;
		wall[2] = 0xabadcafe;
		wall[3] = 0xabadcafe;

		address = (uint8 *)wall + 16;

		wall = (uint32 *)((uint8 *)address + size);
		wall[0] = 0xabadcafe;
		wall[1] = 0xabadcafe;
	}
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
		uint32 *wall = (uint32 *)((uint8 *)address - WALL_SIZE - 8);
		uint32 alignOffset = wall[0];

#if USE_CECKING_WALL
		struct list_link *link = (struct list_link)wall - 1;

		acquire_sem(sCheckWallLock);
		list_remove_link(link);
		release_sem(sCheckWallLock);
#endif
		check_wall(address);
		address = (uint8 *)address - alignOffset;
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

	if (page[0].bin_index >= bin_count)
		panic("free(): page %p: invalid bin_index %d\n", page, page->bin_index);

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

		mutex_lock(&heap_lock);
		page = &heap_alloc_table[((unsigned)address - heap_base) / B_PAGE_SIZE];
	
		TRACE(("realloc(): page %p: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count));
	
		if (page[0].bin_index >= bin_count)
			panic("realloc(): page %p: invalid bin_index %d\n", page, page->bin_index);
	
		maxSize = bins[page[0].bin_index].element_size;
		minSize = page[0].bin_index > 0 ? bins[page[0].bin_index - 1].element_size : 0;
	
		mutex_unlock(&heap_lock);
	
		// does the new allocation simply fit in the bin?
		if (newSize > minSize && newSize < maxSize)
			return address;
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

