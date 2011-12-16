/*
 * Copyright 2008-2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <malloc.h>
#include <malloc_debug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <sys/mman.h>

#include <errno_private.h>
#include <locks.h>
#include <syscalls.h>

#include <Debug.h>
#include <OS.h>


//#define TRACE_HEAP
#ifdef TRACE_HEAP
#	define INFO(x) debug_printf x
#else
#	define INFO(x) ;
#endif

#undef ASSERT
#define ASSERT(x)	if (!(x)) panic("assert failed: %s", #x);


static bool sDebuggerCalls = true;
static bool sReuseMemory = true;
static bool sParanoidValidation = false;
static thread_id sWallCheckThread = -1;
static bool sStopWallChecking = false;
static bool sUseGuardPage = false;


void
panic(const char *format, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	if (sDebuggerCalls)
		debugger(buffer);
	else
		debug_printf(buffer);
}


#define ROUNDUP(x, y)		(((x) + (y) - 1) / (y) * (y))

#define HEAP_INITIAL_SIZE			1 * 1024 * 1024
#define HEAP_GROW_SIZE				2 * 1024 * 1024
#define HEAP_AREA_USE_THRESHOLD		1 * 1024 * 1024

typedef struct heap_leak_check_info_s {
	size_t		size;
	thread_id	thread;
} heap_leak_check_info;

typedef struct heap_page_s heap_page;

typedef struct heap_area_s {
	area_id			area;

	addr_t			base;
	size_t			size;

	uint32			page_count;
	uint32			free_page_count;

	heap_page *		free_pages;
	heap_page *		page_table;

	heap_area_s *	prev;
	heap_area_s *	next;
	heap_area_s *	all_next;
} heap_area;

typedef struct heap_page_s {
	heap_area *		area;
	uint16			index;
	uint16			bin_index : 5;
	uint16			free_count : 10;
	uint16			in_use : 1;
	heap_page_s *	next;
	heap_page_s *	prev;
	union {
		uint16			empty_index;
		uint16			allocation_id; // used for bin == bin_count allocations
	};
	addr_t *		free_list;
} heap_page;

typedef struct heap_bin_s {
	mutex		lock;
	uint32		element_size;
	uint16		max_free_count;
	heap_page *	page_list; // sorted so that the desired page is always first
} heap_bin;

typedef struct heap_allocator_s {
	rw_lock		area_lock;
	mutex		page_lock;

	const char *name;
	uint32		bin_count;
	uint32		page_size;

	uint32		total_pages;
	uint32		total_free_pages;
	uint32		empty_areas;

	heap_bin *	bins;
	heap_area *	areas; // sorted so that the desired area is always first
	heap_area *	all_areas; // all areas including full ones
} heap_allocator;

static const uint32 kAreaAllocationMagic = 'AAMG';
typedef struct area_allocation_info_s {
	area_id		area;
	void *		base;
	uint32		magic;
	size_t		size;
	thread_id	thread;
	size_t		allocation_size;
	size_t		allocation_alignment;
	void *		allocation_base;
} area_allocation_info;

typedef struct heap_class_s {
	const char *name;
	uint32		initial_percentage;
	size_t		max_allocation_size;
	size_t		page_size;
	size_t		min_bin_size;
	size_t		bin_alignment;
	uint32		min_count_per_page;
	size_t		max_waste_per_page;
} heap_class;

// Heap class configuration
#define HEAP_CLASS_COUNT 3
static const heap_class sHeapClasses[HEAP_CLASS_COUNT] = {
	{
		"small",					/* name */
		50,							/* initial percentage */
		B_PAGE_SIZE / 8,			/* max allocation size */
		B_PAGE_SIZE,				/* page size */
		8,							/* min bin size */
		4,							/* bin alignment */
		8,							/* min count per page */
		16							/* max waste per page */
	},
	{
		"medium",					/* name */
		30,							/* initial percentage */
		B_PAGE_SIZE * 2,			/* max allocation size */
		B_PAGE_SIZE * 8,			/* page size */
		B_PAGE_SIZE / 8,			/* min bin size */
		32,							/* bin alignment */
		4,							/* min count per page */
		64							/* max waste per page */
	},
	{
		"large",					/* name */
		20,							/* initial percentage */
		HEAP_AREA_USE_THRESHOLD,	/* max allocation size */
		B_PAGE_SIZE * 16,			/* page size */
		B_PAGE_SIZE * 2,			/* min bin size */
		128,						/* bin alignment */
		1,							/* min count per page */
		256							/* max waste per page */
	}
};

static heap_allocator *sHeaps[HEAP_CLASS_COUNT];


// #pragma mark - Debug functions


static void
dump_page(heap_page *page)
{
	uint32 count = 0;
	for (addr_t *temp = page->free_list; temp != NULL; temp = (addr_t *)*temp)
		count++;

	printf("\t\tpage %p: bin_index: %u; free_count: %u; empty_index: %u; "
		"free_list %p (%lu entr%s)\n", page, page->bin_index, page->free_count,
		page->empty_index, page->free_list, count, count == 1 ? "y" : "ies");
}


static void
dump_bin(heap_bin *bin)
{
	printf("\telement_size: %lu; max_free_count: %u; page_list %p;\n",
		bin->element_size, bin->max_free_count, bin->page_list);

	for (heap_page *temp = bin->page_list; temp != NULL; temp = temp->next)
		dump_page(temp);
}


static void
dump_bin_list(heap_allocator *heap)
{
	for (uint32 i = 0; i < heap->bin_count; i++)
		dump_bin(&heap->bins[i]);
	printf("\n");
}


static void
dump_allocator_areas(heap_allocator *heap)
{
	heap_area *area = heap->all_areas;
	while (area) {
		printf("\tarea %p: area: %ld; base: 0x%08lx; size: %lu; page_count: "
			"%lu; free_pages: %p (%lu entr%s)\n", area, area->area, area->base,
			area->size, area->page_count, area->free_pages,
			area->free_page_count, area->free_page_count == 1 ? "y" : "ies");
		area = area->all_next;
	}

	printf("\n");
}


static void
dump_allocator(heap_allocator *heap, bool areas, bool bins)
{
	printf("allocator %p: name: %s; page_size: %lu; bin_count: %lu; pages: "
		"%lu; free_pages: %lu; empty_areas: %lu\n", heap, heap->name,
		heap->page_size, heap->bin_count, heap->total_pages,
		heap->total_free_pages, heap->empty_areas);

	if (areas)
		dump_allocator_areas(heap);
	if (bins)
		dump_bin_list(heap);
}


static void
dump_allocations(bool statsOnly, thread_id thread)
{
	size_t totalSize = 0;
	uint32 totalCount = 0;
	for (uint32 classIndex = 0; classIndex < HEAP_CLASS_COUNT; classIndex++) {
		heap_allocator *heap = sHeaps[classIndex];

		// go through all the pages in all the areas
		heap_area *area = heap->all_areas;
		while (area) {
			heap_leak_check_info *info = NULL;
			for (uint32 i = 0; i < area->page_count; i++) {
				heap_page *page = &area->page_table[i];
				if (!page->in_use)
					continue;

				addr_t base = area->base + i * heap->page_size;
				if (page->bin_index < heap->bin_count) {
					// page is used by a small allocation bin
					uint32 elementCount = page->empty_index;
					size_t elementSize
						= heap->bins[page->bin_index].element_size;
					for (uint32 j = 0; j < elementCount;
							j++, base += elementSize) {
						// walk the free list to see if this element is in use
						bool elementInUse = true;
						for (addr_t *temp = page->free_list; temp != NULL;
								temp = (addr_t *)*temp) {
							if ((addr_t)temp == base) {
								elementInUse = false;
								break;
							}
						}

						if (!elementInUse)
							continue;

						info = (heap_leak_check_info *)(base + elementSize
							- sizeof(heap_leak_check_info));

						if (thread == -1 || info->thread == thread) {
							// interesting...
							if (!statsOnly) {
								printf("thread: % 6ld; address: 0x%08lx;"
									" size: %lu bytes\n", info->thread,
									base, info->size);
							}

							totalSize += info->size;
							totalCount++;
						}
					}
				} else {
					// page is used by a big allocation, find the page count
					uint32 pageCount = 1;
					while (i + pageCount < area->page_count
						&& area->page_table[i + pageCount].in_use
						&& area->page_table[i + pageCount].bin_index
							== heap->bin_count
						&& area->page_table[i + pageCount].allocation_id
							== page->allocation_id)
						pageCount++;

					info = (heap_leak_check_info *)(base + pageCount
						* heap->page_size - sizeof(heap_leak_check_info));

					if (thread == -1 || info->thread == thread) {
						// interesting...
						if (!statsOnly) {
							printf("thread: % 6ld; address: 0x%08lx;"
								" size: %lu bytes\n", info->thread,
								base, info->size);
						}

						totalSize += info->size;
						totalCount++;
					}

					// skip the allocated pages
					i += pageCount - 1;
				}
			}

			area = area->all_next;
		}
	}

	printf("total allocations: %lu; total bytes: %lu\n", totalCount, totalSize);
}


static void
heap_validate_walls()
{
	for (uint32 classIndex = 0; classIndex < HEAP_CLASS_COUNT; classIndex++) {
		heap_allocator *heap = sHeaps[classIndex];
		ReadLocker areaReadLocker(heap->area_lock);
		for (uint32 i = 0; i < heap->bin_count; i++)
			mutex_lock(&heap->bins[i].lock);
		MutexLocker pageLocker(heap->page_lock);

		// go through all the pages in all the areas
		heap_area *area = heap->all_areas;
		while (area) {
			heap_leak_check_info *info = NULL;
			for (uint32 i = 0; i < area->page_count; i++) {
				heap_page *page = &area->page_table[i];
				if (!page->in_use)
					continue;

				addr_t base = area->base + i * heap->page_size;
				if (page->bin_index < heap->bin_count) {
					// page is used by a small allocation bin
					uint32 elementCount = page->empty_index;
					size_t elementSize
						= heap->bins[page->bin_index].element_size;
					for (uint32 j = 0; j < elementCount;
							j++, base += elementSize) {
						// walk the free list to see if this element is in use
						bool elementInUse = true;
						for (addr_t *temp = page->free_list; temp != NULL;
								temp = (addr_t *)*temp) {
							if ((addr_t)temp == base) {
								elementInUse = false;
								break;
							}
						}

						if (!elementInUse)
							continue;

						info = (heap_leak_check_info *)(base + elementSize
							- sizeof(heap_leak_check_info));
						if (info->size > elementSize - sizeof(addr_t)
								- sizeof(heap_leak_check_info)) {
							panic("leak check info has invalid size %lu for"
								" element size %lu\n", info->size, elementSize);
							continue;
						}

						addr_t wallValue;
						addr_t wallAddress = base + info->size;
						memcpy(&wallValue, (void *)wallAddress, sizeof(addr_t));
						if (wallValue != wallAddress) {
							panic("someone wrote beyond small allocation at"
								" 0x%08lx; size: %lu bytes; allocated by %ld;"
								" value: 0x%08lx\n", base, info->size,
								info->thread, wallValue);
						}
					}
				} else {
					// page is used by a big allocation, find the page count
					uint32 pageCount = 1;
					while (i + pageCount < area->page_count
						&& area->page_table[i + pageCount].in_use
						&& area->page_table[i + pageCount].bin_index
							== heap->bin_count
						&& area->page_table[i + pageCount].allocation_id
							== page->allocation_id)
						pageCount++;

					info = (heap_leak_check_info *)(base + pageCount
						* heap->page_size - sizeof(heap_leak_check_info));

					if (info->size > pageCount * heap->page_size
							- sizeof(addr_t) - sizeof(heap_leak_check_info)) {
						panic("leak check info has invalid size %lu for"
							" page count %lu (%lu bytes)\n", info->size,
							pageCount, pageCount * heap->page_size);
						continue;
					}

					addr_t wallValue;
					addr_t wallAddress = base + info->size;
					memcpy(&wallValue, (void *)wallAddress, sizeof(addr_t));
					if (wallValue != wallAddress) {
						panic("someone wrote beyond big allocation at 0x%08lx;"
							" size: %lu bytes; allocated by %ld;"
							" value: 0x%08lx\n", base, info->size,
							info->thread, wallValue);
					}

					// skip the allocated pages
					i += pageCount - 1;
				}
			}

			area = area->all_next;
		}

		pageLocker.Unlock();
		for (uint32 i = 0; i < heap->bin_count; i++)
			mutex_unlock(&heap->bins[i].lock);
	}
}


static void
heap_validate_heap(heap_allocator *heap)
{
	ReadLocker areaReadLocker(heap->area_lock);
	for (uint32 i = 0; i < heap->bin_count; i++)
		mutex_lock(&heap->bins[i].lock);
	MutexLocker pageLocker(heap->page_lock);

	uint32 totalPageCount = 0;
	uint32 totalFreePageCount = 0;
	heap_area *area = heap->all_areas;
	while (area != NULL) {
		// validate the free pages list
		uint32 freePageCount = 0;
		heap_page *lastPage = NULL;
		heap_page *page = area->free_pages;
		while (page) {
			if ((addr_t)page < (addr_t)&area->page_table[0]
				|| (addr_t)page >= (addr_t)&area->page_table[area->page_count])
				panic("free page is not part of the page table\n");

			if (page->index >= area->page_count)
				panic("free page has invalid index\n");

			if ((addr_t)&area->page_table[page->index] != (addr_t)page)
				panic("free page index does not lead to target page\n");

			if (page->prev != lastPage)
				panic("free page entry has invalid prev link\n");

			if (page->in_use)
				panic("free page marked as in use\n");

			lastPage = page;
			page = page->next;
			freePageCount++;
		}

		totalPageCount += freePageCount;
		totalFreePageCount += freePageCount;
		if (area->free_page_count != freePageCount) {
			panic("free page count %ld doesn't match free page list %ld\n",
				area->free_page_count, freePageCount);
		}

		// validate the page table
		uint32 usedPageCount = 0;
		for (uint32 i = 0; i < area->page_count; i++) {
			if (area->page_table[i].in_use)
				usedPageCount++;
		}

		totalPageCount += usedPageCount;
		if (freePageCount + usedPageCount != area->page_count) {
			panic("free pages and used pages do not add up (%lu + %lu != %lu)\n",
				freePageCount, usedPageCount, area->page_count);
		}

		area = area->all_next;
	}

	// validate the areas
	area = heap->areas;
	heap_area *lastArea = NULL;
	uint32 lastFreeCount = 0;
	while (area != NULL) {
		if (area->free_page_count < lastFreeCount)
			panic("size ordering of area list broken\n");

		if (area->prev != lastArea)
			panic("area list entry has invalid prev link\n");

		lastArea = area;
		lastFreeCount = area->free_page_count;
		area = area->next;
	}

	lastArea = NULL;
	area = heap->all_areas;
	while (area != NULL) {
		if (lastArea != NULL && lastArea->base < area->base)
			panic("base ordering of all_areas list broken\n");

		lastArea = area;
		area = area->all_next;
	}

	// validate the bins
	for (uint32 i = 0; i < heap->bin_count; i++) {
		heap_bin *bin = &heap->bins[i];
		heap_page *lastPage = NULL;
		heap_page *page = bin->page_list;
		lastFreeCount = 0;
		while (page) {
			area = heap->all_areas;
			while (area) {
				if (area == page->area)
					break;
				area = area->all_next;
			}

			if (area == NULL) {
				panic("page area not present in area list\n");
				page = page->next;
				continue;
			}

			if ((addr_t)page < (addr_t)&area->page_table[0]
				|| (addr_t)page >= (addr_t)&area->page_table[area->page_count])
				panic("used page is not part of the page table\n");

			if (page->index >= area->page_count)
				panic("used page has invalid index %lu\n", page->index);

			if ((addr_t)&area->page_table[page->index] != (addr_t)page) {
				panic("used page index does not lead to target page"
					" (%lu vs. %lu)\n", (addr_t)&area->page_table[page->index],
					page);
			}

			if (page->prev != lastPage) {
				panic("used page entry has invalid prev link (%p vs %p bin "
					"%lu)\n", page->prev, lastPage, i);
			}

			if (!page->in_use)
				panic("used page %p marked as not in use\n", page);

			if (page->bin_index != i) {
				panic("used page with bin index %u in page list of bin %lu\n",
					page->bin_index, i);
			}

			if (page->free_count < lastFreeCount)
				panic("ordering of bin page list broken\n");

			// validate the free list
			uint32 freeSlotsCount = 0;
			addr_t *element = page->free_list;
			addr_t pageBase = area->base + page->index * heap->page_size;
			while (element) {
				if ((addr_t)element < pageBase
					|| (addr_t)element >= pageBase + heap->page_size)
					panic("free list entry out of page range %p\n", element);

				if (((addr_t)element - pageBase) % bin->element_size != 0)
					panic("free list entry not on a element boundary\n");

				element = (addr_t *)*element;
				freeSlotsCount++;
			}

			uint32 slotCount = bin->max_free_count;
			if (page->empty_index > slotCount) {
				panic("empty index beyond slot count (%u with %lu slots)\n",
					page->empty_index, slotCount);
			}

			freeSlotsCount += (slotCount - page->empty_index);
			if (freeSlotsCount > slotCount)
				panic("more free slots than fit into the page\n");

			lastPage = page;
			lastFreeCount = page->free_count;
			page = page->next;
		}
	}

	pageLocker.Unlock();
	for (uint32 i = 0; i < heap->bin_count; i++)
		mutex_unlock(&heap->bins[i].lock);
}


// #pragma mark - Heap functions


static void
heap_add_area(heap_allocator *heap, area_id areaID, addr_t base, size_t size)
{
	heap_area *area = (heap_area *)base;
	area->area = areaID;

	base += sizeof(heap_area);
	size -= sizeof(heap_area);

	uint32 pageCount = size / heap->page_size;
	size_t pageTableSize = pageCount * sizeof(heap_page);
	area->page_table = (heap_page *)base;
	base += pageTableSize;
	size -= pageTableSize;

	// the rest is now actually usable memory (rounded to the next page)
	area->base = ROUNDUP(base, B_PAGE_SIZE);
	area->size = size & ~(B_PAGE_SIZE - 1);

	// now we know the real page count
	pageCount = area->size / heap->page_size;
	area->page_count = pageCount;

	// zero out the page table and fill in page indexes
	memset((void *)area->page_table, 0, pageTableSize);
	for (uint32 i = 0; i < pageCount; i++) {
		area->page_table[i].area = area;
		area->page_table[i].index = i;
	}

	// add all pages up into the free pages list
	for (uint32 i = 1; i < pageCount; i++) {
		area->page_table[i - 1].next = &area->page_table[i];
		area->page_table[i].prev = &area->page_table[i - 1];
	}
	area->free_pages = &area->page_table[0];
	area->free_page_count = pageCount;
	area->page_table[0].prev = NULL;
	area->next = NULL;

	WriteLocker areaWriteLocker(heap->area_lock);
	MutexLocker pageLocker(heap->page_lock);
	if (heap->areas == NULL) {
		// it's the only (empty) area in that heap
		area->prev = NULL;
		heap->areas = area;
	} else {
		// link in this area as the last one as it is completely empty
		heap_area *lastArea = heap->areas;
		while (lastArea->next != NULL)
			lastArea = lastArea->next;

		lastArea->next = area;
		area->prev = lastArea;
	}

	// insert this area in the all_areas list so it stays ordered by base
	if (heap->all_areas == NULL || heap->all_areas->base < area->base) {
		area->all_next = heap->all_areas;
		heap->all_areas = area;
	} else {
		heap_area *insert = heap->all_areas;
		while (insert->all_next && insert->all_next->base > area->base)
			insert = insert->all_next;

		area->all_next = insert->all_next;
		insert->all_next = area;
	}

	heap->total_pages += area->page_count;
	heap->total_free_pages += area->free_page_count;

	if (areaID >= 0) {
		// this later on deletable area is yet empty - the empty count will be
		// decremented as soon as this area is used for the first time
		heap->empty_areas++;
	}

	pageLocker.Unlock();
	areaWriteLocker.Unlock();
}


static status_t
heap_remove_area(heap_allocator *heap, heap_area *area)
{
	if (area->free_page_count != area->page_count) {
		panic("tried removing heap area that has still pages in use");
		return B_ERROR;
	}

	if (area->prev == NULL && area->next == NULL) {
		panic("tried removing the last non-full heap area");
		return B_ERROR;
	}

	if (heap->areas == area)
		heap->areas = area->next;
	if (area->prev != NULL)
		area->prev->next = area->next;
	if (area->next != NULL)
		area->next->prev = area->prev;

	if (heap->all_areas == area)
		heap->all_areas = area->all_next;
	else {
		heap_area *previous = heap->all_areas;
		while (previous) {
			if (previous->all_next == area) {
				previous->all_next = area->all_next;
				break;
			}

			previous = previous->all_next;
		}

		if (previous == NULL)
			panic("removing heap area that is not in all list");
	}

	heap->total_pages -= area->page_count;
	heap->total_free_pages -= area->free_page_count;
	return B_OK;
}


heap_allocator *
heap_create_allocator(const char *name, addr_t base, size_t size,
	const heap_class *heapClass)
{
	heap_allocator *heap = (heap_allocator *)base;
	base += sizeof(heap_allocator);
	size -= sizeof(heap_allocator);

	heap->name = name;
	heap->page_size = heapClass->page_size;
	heap->total_pages = heap->total_free_pages = heap->empty_areas = 0;
	heap->areas = heap->all_areas = NULL;
	heap->bins = (heap_bin *)base;

	heap->bin_count = 0;
	size_t binSize = 0, lastSize = 0;
	uint32 count = heap->page_size / heapClass->min_bin_size;
	for (; count >= heapClass->min_count_per_page; count--, lastSize = binSize) {
		if (heap->bin_count >= 32)
			panic("heap configuration invalid - max bin count reached\n");

		binSize = (heap->page_size / count) & ~(heapClass->bin_alignment - 1);
		if (binSize == lastSize)
			continue;
		if (heap->page_size - count * binSize > heapClass->max_waste_per_page)
			continue;

		heap_bin *bin = &heap->bins[heap->bin_count];
		mutex_init(&bin->lock, "heap bin lock");
		bin->element_size = binSize;
		bin->max_free_count = heap->page_size / binSize;
		bin->page_list = NULL;
		heap->bin_count++;
	};

	base += heap->bin_count * sizeof(heap_bin);
	size -= heap->bin_count * sizeof(heap_bin);

	rw_lock_init(&heap->area_lock, "heap area lock");
	mutex_init(&heap->page_lock, "heap page lock");

	heap_add_area(heap, -1, base, size);
	return heap;
}


static inline void
heap_free_pages_added(heap_allocator *heap, heap_area *area, uint32 pageCount)
{
	area->free_page_count += pageCount;
	heap->total_free_pages += pageCount;

	if (area->free_page_count == pageCount) {
		// we need to add ourselfs to the area list of the heap
		area->prev = NULL;
		area->next = heap->areas;
		if (area->next)
			area->next->prev = area;
		heap->areas = area;
	} else {
		// we might need to move back in the area list
		if (area->next && area->next->free_page_count < area->free_page_count) {
			// move ourselfs so the list stays ordered
			heap_area *insert = area->next;
			while (insert->next
				&& insert->next->free_page_count < area->free_page_count)
				insert = insert->next;

			if (area->prev)
				area->prev->next = area->next;
			if (area->next)
				area->next->prev = area->prev;
			if (heap->areas == area)
				heap->areas = area->next;

			area->prev = insert;
			area->next = insert->next;
			if (area->next)
				area->next->prev = area;
			insert->next = area;
		}
	}

	if (area->free_page_count == area->page_count && area->area >= 0)
		heap->empty_areas++;
}


static inline void
heap_free_pages_removed(heap_allocator *heap, heap_area *area, uint32 pageCount)
{
	if (area->free_page_count == area->page_count && area->area >= 0) {
		// this area was completely empty
		heap->empty_areas--;
	}

	area->free_page_count -= pageCount;
	heap->total_free_pages -= pageCount;

	if (area->free_page_count == 0) {
		// the area is now full so we remove it from the area list
		if (area->prev)
			area->prev->next = area->next;
		if (area->next)
			area->next->prev = area->prev;
		if (heap->areas == area)
			heap->areas = area->next;
		area->next = area->prev = NULL;
	} else {
		// we might need to move forward in the area list
		if (area->prev && area->prev->free_page_count > area->free_page_count) {
			// move ourselfs so the list stays ordered
			heap_area *insert = area->prev;
			while (insert->prev
				&& insert->prev->free_page_count > area->free_page_count)
				insert = insert->prev;

			if (area->prev)
				area->prev->next = area->next;
			if (area->next)
				area->next->prev = area->prev;

			area->prev = insert->prev;
			area->next = insert;
			if (area->prev)
				area->prev->next = area;
			if (heap->areas == insert)
				heap->areas = area;
			insert->prev = area;
		}
	}
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


static heap_page *
heap_allocate_contiguous_pages(heap_allocator *heap, uint32 pageCount,
	size_t alignment)
{
	heap_area *area = heap->areas;
	while (area) {
		if (area->free_page_count < pageCount) {
			area = area->next;
			continue;
		}

		uint32 step = 1;
		uint32 firstValid = 0;
		const uint32 lastValid = area->page_count - pageCount + 1;

		if (alignment > heap->page_size) {
			firstValid = (ROUNDUP(area->base, alignment) - area->base)
				/ heap->page_size;
			step = alignment / heap->page_size;
		}

		int32 first = -1;
		for (uint32 i = firstValid; i < lastValid; i += step) {
			if (area->page_table[i].in_use)
				continue;

			first = i;

			for (uint32 j = 1; j < pageCount; j++) {
				if (area->page_table[i + j].in_use) {
					first = -1;
					i += j / step * step;
					break;
				}
			}

			if (first >= 0)
				break;
		}

		if (first < 0) {
			area = area->next;
			continue;
		}

		for (uint32 i = first; i < first + pageCount; i++) {
			heap_page *page = &area->page_table[i];
			page->in_use = 1;
			page->bin_index = heap->bin_count;

			heap_unlink_page(page, &area->free_pages);

			page->next = page->prev = NULL;
			page->free_list = NULL;
			page->allocation_id = (uint16)first;
		}

		heap_free_pages_removed(heap, area, pageCount);
		return &area->page_table[first];
	}

	return NULL;
}


static void
heap_add_leak_check_info(addr_t address, size_t allocated, size_t size)
{
	size -= sizeof(addr_t) + sizeof(heap_leak_check_info);
	heap_leak_check_info *info = (heap_leak_check_info *)(address + allocated
		- sizeof(heap_leak_check_info));
	info->thread = find_thread(NULL);
	info->size = size;

	addr_t wallAddress = address + size;
	memcpy((void *)wallAddress, &wallAddress, sizeof(addr_t));
}


static void *
heap_raw_alloc(heap_allocator *heap, size_t size, size_t alignment)
{
	INFO(("heap %p: allocate %lu bytes from raw pages with alignment %lu\n",
		heap, size, alignment));

	uint32 pageCount = (size + heap->page_size - 1) / heap->page_size;

	MutexLocker pageLocker(heap->page_lock);
	heap_page *firstPage = heap_allocate_contiguous_pages(heap, pageCount,
		alignment);
	if (firstPage == NULL) {
		INFO(("heap %p: found no contiguous pages to allocate %ld bytes\n",
			heap, size));
		return NULL;
	}

	addr_t address = firstPage->area->base + firstPage->index * heap->page_size;
	heap_add_leak_check_info(address, pageCount * heap->page_size, size);
	return (void *)address;
}


static void *
heap_allocate_from_bin(heap_allocator *heap, uint32 binIndex, size_t size)
{
	heap_bin *bin = &heap->bins[binIndex];
	INFO(("heap %p: allocate %lu bytes from bin %lu with element_size %lu\n",
		heap, size, binIndex, bin->element_size));

	MutexLocker binLocker(bin->lock);
	heap_page *page = bin->page_list;
	if (page == NULL) {
		MutexLocker pageLocker(heap->page_lock);
		heap_area *area = heap->areas;
		if (area == NULL) {
			INFO(("heap %p: no free pages to allocate %lu bytes\n", heap,
				size));
			return NULL;
		}

		// by design there are only areas in the list that still have
		// free pages available
		page = area->free_pages;
		area->free_pages = page->next;
		if (page->next)
			page->next->prev = NULL;

		heap_free_pages_removed(heap, area, 1);

		if (page->in_use)
			panic("got an in use page from the free pages list\n");
		page->in_use = 1;

		pageLocker.Unlock();

		page->bin_index = binIndex;
		page->free_count = bin->max_free_count;
		page->empty_index = 0;
		page->free_list = NULL;
		page->next = page->prev = NULL;
		bin->page_list = page;
	}

	// we have a page where we have a free slot
	void *address = NULL;
	if (page->free_list) {
		// there's a previously freed entry we can use
		address = page->free_list;
		page->free_list = (addr_t *)*page->free_list;
	} else {
		// the page hasn't been fully allocated so use the next empty_index
		address = (void *)(page->area->base + page->index * heap->page_size
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

	heap_add_leak_check_info((addr_t)address, bin->element_size, size);
	return address;
}


static bool
is_valid_alignment(size_t number)
{
	// this cryptic line accepts zero and all powers of two
	return ((~number + 1) | ((number << 1) - 1)) == ~0UL;
}


void *
heap_memalign(heap_allocator *heap, size_t alignment, size_t size)
{
	INFO(("memalign(alignment = %lu, size = %lu)\n", alignment, size));

	if (!is_valid_alignment(alignment)) {
		panic("memalign() with an alignment which is not a power of 2 (%lu)\n",
			alignment);
	}

	size += sizeof(addr_t) + sizeof(heap_leak_check_info);

	void *address = NULL;
	if (alignment < B_PAGE_SIZE) {
		if (alignment != 0) {
			// TODO: The alignment is done by ensuring that the element size
			// of the target bin is aligned with the requested alignment. This
			// has the problem that it wastes space because a better (smaller)
			// bin could possibly be selected. We should pick the best bin and
			// check if there is an aligned block in the free list or if a new
			// (page aligned) page has to be allocated anyway.
			size = ROUNDUP(size, alignment);
			for (uint32 i = 0; i < heap->bin_count; i++) {
				if (size <= heap->bins[i].element_size
					&& is_valid_alignment(heap->bins[i].element_size)) {
					address = heap_allocate_from_bin(heap, i, size);
					break;
				}
			}
		} else {
			for (uint32 i = 0; i < heap->bin_count; i++) {
				if (size <= heap->bins[i].element_size) {
					address = heap_allocate_from_bin(heap, i, size);
					break;
				}
			}
		}
	}

	if (address == NULL)
		address = heap_raw_alloc(heap, size, alignment);

	size -= sizeof(addr_t) + sizeof(heap_leak_check_info);

	INFO(("memalign(): asked to allocate %lu bytes, returning pointer %p\n",
		size, address));

	if (address == NULL)
		return address;

	memset(address, 0xcc, size);

	// make sure 0xdeadbeef is cleared if we do not overwrite the memory
	// and the user does not clear it
	if (((uint32 *)address)[1] == 0xdeadbeef)
		((uint32 *)address)[1] = 0xcccccccc;

	return address;
}


status_t
heap_free(heap_allocator *heap, void *address)
{
	if (address == NULL)
		return B_OK;

	ReadLocker areaReadLocker(heap->area_lock);
	heap_area *area = heap->all_areas;
	while (area) {
		// since the all_areas list is ordered by base with the biggest
		// base at the top, we need only find the first area with a base
		// smaller than our address to become our only candidate for freeing
		if (area->base <= (addr_t)address) {
			if ((addr_t)address >= area->base + area->size) {
				// none of the other areas can contain the address as the list
				// is ordered
				return B_ENTRY_NOT_FOUND;
			}

			// this area contains the allocation, we're done searching
			break;
		}

		area = area->all_next;
	}

	if (area == NULL) {
		// this address does not belong to us
		return B_ENTRY_NOT_FOUND;
	}

	INFO(("free(): asked to free pointer %p\n", address));

	heap_page *page = &area->page_table[((addr_t)address - area->base)
		/ heap->page_size];

	INFO(("free(): page %p: bin_index %d, free_count %d\n", page,
		page->bin_index, page->free_count));

	if (page->bin_index > heap->bin_count) {
		panic("free(): page %p: invalid bin_index %d\n", page,
			page->bin_index);
		return B_ERROR;
	}

	addr_t pageBase = area->base + page->index * heap->page_size;
	if (page->bin_index < heap->bin_count) {
		// small allocation
		heap_bin *bin = &heap->bins[page->bin_index];
		if (((addr_t)address - pageBase) % bin->element_size != 0) {
			panic("free(): address %p does not fall on allocation boundary"
				" for page base %p and element size %lu\n", address,
				(void *)pageBase, bin->element_size);
			return B_ERROR;
		}

		MutexLocker binLocker(bin->lock);

		if (((uint32 *)address)[1] == 0xdeadbeef) {
			// This block looks like it was freed already, walk the free list
			// on this page to make sure this address doesn't exist.
			for (addr_t *temp = page->free_list; temp != NULL;
					temp = (addr_t *)*temp) {
				if (temp == address) {
					panic("free(): address %p already exists in page free"
						" list (double free)\n", address);
					return B_ERROR;
				}
			}
		}

		heap_leak_check_info *info = (heap_leak_check_info *)((addr_t)address
			+ bin->element_size - sizeof(heap_leak_check_info));
		if (info->size > bin->element_size - sizeof(addr_t)
				- sizeof(heap_leak_check_info)) {
			panic("leak check info has invalid size %lu for element size %lu,"
				" probably memory has been overwritten past allocation size\n",
				info->size, bin->element_size);
		}

		addr_t wallValue;
		addr_t wallAddress = (addr_t)address + info->size;
		memcpy(&wallValue, (void *)wallAddress, sizeof(addr_t));
		if (wallValue != wallAddress) {
			panic("someone wrote beyond small allocation at %p;"
				" size: %lu bytes; allocated by %ld; value: 0x%08lx\n",
				address, info->size, info->thread, wallValue);
		}

		// the first 4 bytes are overwritten with the next free list pointer
		// later
		uint32 *dead = (uint32 *)address;
		for (uint32 i = 0; i < bin->element_size / sizeof(uint32); i++)
			dead[i] = 0xdeadbeef;

		if (sReuseMemory) {
			// add the address to the page free list
			*(addr_t *)address = (addr_t)page->free_list;
			page->free_list = (addr_t *)address;
			page->free_count++;

			if (page->free_count == bin->max_free_count) {
				// we are now empty, remove the page from the bin list
				MutexLocker pageLocker(heap->page_lock);
				heap_unlink_page(page, &bin->page_list);
				page->in_use = 0;
				heap_link_page(page, &area->free_pages);
				heap_free_pages_added(heap, area, 1);
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
		}
	} else {
		if ((addr_t)address != pageBase) {
			panic("free(): large allocation address %p not on page base %p\n",
				address, (void *)pageBase);
			return B_ERROR;
		}

		// large allocation, just return the pages to the page free list
		uint32 allocationID = page->allocation_id;
		uint32 maxPages = area->page_count - page->index;
		uint32 pageCount = 0;

		MutexLocker pageLocker(heap->page_lock);
		for (uint32 i = 0; i < maxPages; i++) {
			// loop until we find the end of this allocation
			if (!page[i].in_use || page[i].bin_index != heap->bin_count
				|| page[i].allocation_id != allocationID)
				break;

			// this page still belongs to the same allocation
			if (sReuseMemory) {
				page[i].in_use = 0;
				page[i].allocation_id = 0;

				// return it to the free list
				heap_link_page(&page[i], &area->free_pages);
			}

			pageCount++;
		}

		size_t allocationSize = pageCount * heap->page_size;
		heap_leak_check_info *info = (heap_leak_check_info *)((addr_t)address
			+ allocationSize - sizeof(heap_leak_check_info));
		if (info->size > allocationSize - sizeof(addr_t)
				- sizeof(heap_leak_check_info)) {
			panic("leak check info has invalid size %lu for allocation of %lu,"
				" probably memory has been overwritten past allocation size\n",
				info->size, allocationSize);
		}

		addr_t wallValue;
		addr_t wallAddress = (addr_t)address + info->size;
		memcpy(&wallValue, (void *)wallAddress, sizeof(addr_t));
		if (wallValue != wallAddress) {
			panic("someone wrote beyond big allocation at %p;"
				" size: %lu bytes; allocated by %ld; value: 0x%08lx\n",
				address, info->size, info->thread, wallValue);
		}

		uint32 *dead = (uint32 *)address;
		for (uint32 i = 0; i < allocationSize / sizeof(uint32); i++)
			dead[i] = 0xdeadbeef;

		if (sReuseMemory)
			heap_free_pages_added(heap, area, pageCount);
	}

	areaReadLocker.Unlock();

	if (heap->empty_areas > 1) {
		WriteLocker areaWriteLocker(heap->area_lock);
		MutexLocker pageLocker(heap->page_lock);

		area = heap->areas;
		while (area != NULL && heap->empty_areas > 1) {
			heap_area *next = area->next;
			if (area->area >= 0
				&& area->free_page_count == area->page_count
				&& heap_remove_area(heap, area) == B_OK) {
				delete_area(area->area);
				heap->empty_areas--;
			}

			area = next;
		}
	}

	return B_OK;
}


static status_t
heap_realloc(heap_allocator *heap, void *address, void **newAddress,
	size_t newSize)
{
	ReadLocker areaReadLocker(heap->area_lock);
	heap_area *area = heap->all_areas;
	while (area) {
		// since the all_areas list is ordered by base with the biggest
		// base at the top, we need only find the first area with a base
		// smaller than our address to become our only candidate for
		// reallocating
		if (area->base <= (addr_t)address) {
			if ((addr_t)address >= area->base + area->size) {
				// none of the other areas can contain the address as the list
				// is ordered
				return B_ENTRY_NOT_FOUND;
			}

			// this area contains the allocation, we're done searching
			break;
		}

		area = area->all_next;
	}

	if (area == NULL) {
		// this address does not belong to us
		return B_ENTRY_NOT_FOUND;
	}

	INFO(("realloc(address = %p, newSize = %lu)\n", address, newSize));

	heap_page *page = &area->page_table[((addr_t)address - area->base)
		/ heap->page_size];
	if (page->bin_index > heap->bin_count) {
		panic("realloc(): page %p: invalid bin_index %d\n", page,
			page->bin_index);
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
		uint32 maxPages = area->page_count - page->index;
		maxSize = heap->page_size;

		MutexLocker pageLocker(heap->page_lock);
		for (uint32 i = 1; i < maxPages; i++) {
			if (!page[i].in_use || page[i].bin_index != heap->bin_count
				|| page[i].allocation_id != allocationID)
				break;

			minSize += heap->page_size;
			maxSize += heap->page_size;
		}
	}

	newSize += sizeof(addr_t) + sizeof(heap_leak_check_info);

	// does the new allocation simply fit in the old allocation?
	if (newSize > minSize && newSize <= maxSize) {
		// update the size info (the info is at the end so stays where it is)
		heap_leak_check_info *info = (heap_leak_check_info *)((addr_t)address
			+ maxSize - sizeof(heap_leak_check_info));
		newSize -= sizeof(addr_t) + sizeof(heap_leak_check_info);
		*newAddress = address;

		MutexLocker pageLocker(heap->page_lock);
		info->size = newSize;
		addr_t wallAddress = (addr_t)address + newSize;
		memcpy((void *)wallAddress, &wallAddress, sizeof(addr_t));
		return B_OK;
	}

	areaReadLocker.Unlock();

	// new leak check info will be created with the malloc below
	newSize -= sizeof(addr_t) + sizeof(heap_leak_check_info);

	// if not, allocate a new chunk of memory
	*newAddress = memalign(0, newSize);
	if (*newAddress == NULL) {
		// we tried but it didn't work out, but still the operation is done
		return B_OK;
	}

	// copy the old data and free the old allocation
	memcpy(*newAddress, address, min_c(maxSize, newSize));
	heap_free(heap, address);
	return B_OK;
}


inline uint32
heap_class_for(size_t size)
{
	// take the extra info size into account
	size += sizeof(addr_t) + sizeof(heap_leak_check_info);

	uint32 index = 0;
	for (; index < HEAP_CLASS_COUNT - 1; index++) {
		if (size <= sHeapClasses[index].max_allocation_size)
			return index;
	}

	return index;
}


static status_t
heap_get_allocation_info(heap_allocator *heap, void *address, size_t *size,
	thread_id *thread)
{
	ReadLocker areaReadLocker(heap->area_lock);
	heap_area *area = heap->all_areas;
	while (area) {
		// since the all_areas list is ordered by base with the biggest
		// base at the top, we need only find the first area with a base
		// smaller than our address to become our only candidate for freeing
		if (area->base <= (addr_t)address) {
			if ((addr_t)address >= area->base + area->size) {
				// none of the other areas can contain the address as the list
				// is ordered
				return B_ENTRY_NOT_FOUND;
			}

			// this area contains the allocation, we're done searching
			break;
		}

		area = area->all_next;
	}

	if (area == NULL) {
		// this address does not belong to us
		return B_ENTRY_NOT_FOUND;
	}

	heap_page *page = &area->page_table[((addr_t)address - area->base)
		/ heap->page_size];

	if (page->bin_index > heap->bin_count) {
		panic("get_allocation_info(): page %p: invalid bin_index %d\n", page,
			page->bin_index);
		return B_ERROR;
	}

	heap_leak_check_info *info = NULL;
	addr_t pageBase = area->base + page->index * heap->page_size;
	if (page->bin_index < heap->bin_count) {
		// small allocation
		heap_bin *bin = &heap->bins[page->bin_index];
		if (((addr_t)address - pageBase) % bin->element_size != 0) {
			panic("get_allocation_info(): address %p does not fall on"
				" allocation boundary for page base %p and element size %lu\n",
				address, (void *)pageBase, bin->element_size);
			return B_ERROR;
		}

		MutexLocker binLocker(bin->lock);

		info = (heap_leak_check_info *)((addr_t)address + bin->element_size
			- sizeof(heap_leak_check_info));
		if (info->size > bin->element_size - sizeof(addr_t)
				- sizeof(heap_leak_check_info)) {
			panic("leak check info has invalid size %lu for element size %lu,"
				" probably memory has been overwritten past allocation size\n",
				info->size, bin->element_size);
			return B_ERROR;
		}
	} else {
		if ((addr_t)address != pageBase) {
			panic("get_allocation_info(): large allocation address %p not on"
				" page base %p\n", address, (void *)pageBase);
			return B_ERROR;
		}

		uint32 allocationID = page->allocation_id;
		uint32 maxPages = area->page_count - page->index;
		uint32 pageCount = 0;

		MutexLocker pageLocker(heap->page_lock);
		for (uint32 i = 0; i < maxPages; i++) {
			// loop until we find the end of this allocation
			if (!page[i].in_use || page[i].bin_index != heap->bin_count
				|| page[i].allocation_id != allocationID)
				break;

			pageCount++;
		}

		size_t allocationSize = pageCount * heap->page_size;
		info = (heap_leak_check_info *)((addr_t)address + allocationSize
			- sizeof(heap_leak_check_info));
		if (info->size > allocationSize - sizeof(addr_t)
				- sizeof(heap_leak_check_info)) {
			panic("leak check info has invalid size %lu for allocation of %lu,"
				" probably memory has been overwritten past allocation size\n",
				info->size, allocationSize);
			return B_ERROR;
		}
	}

	if (size != NULL)
		*size = info->size;
	if (thread != NULL)
		*thread = info->thread;

	return B_OK;
}


//	#pragma mark -


static status_t
heap_create_new_heap_area(heap_allocator *heap, const char *name, size_t size)
{
	void *address = NULL;
	area_id heapArea = create_area(name, &address, B_ANY_ADDRESS, size,
		B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (heapArea < B_OK) {
		INFO(("heap: couldn't allocate heap area \"%s\"\n", name));
		return heapArea;
	}

	heap_add_area(heap, heapArea, (addr_t)address, size);
	if (sParanoidValidation)
		heap_validate_heap(heap);

	return B_OK;
}


static int32
heap_wall_checker(void *data)
{
	int msInterval = (int32)data;
	while (!sStopWallChecking) {
		heap_validate_walls();
		snooze(msInterval * 1000);
	}

	return 0;
}


//	#pragma mark - Heap Debug API


extern "C" status_t
heap_debug_start_wall_checking(int msInterval)
{
	if (sWallCheckThread < 0) {
		sWallCheckThread = spawn_thread(heap_wall_checker, "heap wall checker",
			B_LOW_PRIORITY, (void *)msInterval);
	}

	if (sWallCheckThread < 0)
		return sWallCheckThread;

	sStopWallChecking = false;
	return resume_thread(sWallCheckThread);
}


extern "C" status_t
heap_debug_stop_wall_checking()
{
	int32 result;
	sStopWallChecking = true;
	return wait_for_thread(sWallCheckThread, &result);
}


extern "C" void
heap_debug_set_paranoid_validation(bool enabled)
{
	sParanoidValidation = enabled;
}


extern "C" void
heap_debug_set_memory_reuse(bool enabled)
{
	sReuseMemory = enabled;
}


extern "C" void
heap_debug_set_debugger_calls(bool enabled)
{
	sDebuggerCalls = enabled;
}


extern "C" void
heap_debug_validate_heaps()
{
	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++)
		heap_validate_heap(sHeaps[i]);
}


extern "C" void
heap_debug_validate_walls()
{
	heap_validate_walls();
}


extern "C" void
heap_debug_dump_allocations(bool statsOnly, thread_id thread)
{
	dump_allocations(statsOnly, thread);
}


extern "C" void
heap_debug_dump_heaps(bool dumpAreas, bool dumpBins)
{
	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++)
		dump_allocator(sHeaps[i], dumpAreas, dumpBins);
}


extern "C" void *
heap_debug_malloc_with_guard_page(size_t size)
{
	size_t areaSize = ROUNDUP(size + sizeof(area_allocation_info) + B_PAGE_SIZE,
		B_PAGE_SIZE);
	if (areaSize < size) {
		// the size overflowed
		return NULL;
	}

	void *address = NULL;
	area_id allocationArea = create_area("guarded area", &address,
		B_ANY_ADDRESS, areaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (allocationArea < B_OK) {
		panic("heap: failed to create area for guarded allocation of %lu"
			" bytes\n", size);
		return NULL;
	}

	if (mprotect((void *)((addr_t)address + areaSize - B_PAGE_SIZE),
			B_PAGE_SIZE, PROT_NONE) != 0) {
		panic("heap: failed to protect guard page: %s\n", strerror(errno));
		delete_area(allocationArea);
		return NULL;
	}

	area_allocation_info *info = (area_allocation_info *)address;
	info->magic = kAreaAllocationMagic;
	info->area = allocationArea;
	info->base = address;
	info->size = areaSize;
	info->thread = find_thread(NULL);
	info->allocation_size = size;
	info->allocation_alignment = 0;

	// the address is calculated so that the end of the allocation
	// is at the end of the usable space of the requested area
	address = (void *)((addr_t)address + areaSize - B_PAGE_SIZE - size);

	INFO(("heap: allocated area %ld for guarded allocation of %lu bytes\n",
		allocationArea, size));

	info->allocation_base = address;

	memset(address, 0xcc, size);
	return address;
}


extern "C" status_t
heap_debug_get_allocation_info(void *address, size_t *size,
	thread_id *thread)
{
	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		heap_allocator *heap = sHeaps[i];
		if (heap_get_allocation_info(heap, address, size, thread) == B_OK)
			return B_OK;
	}

	// or maybe it was a huge allocation using an area
	area_info areaInfo;
	area_id area = area_for(address);
	if (area >= B_OK && get_area_info(area, &areaInfo) == B_OK) {
		area_allocation_info *info = (area_allocation_info *)areaInfo.address;

		// just make extra sure it was allocated by us
		if (info->magic == kAreaAllocationMagic && info->area == area
			&& info->size == areaInfo.size && info->base == areaInfo.address
			&& info->allocation_size < areaInfo.size) {
			if (size)
				*size = info->allocation_size;
			if (thread)
				*thread = info->thread;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


//	#pragma mark - Init


extern "C" status_t
__init_heap(void)
{
	// This will locate the heap base at 384 MB and reserve the next 1152 MB
	// for it. They may get reclaimed by other areas, though, but the maximum
	// size of the heap is guaranteed until the space is really needed.
	void *heapBase = (void *)0x18000000;
	status_t status = _kern_reserve_address_range((addr_t *)&heapBase,
		B_EXACT_ADDRESS, 0x48000000);

	area_id heapArea = create_area("heap", (void **)&heapBase,
		status == B_OK ? B_EXACT_ADDRESS : B_BASE_ADDRESS,
		HEAP_INITIAL_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (heapArea < B_OK)
		return heapArea;

	addr_t base = (addr_t)heapBase;
	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		size_t partSize = HEAP_INITIAL_SIZE
			* sHeapClasses[i].initial_percentage / 100;
		sHeaps[i] = heap_create_allocator(sHeapClasses[i].name, base, partSize,
			&sHeapClasses[i]);
		base += partSize;
	}

	return B_OK;
}


extern "C" void
__init_heap_post_env(void)
{
	const char *mode = getenv("MALLOC_DEBUG");
	if (mode != NULL) {
		if (strchr(mode, 'p'))
			heap_debug_set_paranoid_validation(true);
		if (strchr(mode, 'w'))
			heap_debug_start_wall_checking(500);
		else if (strchr(mode, 'W'))
			heap_debug_start_wall_checking(100);
		if (strchr(mode, 'g'))
			sUseGuardPage = true;
		if (strchr(mode, 'r'))
			heap_debug_set_memory_reuse(false);
	}
}


//	#pragma mark - Public API


extern "C" void *
sbrk_hook(long)
{
	debug_printf("sbrk not supported on malloc debug\n");
	panic("sbrk not supported on malloc debug\n");
	return NULL;
}


void *
memalign(size_t alignment, size_t size)
{
	size_t alignedSize = size + sizeof(addr_t) + sizeof(heap_leak_check_info);
	if (alignment != 0 && alignment < B_PAGE_SIZE)
		alignedSize = ROUNDUP(alignedSize, alignment);

	if (alignedSize >= HEAP_AREA_USE_THRESHOLD) {
		// don't even attempt such a huge allocation - use areas instead
		size_t areaSize = ROUNDUP(size + sizeof(area_allocation_info)
			+ alignment, B_PAGE_SIZE);
		if (areaSize < size) {
			// the size overflowed
			return NULL;
		}

		void *address = NULL;
		area_id allocationArea = create_area("memalign area", &address,
			B_ANY_ADDRESS, areaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (allocationArea < B_OK) {
			panic("heap: failed to create area for huge allocation of %lu"
				" bytes\n", size);
			return NULL;
		}

		area_allocation_info *info = (area_allocation_info *)address;
		info->magic = kAreaAllocationMagic;
		info->area = allocationArea;
		info->base = address;
		info->size = areaSize;
		info->thread = find_thread(NULL);
		info->allocation_size = size;
		info->allocation_alignment = alignment;

		address = (void *)((addr_t)address + sizeof(area_allocation_info));
		if (alignment != 0) {
			address = (void *)ROUNDUP((addr_t)address, alignment);
			ASSERT((addr_t)address % alignment == 0);
			ASSERT((addr_t)address + size - 1 < (addr_t)info + areaSize - 1);
		}

		INFO(("heap: allocated area %ld for huge allocation of %lu bytes\n",
			allocationArea, size));

		info->allocation_base = address;

		memset(address, 0xcc, size);
		return address;
	}

	uint32 heapClass = alignment < B_PAGE_SIZE
		? heap_class_for(alignedSize) : 0;

	heap_allocator *heap = sHeaps[heapClass];
	void *result = heap_memalign(heap, alignment, size);
	if (result == NULL) {
		// add new area and try again
		heap_create_new_heap_area(heap, "additional heap area", HEAP_GROW_SIZE);
		result = heap_memalign(heap, alignment, size);
	}

	if (sParanoidValidation)
		heap_validate_heap(heap);

	if (result == NULL) {
		panic("heap: heap has run out of memory trying to allocate %lu bytes\n",
			size);
	}

	if (alignment != 0)
		ASSERT((addr_t)result % alignment == 0);

	return result;
}


void *
malloc(size_t size)
{
	if (sUseGuardPage)
		return heap_debug_malloc_with_guard_page(size);

	return memalign(0, size);
}


void
free(void *address)
{
	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		heap_allocator *heap = sHeaps[i];
		if (heap_free(heap, address) == B_OK) {
			if (sParanoidValidation)
				heap_validate_heap(heap);
			return;
		}
	}

	// or maybe it was a huge allocation using an area
	area_info areaInfo;
	area_id area = area_for(address);
	if (area >= B_OK && get_area_info(area, &areaInfo) == B_OK) {
		area_allocation_info *info = (area_allocation_info *)areaInfo.address;

		// just make extra sure it was allocated by us
		if (info->magic == kAreaAllocationMagic && info->area == area
			&& info->size == areaInfo.size && info->base == areaInfo.address
			&& info->allocation_size < areaInfo.size) {
			delete_area(area);
			INFO(("free(): freed huge allocation by deleting area %ld\n",
				area));
			return;
		}
	}

	panic("free(): free failed for address %p\n", address);
}


void *
realloc(void *address, size_t newSize)
{
	if (address == NULL)
		return memalign(0, newSize);

	if (newSize == 0) {
		free(address);
		return NULL;
	}

	void *newAddress = NULL;
	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		heap_allocator *heap = sHeaps[i];
		if (heap_realloc(heap, address, &newAddress, newSize) == B_OK) {
			if (sParanoidValidation)
				heap_validate_heap(heap);
			return newAddress;
		}
	}

	// or maybe it was a huge allocation using an area
	area_info areaInfo;
	area_id area = area_for(address);
	if (area >= B_OK && get_area_info(area, &areaInfo) == B_OK) {
		area_allocation_info *info = (area_allocation_info *)areaInfo.address;

		// just make extra sure it was allocated by us
		if (info->magic == kAreaAllocationMagic && info->area == area
			&& info->size == areaInfo.size && info->base == areaInfo.address
			&& info->allocation_size < areaInfo.size) {
			if (sUseGuardPage) {
				size_t available = info->size - B_PAGE_SIZE
					- sizeof(area_allocation_info);

				if (available >= newSize) {
					// there is enough room available for the newSize
					newAddress = (void*)((addr_t)info->allocation_base
						+ info->allocation_size - newSize);
					INFO(("realloc(): new size %ld fits in old area %ld with "
						"%ld available -> new address: %p\n", newSize, area,
						available, newAddress));
					memmove(newAddress, info->allocation_base,
						min_c(newSize, info->allocation_size));
					info->allocation_base = newAddress;
					info->allocation_size = newSize;
					return newAddress;
				}
			} else {
				size_t available = info->size - ((addr_t)info->allocation_base
					- (addr_t)info->base);

				if (available >= newSize) {
					// there is enough room available for the newSize
					INFO(("realloc(): new size %ld fits in old area %ld with "
						"%ld available\n", newSize, area, available));
					info->allocation_size = newSize;
					return address;
				}
			}

			// have to allocate/copy/free - TODO maybe resize the area instead?
			newAddress = malloc(newSize);
			if (newAddress == NULL) {
				panic("realloc(): failed to allocate new block of %ld"
					" bytes\n", newSize);
				return NULL;
			}

			memcpy(newAddress, address, min_c(newSize, info->allocation_size));
			delete_area(area);
			INFO(("realloc(): allocated new block %p for size %ld and deleted "
				"old area %ld\n", newAddress, newSize, area));
			return newAddress;
		}
	}

	panic("realloc(): failed to realloc address %p to size %lu\n", address,
		newSize);
	return NULL;
}


void *
calloc(size_t numElements, size_t size)
{
	void *address = malloc(numElements * size);
	if (address != NULL)
		memset(address, 0, numElements * size);

	return address;
}


extern "C" void *
valloc(size_t size)
{
	return memalign(B_PAGE_SIZE, size);
}


extern "C" int
posix_memalign(void **pointer, size_t alignment, size_t size)
{
	if (!is_valid_alignment(alignment))
		return EINVAL;

	*pointer = memalign(alignment, size);
	if (*pointer == NULL)
		return ENOMEM;

	return 0;
}
