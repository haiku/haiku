/* Heap + other assorted stuff. Needs cleanup */

/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <vm.h>
#include <lock.h>
#include <memheap.h>
#include <debug.h>

#include <arch/cpu.h>

#include <stage2.h>

#include <string.h>

#define PARANOID_KFREE 1
#define MAKE_NOIZE 0

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

static void dump_bin_list(int argc, char **argv)
{
	int i;

	dprintf("%d heap bins at %p:\n", bin_count, bins);

	for(i=0; i<bin_count; i++) {
		dump_bin(i);
	}
}

// called from vm_init. The heap should already be mapped in at this point, we just
// do a little housekeeping to set up the data structure.
int heap_init(addr new_heap_base, unsigned int new_heap_size)
{
	// set some global pointers
	heap_alloc_table = (struct heap_page *)new_heap_base;
	heap_size = new_heap_size;
	heap_base = PAGE_ALIGN((unsigned int)heap_alloc_table + (heap_size / PAGE_SIZE) * sizeof(struct heap_page));
	heap_base_ptr = heap_base;
	dprintf("heap_alloc_table = %p, heap_base = 0x%lx, heap_size = 0x%lx\n", heap_alloc_table, heap_base, heap_size);

	// zero out the heap alloc table at the base of the heap
	memset((void *)heap_alloc_table, 0, (heap_size / PAGE_SIZE) * sizeof(struct heap_page));

	// pre-init the mutex to at least fall through any semaphore calls
	heap_lock.sem = -1;
	heap_lock.count = 0;

	// set up some debug commands
	dbg_add_command(&dump_bin_list, "heap_bindump", "dump stats about bin usage");

	return 0;
}

int heap_init_postsem(kernel_args *ka)
{
	if(mutex_init(&heap_lock, "heap_mutex") < 0) {
		panic("error creating heap mutex\n");
	}
	return 0;
}

static char *raw_alloc(unsigned int size, int bin_index)
{
	unsigned int new_heap_ptr;
	char *retval;
	struct heap_page *page;
	unsigned int addr;

	new_heap_ptr = heap_base_ptr + PAGE_ALIGN(size);
	if(new_heap_ptr > heap_base + heap_size) {
		panic("heap overgrew itself!\n");
	}

	for(addr = heap_base_ptr; addr < new_heap_ptr; addr += PAGE_SIZE) {
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

void *kmalloc(unsigned int size)
{
	void *address = NULL;
	int bin_index;
	unsigned int i;
	struct heap_page *page;

#if MAKE_NOIZE
	dprintf("kmalloc: asked to allocate size %d\n", size);
#endif

	mutex_lock(&heap_lock);

	for (bin_index = 0; bin_index < bin_count; bin_index++)
		if (size <= bins[bin_index].element_size)
			break;

	if (bin_index == bin_count) {
		// XXX fix the raw alloc later.
		//address = raw_alloc(size, bin_index);
		panic("kmalloc: asked to allocate too much for now!\n");
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
#if MAKE_NOIZE
		dprintf("kmalloc0: page 0x%x: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count);
#endif
		for(i = 1; i < bins[bin_index].element_size / PAGE_SIZE; i++) {
			page[i].free_count--;
#if MAKE_NOIZE
			dprintf("kmalloc1: page 0x%x: bin_index %d, free_count %d\n", page[i], page[i].bin_index, page[i].free_count);
#endif
		}
	}

out:
	mutex_unlock(&heap_lock);

#if MAKE_NOIZE
	dprintf("kmalloc: asked to allocate size %d, returning ptr = %p\n", size, address);
#endif
	return address;
}

void kfree(void *address)
{
	struct heap_page *page;
	struct heap_bin *bin;
	unsigned int i;

	if (address == NULL)
		return;

	if ((addr)address < heap_base || (addr)address >= (heap_base + heap_size))
		panic("kfree: asked to free invalid address %p\n", address);

	mutex_lock(&heap_lock);

#if MAKE_NOIZE
	dprintf("kfree: asked to free at ptr = %p\n", address);
#endif

	page = &heap_alloc_table[((unsigned)address - heap_base) / PAGE_SIZE];

#if MAKE_NOIZE
	dprintf("kfree: page 0x%x: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count);
#endif

	if(page[0].bin_index >= bin_count)
		panic("kfree: page %p: invalid bin_index %d\n", page, page->bin_index);

	bin = &bins[page[0].bin_index];

//	if((addr)address % bin->element_size != 0)
//		panic("kfree: passed invalid pointer 0x%x! Supposed to be in bin for esize 0x%x\n", address, bin->element_size);

	for(i = 0; i < bin->element_size / PAGE_SIZE; i++) {
		if(page[i].bin_index != page[0].bin_index)
			panic("kfree: not all pages in allocation match bin_index\n");
		page[i].free_count++;
	}

#if PARANOID_KFREE
	// walk the free list on this bin to make sure this address doesn't exist already
	{
		unsigned int *temp;
		for(temp = bin->free_list; temp != NULL; temp = (unsigned int *)*temp) {
			if(temp == (unsigned int *)address) {
				panic("kfree: address %p already exists in bin free list\n", address);
			}
		}
	}
#endif

	*(unsigned int *)address = (unsigned int)bin->free_list;
	bin->free_list = address;
	bin->alloc_count--;
	bin->free_count++;

//out:
	mutex_unlock(&heap_lock);
}

char *kstrdup(const char *text)
{
	char *buf = (char *)kmalloc(strlen(text) + 1);

	if(buf != NULL)
		strcpy(buf,text);
	return buf;
}

