/*
 * Copyright 2008, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#include <debug.h>
#include <heap.h>
#include <int.h>
#include <kernel.h>
#include <lock.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <vm.h>

//#define TRACE_HEAP
#ifdef TRACE_HEAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// initialize newly allocated memory with something non zero
#define PARANOID_KMALLOC 1
// check for double free, and fill freed memory with 0xdeadbeef
#define PARANOID_KFREE 1
// validate sanity of the heap after each operation (slow!)
#define PARANOID_VALIDATION 0

typedef struct heap_page_s {
	uint16			index;
	uint16			bin_index : 5;
	uint16			free_count : 10;
	uint16			in_use : 1;
	heap_page_s *	next;
	heap_page_s *	prev;
	uint16			empty_index;
	addr_t *		free_list;
} heap_page;

// used for bin == bin_count allocations
#define allocation_id free_count

typedef struct heap_bin_s {
	uint32		element_size;
	uint16		max_free_count;
	heap_page *	page_list; // sorted so that the desired page is always first
} heap_bin;

typedef struct heap_allocator_s {
	addr_t		base;
	size_t		size;
	mutex		lock;
	vint32		large_alloc_id;

	uint32		bin_count;
	uint32		page_count;
	heap_page *	free_pages;

	heap_bin *	bins;
	heap_page *	page_table;

	heap_allocator_s *	next;
} heap_allocator;

static heap_allocator *sHeapList = NULL;
static heap_allocator *sLastGrowRequest = NULL;
static sem_id sHeapGrowSem = -1;
static sem_id sHeapGrownNotify = -1;


static void
dump_page(heap_page *page)
{
	uint32 count = 0;
	for (addr_t *temp = page->free_list; temp != NULL; temp = (addr_t *)*temp)
		count++;

	dprintf("\t\tpage %p: bin_index: %u; free_count: %u; empty_index: %u; free_list %p (%lu entr%s)\n",
		page, page->bin_index, page->free_count, page->empty_index,
		page->free_list, count, count == 1 ? "y" : "ies");
}


static void
dump_bin(heap_bin *bin)
{
	dprintf("\telement_size: %lu; max_free_count: %u; page_list %p;\n",
		bin->element_size, bin->max_free_count, bin->page_list);

	for (heap_page *temp = bin->page_list; temp != NULL; temp = temp->next)
		dump_page(temp);
}


static void
dump_allocator(heap_allocator *heap)
{
	uint32 count = 0;
	for (heap_page *page = heap->free_pages; page != NULL; page = page->next)
		count++;

	dprintf("allocator %p: base: 0x%08lx; size: %lu; bin_count: %lu; free_pages: %p (%lu entr%s)\n", heap,
		heap->base, heap->size, heap->bin_count, heap->free_pages, count,
		count == 1 ? "y" : "ies");

	for (uint32 i = 0; i < heap->bin_count; i++)
		dump_bin(&heap->bins[i]);

	dprintf("\n");
}


static int
dump_heap_list(int argc, char **argv)
{
	heap_allocator *heap = sHeapList;
	while (heap) {
		dump_allocator(heap);
		heap = heap->next;
	}

	return 0;
}


#if PARANOID_VALIDATION
static void
heap_validate_heap(heap_allocator *heap)
{
	mutex_lock(&heap->lock);

	// validate the free pages list
	uint32 freePageCount = 0;
	heap_page *lastPage = NULL;
	heap_page *page = heap->free_pages;
	while (page) {
		if ((addr_t)page < (addr_t)&heap->page_table[0]
			|| (addr_t)page >= (addr_t)&heap->page_table[heap->page_count])
			panic("free page is not part of the page table\n");

		if (page->index >= heap->page_count)
			panic("free page has invalid index\n");

		if ((addr_t)&heap->page_table[page->index] != (addr_t)page)
			panic("free page index does not lead to target page\n");

		if (page->prev != lastPage)
			panic("free page entry has invalid prev link\n");

		if (page->in_use)
			panic("free page marked as in use\n");

		lastPage = page;
		page = page->next;
		freePageCount++;
	}

	// validate the page table
	uint32 usedPageCount = 0;
	for (uint32 i = 0; i < heap->page_count; i++) {
		if (heap->page_table[i].in_use)
			usedPageCount++;
	}

	if (freePageCount + usedPageCount != heap->page_count) {
		panic("free pages and used pages do not add up (%lu + %lu != %lu)\n",
			freePageCount, usedPageCount, heap->page_count);
	}

	// validate the bins
	for (uint32 i = 0; i < heap->bin_count; i++) {
		heap_bin *bin = &heap->bins[i];

		lastPage = NULL;
		page = bin->page_list;
		int32 lastFreeCount = 0;
		while (page) {
			if ((addr_t)page < (addr_t)&heap->page_table[0]
				|| (addr_t)page >= (addr_t)&heap->page_table[heap->page_count])
				panic("used page is not part of the page table\n");

			if (page->index >= heap->page_count)
				panic("used page has invalid index\n");

			if ((addr_t)&heap->page_table[page->index] != (addr_t)page)
				panic("used page index does not lead to target page\n");

			if (page->prev != lastPage)
				panic("used page entry has invalid prev link (%p vs %p bin %lu)\n",
					page->prev, lastPage, i);

			if (!page->in_use)
				panic("used page marked as not in use\n");

			if (page->bin_index != i)
				panic("used page with bin index %u in page list of bin %lu\n",
					page->bin_index, i);

			if (page->free_count < lastFreeCount)
				panic("ordering of bin page list broken\n");

			// validate the free list
			uint32 freeSlotsCount = 0;
			addr_t *element = page->free_list;
			addr_t pageBase = heap->base + page->index * B_PAGE_SIZE;
			while (element) {
				if ((addr_t)element < pageBase
					|| (addr_t)element >= pageBase + B_PAGE_SIZE)
					panic("free list entry out of page range\n");

				if (((addr_t)element - pageBase) % bin->element_size != 0)
					panic("free list entry not on a element boundary\n");

				element = (addr_t *)*element;
				freeSlotsCount++;
			}

			uint32 slotCount = bin->max_free_count;
			if (page->empty_index > slotCount)
				panic("empty index beyond slot count (%u with %lu slots)\n",
					page->empty_index, slotCount);

			freeSlotsCount += (slotCount - page->empty_index);
			if (freeSlotsCount > slotCount)
				panic("more free slots than fit into the page\n");

			lastPage = page;
			lastFreeCount = page->free_count;
			page = page->next;
		}
	}

	mutex_unlock(&heap->lock);
}
#endif


heap_allocator *
heap_attach(addr_t base, size_t size, bool postSem)
{
	heap_allocator *heap = (heap_allocator *)base;
	base += sizeof(heap_allocator);
	size -= sizeof(heap_allocator);

	size_t binSizes[] = { 16, 32, 64, 96, 128, 192, 256, 384, 512, 1024, 2048, B_PAGE_SIZE };
	uint32 binCount = sizeof(binSizes) / sizeof(binSizes[0]);
	heap->bin_count = binCount;
	heap->bins = (heap_bin *)base;
	base += binCount * sizeof(heap_bin);
	size -= binCount * sizeof(heap_bin);

	for (uint32 i = 0; i < binCount; i++) {
		heap_bin *bin = &heap->bins[i];
		bin->element_size = binSizes[i];
		bin->max_free_count = B_PAGE_SIZE / binSizes[i];
		bin->page_list = NULL;
	}

	uint32 pageCount = size / B_PAGE_SIZE;
	size_t pageTableSize = pageCount * sizeof(heap_page);
	heap->page_table = (heap_page *)base;
	base += pageTableSize;
	size -= pageTableSize;

	// the rest is now actually usable memory (rounded to the next page)
	heap->base = (addr_t)(base + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;
	heap->size = (size_t)(size / B_PAGE_SIZE) * B_PAGE_SIZE;

	// now we know the real page count
	pageCount = heap->size / B_PAGE_SIZE;
	heap->page_count = pageCount;

	// zero out the heap alloc table at the base of the heap
	memset((void *)heap->page_table, 0, pageTableSize);
	for (uint32 i = 0; i < pageCount; i++)
		heap->page_table[i].index = i;

	// add all pages up into the free pages list
	for (uint32 i = 1; i < pageCount; i++) {
		heap->page_table[i - 1].next = &heap->page_table[i];
		heap->page_table[i].prev = &heap->page_table[i - 1];
	}
	heap->free_pages = &heap->page_table[0];
	heap->page_table[0].prev = NULL;

	if (postSem) {
		if (mutex_init(&heap->lock, "heap_mutex") < 0) {
			panic("heap_attach(): error creating heap mutex\n");
			return NULL;
		}
	} else {
		// pre-init the mutex to at least fall through any semaphore calls
		heap->lock.sem = -1;
		heap->lock.holder = -1;
	}

	heap->next = NULL;
	dprintf("heap_attach: attached to %p - usable range 0x%08lx - 0x%08lx\n",
		heap, heap->base, heap->base + heap->size);
	return heap;
}


static inline uint32
heap_next_alloc_id(heap_allocator *heap)
{
	return atomic_add(&heap->large_alloc_id, 1) & ((1 << 9) - 1);
}


static inline void
heap_link_page(heap_page *page, heap_page **list)
{
	page->prev = NULL;
	page->next = *list;
	if (page->next)
		page->next->prev = page;
	*list = page;
}


static inline void
heap_unlink_page(heap_page *page, heap_page **list)
{
	if (page->prev)
		page->prev->next = page->next;
	if (page->next)
		page->next->prev = page->prev;
	if (list && *list == page) {
		*list = page->next;
		if (page->next)
			page->next->prev = NULL;
	}
}


static void *
heap_raw_alloc(heap_allocator *heap, size_t size, uint32 binIndex)
{
	heap_bin *bin = NULL;
	if (binIndex < heap->bin_count)
		bin = &heap->bins[binIndex];

	if (bin && bin->page_list != NULL) {
		// we have a page where we have a free slot
		void *address = NULL;
		heap_page *page = bin->page_list;
		if (page->free_list) {
			// there's a previously freed entry we can use
			address = page->free_list;
			page->free_list = (addr_t *)*page->free_list;
		} else {
			// the page hasn't been fully allocated so use the next empty_index
			address = (void *)(heap->base + page->index * B_PAGE_SIZE
				+ page->empty_index * bin->element_size);
			page->empty_index++;
		}

		page->free_count--;
		if (page->free_count == 0) {
			// the page is now full so we remove it from the page_list
			bin->page_list = page->next;
			if (page->next)
				page->next->prev = NULL;
			page->next = page->prev = NULL;
		}

		return address;
	}

	// we don't have anything free right away, we must allocate a new page
	if (heap->free_pages == NULL) {
		// there are no free pages anymore, we ran out of memory
		TRACE(("heap %p: no free pages to allocate %lu bytes\n", heap, size));
		return NULL;
	}

	if (bin) {
		// small allocation, just grab the next free page
		heap_page *page = heap->free_pages;
		heap->free_pages = page->next;
		if (page->next)
			page->next->prev = NULL;

		page->in_use = 1;
		page->bin_index = binIndex;
		page->free_count = bin->max_free_count - 1;
		page->empty_index = 1;
		page->free_list = NULL;
		page->next = page->prev = NULL;

		if (page->free_count > 0) {
			// by design there are no other pages in the bins page list
			bin->page_list = page;
		}

		// we return the first slot in this page
		return (void *)(heap->base + page->index * B_PAGE_SIZE);
	}

	// large allocation, we must search for contiguous slots
	bool found = false;
	int32 first = -1;
	for (uint32 i = 0; i < heap->page_count; i++) {
		if (heap->page_table[i].in_use) {
			first = -1;
			continue;
		}

		if (first > 0) {
			if ((1 + i - first) * B_PAGE_SIZE >= size) {
				found = true;
				break;
			}
		} else
			first = i;
	}

	if (!found) {
		TRACE(("heap %p: found no contiguous pages to allocate %ld bytes\n", heap, size));
		return NULL;
	}

	uint32 allocationID = heap_next_alloc_id(heap);
	uint32 pageCount = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	for (uint32 i = first; i < first + pageCount; i++) {
		heap_page *page = &heap->page_table[i];
		page->in_use = 1;
		page->bin_index = binIndex;

		heap_unlink_page(page, &heap->free_pages);

		page->next = page->prev = NULL;
		page->free_list = NULL;
		page->allocation_id = allocationID;
	}

	return (void *)(heap->base + first * B_PAGE_SIZE);
}


#if DEBUG
static bool
is_valid_alignment(size_t number)
{
	// this cryptic line accepts zero and all powers of two
	return ((~number + 1) | ((number << 1) - 1)) == ~0UL;
}
#endif


static void *
heap_memalign(heap_allocator *heap, size_t alignment, size_t size,
	bool *shouldGrow)
{
	TRACE(("memalign(alignment = %lu, size = %lu)\n", alignment, size));

#if DEBUG
	if (!is_valid_alignment(alignment))
		panic("memalign() with an alignment which is not a power of 2\n");
#endif

	mutex_lock(&heap->lock);

	// ToDo: that code "aligns" the buffer because the bins are always
	//	aligned on their bin size
	if (size < alignment)
		size = alignment;

	uint32 binIndex;
	for (binIndex = 0; binIndex < heap->bin_count; binIndex++) {
		if (size <= heap->bins[binIndex].element_size)
			break;
	}

	void *address = heap_raw_alloc(heap, size, binIndex);

	TRACE(("memalign(): asked to allocate %lu bytes, returning pointer %p\n", size, address));

	if (heap->next == NULL) {
		// suggest growing if we are the last heap and we have
		// less than three free pages left
		*shouldGrow = (heap->free_pages == NULL
			|| heap->free_pages->next == NULL
			|| heap->free_pages->next->next == NULL);
	}

	mutex_unlock(&heap->lock);
	if (address == NULL)
		return address;

#if PARANOID_KFREE
	// make sure 0xdeadbeef is cleared if we do not overwrite the memory
	// and the user does not clear it
	((uint32 *)address)[1] = 0xcccccccc;
#endif

#if PARANOID_KMALLOC
	memset(address, 0xcc, size);
#endif

	return address;
}


static status_t
heap_free(heap_allocator *heap, void *address)
{
	if (address == NULL)
		return B_OK;

	if ((addr_t)address < heap->base
		|| (addr_t)address >= heap->base + heap->size) {
		// this address does not belong to us
		return B_ENTRY_NOT_FOUND;
	}

	mutex_lock(&heap->lock);

	TRACE(("free(): asked to free at ptr = %p\n", address));

	heap_page *page = &heap->page_table[((addr_t)address - heap->base) / B_PAGE_SIZE];

	TRACE(("free(): page %p: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count));

	if (page->bin_index > heap->bin_count) {
		panic("free(): page %p: invalid bin_index %d\n", page, page->bin_index);
		mutex_unlock(&heap->lock);
		return B_ERROR;
	}

	if (page->bin_index < heap->bin_count) {
		// small allocation
		heap_bin *bin = &heap->bins[page->bin_index];
		if (((addr_t)address - heap->base - page->index * B_PAGE_SIZE) % bin->element_size != 0) {
			panic("free(): passed invalid pointer %p supposed to be in bin for element size %ld\n", address, bin->element_size);
			mutex_unlock(&heap->lock);
			return B_ERROR;
		}

#if PARANOID_KFREE
		if (((uint32 *)address)[1] == 0xdeadbeef) {
			// This block looks like it was freed already, walk the free list
			// on this page to make sure this address doesn't exist.
			for (addr_t *temp = page->free_list; temp != NULL; temp = (addr_t *)*temp) {
				if (temp == address) {
					panic("free(): address %p already exists in page free list\n", address);
					mutex_unlock(&heap->lock);
					return B_ERROR;
				}
			}
		}

		uint32 *dead = (uint32 *)address;
		if (bin->element_size % 4 != 0) {
			panic("free(): didn't expect a bin element size that is not a multiple of 4\n");
			mutex_unlock(&heap->lock);
			return B_ERROR;
		}

		// the first 4 bytes are overwritten with the next free list pointer later
		for (uint32 i = 1; i < bin->element_size / sizeof(uint32); i++)
			dead[i] = 0xdeadbeef;
#endif

		// add the address to the page free list
		*(addr_t *)address = (addr_t)page->free_list;
		page->free_list = (addr_t *)address;
		page->free_count++;

		if (page->free_count == bin->max_free_count) {
			// we are now empty, remove the page from the bin list
			heap_unlink_page(page, &bin->page_list);
			page->in_use = 0;
			heap_link_page(page, &heap->free_pages);
		} else if (page->free_count == 1) {
			// we need to add ourselfs to the page list of the bin
			heap_link_page(page, &bin->page_list);
		} else {
			// we might need to move back in the free pages list
			if (page->next && page->next->free_count < page->free_count) {
				// move ourselfs so the list stays ordered
				heap_page *insert = page->next;
				while (insert->next
					&& insert->next->free_count < page->free_count)
					insert = insert->next;

				heap_unlink_page(page, &bin->page_list);

				page->prev = insert;
				page->next = insert->next;
				if (page->next)
					page->next->prev = page;
				insert->next = page;
			}
		}
	} else {
		// large allocation, just return the pages to the page free list
		uint32 allocationID = page->allocation_id;
		uint32 maxPages = heap->page_count - page->index;
		for (uint32 i = 0; i < maxPages; i++) {
			// loop until we find the end of this allocation
			if (!page[i].in_use || page[i].bin_index != heap->bin_count
				|| page[i].allocation_id != allocationID)
				break;

			// this page still belongs to the same allocation
			page[i].in_use = 0;
			page[i].allocation_id = 0;

			// return it to the free list
			heap_link_page(&page[i], &heap->free_pages);
		}
	}

	mutex_unlock(&heap->lock);
	return B_OK;
}


static status_t
heap_realloc(heap_allocator *heap, void *address, void **newAddress,
	size_t newSize)
{
	if ((addr_t)address < heap->base
		|| (addr_t)address >= heap->base + heap->size) {
		// this address does not belong to us
		return B_ENTRY_NOT_FOUND;
	}

	mutex_lock(&heap->lock);
	TRACE(("realloc(address = %p, newSize = %lu)\n", address, newSize));

	heap_page *page = &heap->page_table[((addr_t)address - heap->base) / B_PAGE_SIZE];
	if (page->bin_index > heap->bin_count) {
		panic("realloc(): page %p: invalid bin_index %d\n", page, page->bin_index);
		mutex_unlock(&heap->lock);
		return B_ERROR;
	}

	// find out the size of the old allocation first
	size_t minSize = 0;
	size_t maxSize = 0;
	if (page->bin_index < heap->bin_count) {
		// this was a small allocation
		heap_bin *bin = &heap->bins[page->bin_index];
		maxSize = bin->element_size;
		if (page->bin_index > 0)
			minSize = heap->bins[page->bin_index - 1].element_size + 1;
	} else {
		// this was a large allocation
		uint32 allocationID = page->allocation_id;
		uint32 maxPages = heap->page_count - page->index;
		maxSize = B_PAGE_SIZE;
		for (uint32 i = 1; i < maxPages; i++) {
			if (!page[i].in_use || page[i].bin_index != heap->bin_count
				|| page[i].allocation_id != allocationID)
				break;

			minSize += B_PAGE_SIZE;
			maxSize += B_PAGE_SIZE;
		}
	}

	mutex_unlock(&heap->lock);

	// does the new allocation simply fit in the old allocation?
	if (newSize > minSize && newSize <= maxSize) {
		*newAddress = address;
		return B_OK;
	}

	// if not, allocate a new chunk of memory
	*newAddress = malloc(newSize);
	if (*newAddress == NULL) {
		// we tried but it didn't work out, but still the operation is done
		return B_OK;
	}

	// copy the old data and free the old allocation
	memcpy(*newAddress, address, min_c(maxSize, newSize));
	free(address);
	return B_OK;
}


//	#pragma mark -


static int32
heap_grow_thread(void *)
{
	heap_allocator *heap = sHeapList;
	while (true) {
		// wait for a request to grow the heap list
		if (acquire_sem(sHeapGrowSem) < B_OK)
			continue;

		// find the last heap
		while (heap->next)
			heap = heap->next;

		if (sLastGrowRequest != heap) {
			// we have already grown since the latest request, just ignore
			continue;
		}

		TRACE(("heap_grower: kernel heap will run out of memory soon, allocating new one\n"));
		void *heapAddress = NULL;
		area_id heapArea = create_area("additional heap", &heapAddress,
			B_ANY_KERNEL_BLOCK_ADDRESS, HEAP_GROW_SIZE, B_FULL_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (heapArea < B_OK) {
			panic("heap_grower: couldn't allocate additional heap area\n");
			continue;
		}

		heap_allocator *newHeap = heap_attach((addr_t)heapAddress,
			HEAP_GROW_SIZE, true);
		if (newHeap == NULL) {
			panic("heap_grower: could not attach additional heap!\n");
			delete_area(heapArea);
			continue;
		}

#if PARANOID_VALIDATION
		heap_validate_heap(newHeap);
#endif
		heap->next = newHeap;
		TRACE(("heap_grower: new heap linked in\n"));

		// notify anyone waiting for this request
		release_sem_etc(sHeapGrownNotify, -1, B_RELEASE_ALL);
	}

	return 0;
}


status_t
heap_init(addr_t base, size_t size)
{
	sHeapList = heap_attach(base, size, false);

	// set up some debug commands
	add_debugger_command("heap", &dump_heap_list, "dump stats about the kernel heap(s)");
	return B_OK;
}


status_t
heap_init_post_sem()
{
	// create the lock for the initial heap
	if (mutex_init(&sHeapList->lock, "heap_mutex") < B_OK) {
		panic("heap_init_post_sem(): error creating heap mutex\n");
		return B_ERROR;
	}

	sHeapGrowSem = create_sem(0, "heap_grow_sem");
	if (sHeapGrowSem < 0) {
		panic("heap_init_post_sem(): failed to create heap grow sem\n");
		return B_ERROR;
	}

	sHeapGrownNotify = create_sem(0, "heap_grown_notify");
	if (sHeapGrownNotify < 0) {
		panic("heap_init_post_sem(): failed to create heap grown notify sem\n");
		return B_ERROR;
	}

	return B_OK;
}


status_t
heap_init_post_thread()
{
	thread_id thread = spawn_kernel_thread(heap_grow_thread, "heap grower",
		B_URGENT_PRIORITY, NULL);
	if (thread < 0) {
		panic("heap_init_post_thread(): cannot create heap grow thread\n");
		return B_ERROR;
	}

	send_signal_etc(thread, SIGCONT, B_DO_NOT_RESCHEDULE);
	return B_OK;
}


//	#pragma mark -


void *
memalign(size_t alignment, size_t size)
{
	if (!kernel_startup && !are_interrupts_enabled()) {
		panic("memalign(): called with interrupts disabled\n");
		return NULL;
	}

	if (size > (HEAP_GROW_SIZE * 3) / 4) {
		// don't even attempt such a huge allocation
		panic("heap: huge allocation of %lu bytes asked!\n", size);
		return NULL;
	}

	heap_allocator *heap = sHeapList;
	while (heap) {
		bool shouldGrow = false;
		void *result = heap_memalign(heap, alignment, size, &shouldGrow);
		if (heap->next == NULL && (shouldGrow || result == NULL)) {
			// the last heap will or has run out of memory, notify the grower
			sLastGrowRequest = heap;
			if (result == NULL) {
				// urgent request, do the request and wait for at max 250ms
				release_sem(sHeapGrowSem);
				acquire_sem_etc(sHeapGrownNotify, 1, B_RELATIVE_TIMEOUT, 250000);
			} else {
				// not so urgent, just notify the grower
				release_sem_etc(sHeapGrowSem, 1, B_DO_NOT_RESCHEDULE);
			}
		}

		if (result == NULL) {
			heap = heap->next;
			continue;
		}

#if PARANOID_VALIDATION
		heap_validate_heap(heap);
#endif

		return result;
	}

	panic("heap: kernel heap has run out of memory\n");
	return NULL;
}


void *
malloc(size_t size)
{
	return memalign(0, size);
}


void
free(void *address)
{
	if (!kernel_startup && !are_interrupts_enabled()) {
		panic("free(): called with interrupts disabled\n");
		return;
	}

	heap_allocator *heap = sHeapList;
	while (heap) {
		if (heap_free(heap, address) == B_OK) {
#if PARANOID_VALIDATION
			heap_validate_heap(heap);
#endif
			return;
		}

		heap = heap->next;
	}

	panic("free(): free failed for address %p\n", address);
}


void *
realloc(void *address, size_t newSize)
{
	if (!kernel_startup && !are_interrupts_enabled()) {
		panic("realloc(): called with interrupts disabled\n");
		return NULL;
	}

	if (address == NULL)
		return malloc(newSize);

	if (newSize == 0) {
		free(address);
		return NULL;
	}

	heap_allocator *heap = sHeapList;
	while (heap) {
		void *newAddress = NULL;
		if (heap_realloc(heap, address, &newAddress, newSize) == B_OK) {
#if PARANOID_VALIDATION
			heap_validate_heap(heap);
#endif
			return newAddress;
		}

		heap = heap->next;
	}

	panic("realloc(): failed to realloc address %p to size %lu\n", address, newSize);
	return NULL;
}


void *
calloc(size_t numElements, size_t size)
{
	void *address = memalign(0, numElements * size);
	if (address != NULL)
		memset(address, 0, numElements * size);

	return address;
}
