/*
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
/* does currently not work correctly, because the VM malloc()s a PAGE_SIZE and
 * later checks if the returned address is aligned...
 */
#define USE_WALL 0


// heap stuff
// ripped mostly from nujeffos

struct heap_page {
	unsigned short bin_index : 5;
	unsigned short free_count : 9;
	unsigned short cleaning : 1;
	unsigned short in_use : 1;
} PACKED;

static struct heap_page *heap_alloc_table;
static addr heap_base_ptr;
static addr heap_base;
static addr heap_size;

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
	{16, PAGE_SIZE, 0, 0, 0, 0, 0},
	{32, PAGE_SIZE, 0, 0, 0, 0, 0},
	{64, PAGE_SIZE, 0, 0, 0, 0, 0},
	{128, PAGE_SIZE, 0, 0, 0, 0, 0},
	{256, PAGE_SIZE, 0, 0, 0, 0, 0},
	{512, PAGE_SIZE, 0, 0, 0, 0, 0},
	{1024, PAGE_SIZE, 0, 0, 0, 0, 0},
	{2048, PAGE_SIZE, 0, 0, 0, 0, 0},
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

int
heap_init(addr new_heap_base)
{
	const unsigned int page_entries = PAGE_SIZE / sizeof(struct heap_page);
	// set some global pointers
	heap_alloc_table = (struct heap_page *)new_heap_base;
	heap_size = ((uint64)HEAP_SIZE * page_entries / (page_entries + 1)) & ~(PAGE_SIZE-1);
	heap_base = (unsigned int)heap_alloc_table + PAGE_ALIGN(heap_size / page_entries);
	heap_base_ptr = heap_base;
	dprintf("heap_alloc_table = %p, heap_base = 0x%lx, heap_size = 0x%lx\n", heap_alloc_table, heap_base, heap_size);

	// zero out the heap alloc table at the base of the heap
	memset((void *)heap_alloc_table, 0, (heap_size / PAGE_SIZE) * sizeof(struct heap_page));

	// pre-init the mutex to at least fall through any semaphore calls
	heap_lock.sem = -1;
	heap_lock.holder = -1;

	// set up some debug commands
	add_debugger_command("heap_bindump", &dump_bin_list, "dump stats about bin usage");

	return 0;
}


int
heap_init_postsem(kernel_args *ka)
{
	if (mutex_init(&heap_lock, "heap_mutex") < 0)
		panic("error creating heap mutex\n");

	return 0;
}


static char *
raw_alloc(unsigned int size, int bin_index)
{
	unsigned int new_heap_ptr;
	char *retval;
	struct heap_page *page;
	unsigned int addr;

	new_heap_ptr = heap_base_ptr + PAGE_ALIGN(size);
	if (new_heap_ptr > heap_base + heap_size)
		panic("heap overgrew itself!\n");

	for (addr = heap_base_ptr; addr < new_heap_ptr; addr += PAGE_SIZE) {
		page = &heap_alloc_table[(addr - heap_base) / PAGE_SIZE];
		page->in_use = 1;
		page->cleaning = 0;
		page->bin_index = bin_index;
		if (bin_index < bin_count && bins[bin_index].element_size < PAGE_SIZE)
			page->free_count = PAGE_SIZE / bins[bin_index].element_size;
		else
			page->free_count = 1;
	}

	retval = (char *)heap_base_ptr;
	heap_base_ptr = new_heap_ptr;
	return retval;
}


//	#pragma mark -


void *
malloc(size_t size)
{
	void *address = NULL;
	int bin_index;
	unsigned int i;
	struct heap_page *page;

	TRACE(("kmalloc: asked to allocate size %d\n", size));
	
	if (!kernel_startup && !are_interrupts_enabled())
		panic("malloc: called with interrupts disabled\n");

	mutex_lock(&heap_lock);

#if USE_WALL
	// The wall uses 4 bytes to store the actual length of the requested
	// block, 8 bytes for the front wall, and 8 bytes for the back wall
	size += 20;
#endif

	for (bin_index = 0; bin_index < bin_count; bin_index++)
		if (size <= bins[bin_index].element_size)
			break;

	if (bin_index == bin_count) {
		// XXX fix the raw alloc later.
		//address = raw_alloc(size, bin_index);
		panic("malloc: asked to allocate too much for now!\n");
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
		page = &heap_alloc_table[((unsigned int)address - heap_base) / PAGE_SIZE];
		page[0].free_count--;

		TRACE(("kmalloc0: page %p: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count));

		for(i = 1; i < bins[bin_index].element_size / PAGE_SIZE; i++) {
			page[i].free_count--;
			TRACE(("kmalloc1: page 0x%x: bin_index %d, free_count %d\n", page[i], page[i].bin_index, page[i].free_count));
		}
	}

out:
	mutex_unlock(&heap_lock);

	TRACE(("kmalloc: asked to allocate size %d, returning ptr = %p\n", size, address));

#if PARANOID_KMALLOC
	memset(address, 0xCC, size);
#endif

#if PARANOID_POINTER_CHECK
	ptrchecklist_store(address);
#endif

#if USE_WALL
	{
		uint32 *wall = address;
		size -= 20;

		wall[0] = size;
		wall[1] = 0xabadcafe;
		wall[2] = 0xabadcafe;

		address = (uint8 *)address + 12;

		wall = (uint32 *)((uint8 *)address + size);
		wall[0] = 0xabadcafe;
		wall[1] = 0xabadcafe;
	}
#endif

	return address;
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

	if ((addr)address < heap_base || (addr)address >= (heap_base + heap_size))
		panic("free: asked to free invalid address %p\n", address);

#if USE_WALL
	{
		uint32 *wall = (uint32 *)((uint8 *)address - 12);
		uint32 size = wall[0];
		if (wall[1] != 0xabadcafe || wall[2] != 0xabadcafe)
			panic("free: front wall was overwritten (allocation at %p, %lu bytes): %08lx %08lx\n", address, size, wall[1], wall[2]);

		wall = (uint32 *)((uint8 *)address + size);
		if (wall[0] != 0xabadcafe || wall[1] != 0xabadcafe)
			panic("free: back wall was overwritten (allocation at %p, %lu bytes): %08lx %08lx\n", address, size, wall[0], wall[1]);

		address = (uint8 *)address - 12;
	}
#endif

#if PARANOID_POINTER_CHECK
	if (!ptrchecklist_remove(address))
		panic("free(): asked to free invalid pointer %p\n", address);
#endif

	mutex_lock(&heap_lock);

	TRACE(("free(): asked to free at ptr = %p\n", address));

	page = &heap_alloc_table[((unsigned)address - heap_base) / PAGE_SIZE];

	TRACE(("free(): page %p: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count));

	if (page[0].bin_index >= bin_count)
		panic("free(): page %p: invalid bin_index %d\n", page, page->bin_index);

	bin = &bins[page[0].bin_index];

	if (bin->element_size <= PAGE_SIZE && (addr)address % bin->element_size != 0)
		panic("kfree: passed invalid pointer %p! Supposed to be in bin for esize 0x%x\n", address, bin->element_size);

#if PARANOID_KFREE
	// mark the free space as freed
	{
		uint32 deadbeef = 0xdeadbeef;
		uint8 *dead = (uint8 *)address;
		int32 i;

		// the first 4 bytes are overwritten with the next free list pointer later
		for (i = 4; i < bin->element_size; i++)
			dead[i] = ((uint8 *)&deadbeef)[i % 4];
	}
#endif

	for (i = 0; i < bin->element_size / PAGE_SIZE; i++) {
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

	if (address != NULL && ((addr)address < heap_base || (addr)address >= (heap_base + heap_size)))
		panic("realloc(): asked to realloc invalid address %p\n", address);

	if (newSize == 0) {
		free(address);
		return NULL;
	}

	// find out the size of the old allocation first

	if (address != NULL) {
		struct heap_page *page;

		mutex_lock(&heap_lock);
		page = &heap_alloc_table[((unsigned)address - heap_base) / PAGE_SIZE];
	
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

/*
char *
kstrdup(const char *text)
{
	char *buf = (char *)kmalloc(strlen(text) + 1);

	if (buf != NULL)
		strcpy(buf, text);
	return buf;
}
*/
