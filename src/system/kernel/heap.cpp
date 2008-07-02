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
#include <arch/debug.h>
#include <debug.h>
#include <elf.h>
#include <heap.h>
#include <int.h>
#include <kernel.h>
#include <lock.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>
#include <team.h>
#include <thread.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
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
// store size, thread and team info at the end of each allocation block
#define KERNEL_HEAP_LEAK_CHECK 0

#if KERNEL_HEAP_LEAK_CHECK
typedef struct heap_leak_check_info_s {
	addr_t		caller;
	size_t		size;
	thread_id	thread;
	team_id		team;
} heap_leak_check_info;

struct caller_info {
	addr_t		caller;
	uint32		count;
	uint32		size;
};

static const int32 kCallerInfoTableSize = 1024;
static caller_info sCallerInfoTable[kCallerInfoTableSize];
static int32 sCallerInfoCount = 0;
#endif

typedef struct heap_page_s {
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
	uint32		element_size;
	uint16		max_free_count;
	heap_page *	page_list; // sorted so that the desired page is always first
} heap_bin;

typedef struct heap_allocator_s {
	addr_t		base;
	size_t		size;
	mutex		lock;

	uint32		bin_count;
	uint32		page_count;
	uint32		page_size;
	uint32		free_page_count;
	heap_page *	free_pages;

	heap_bin *	bins;
	heap_page *	page_table;

	heap_allocator_s *	next;
} heap_allocator;

typedef struct heap_class_s {
	const char *name;
	uint32		initial_percentage;
	size_t		max_allocation_size;
	size_t		page_size;
	size_t		bin_sizes[20];
} heap_class;

static const uint32 kAreaAllocationMagic = 'AAMG';
typedef struct area_allocation_info_s {
	area_id		area;
	void *		base;
	uint32		magic;
	size_t		size;
	size_t		allocation_size;
	size_t		allocation_alignment;
	void *		allocation_base;
} area_allocation_info;

struct DeferredFreeListEntry : DoublyLinkedListLinkImpl<DeferredFreeListEntry> {
};
typedef DoublyLinkedList<DeferredFreeListEntry> DeferredFreeList;

// Heap class configuration
#define HEAP_CLASS_COUNT 3
static heap_class sHeapClasses[HEAP_CLASS_COUNT] = {
	{ "small", 50, B_PAGE_SIZE, B_PAGE_SIZE,
		{ 8, 12, 16, 24, 32, 48, 64, 96, 128, 160, 192, 256, 384, 512, 1024,
			2048, 4096, 0 }
	},
	{ "large", 30, B_PAGE_SIZE * 32, B_PAGE_SIZE * 32,
		{ 4096, 5120, 6144, 7168, 8192, 12288, 16384, 24576, 32768, 65536,
			131072, 0 }
	},
	{ "huge", 20, HEAP_AREA_USE_THRESHOLD, B_PAGE_SIZE * 64,
		{ 131072, 262144, 0 }
	}
};

static heap_allocator *sHeapList[HEAP_CLASS_COUNT];
static heap_allocator *sLastGrowRequest[HEAP_CLASS_COUNT];
static heap_allocator *sGrowHeapList = NULL;
static thread_id sHeapGrowThread = -1;
static sem_id sHeapGrowSem = -1;
static sem_id sHeapGrownNotify = -1;
static bool sAddGrowHeap = false;

static DeferredFreeList sDeferredFreeList;
static spinlock sDeferredFreeListLock;



// #pragma mark - Tracing

#if KERNEL_HEAP_TRACING
namespace KernelHeapTracing {

class Allocate : public AbstractTraceEntry {
	public:
		Allocate(addr_t address, size_t size)
			:	fAddress(address),
				fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput &out)
		{
			out.Print("heap allocate: 0x%08lx (%lu bytes)", fAddress, fSize);
		}

	private:
		addr_t	fAddress;
		size_t	fSize;
};


class Reallocate : public AbstractTraceEntry {
	public:
		Reallocate(addr_t oldAddress, addr_t newAddress, size_t newSize)
			:	fOldAddress(oldAddress),
				fNewAddress(newAddress),
				fNewSize(newSize)
		{
			Initialized();
		};

		virtual void AddDump(TraceOutput &out)
		{
			out.Print("heap reallocate: 0x%08lx -> 0x%08lx (%lu bytes)",
				fOldAddress, fNewAddress, fNewSize);
		}

	private:
		addr_t	fOldAddress;
		addr_t	fNewAddress;
		size_t	fNewSize;
};


class Free : public AbstractTraceEntry {
	public:
		Free(addr_t address)
			:	fAddress(address)
		{
			Initialized();
		};

		virtual void AddDump(TraceOutput &out)
		{
			out.Print("heap free: 0x%08lx", fAddress);
		}

	private:
		addr_t	fAddress;
};


} // namespace KernelHeapTracing

#	define T(x)	if (!kernel_startup) new(std::nothrow) KernelHeapTracing::x;
#else
#	define T(x)	;
#endif


// #pragma mark - Debug functions


#if KERNEL_HEAP_LEAK_CHECK
static addr_t
get_caller()
{
	// Find the first return address outside of the allocator code. Note, that
	// this makes certain assumptions about how the code for the functions
	// ends up in the kernel object.
	addr_t returnAddresses[5];
	int32 depth = arch_debug_get_stack_trace(returnAddresses, 5, 1, false);
	for (int32 i = 0; i < depth; i++) {
		if (returnAddresses[i] < (addr_t)&get_caller
			|| returnAddresses[i] > (addr_t)&malloc_referenced_release) {
			return returnAddresses[i];
		}
	}

	return 0;
}
#endif


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
	dprintf("allocator %p: base: 0x%08lx; size: %lu; bin_count: %lu; free_pages: %p (%lu entr%s)\n",
		heap, heap->base, heap->size, heap->bin_count, heap->free_pages,
		heap->free_page_count, heap->free_page_count == 1 ? "y" : "ies");

	for (uint32 i = 0; i < heap->bin_count; i++)
		dump_bin(&heap->bins[i]);

	dprintf("\n");
}


static int
dump_heap_list(int argc, char **argv)
{
	if (argc == 2) {
		if (strcmp(argv[1], "grow") == 0) {
			// only dump dedicated grow heap info
			dprintf("dedicated grow heap(s):\n");
			heap_allocator *heap = sGrowHeapList;
			while (heap) {
				dump_allocator(heap);
				heap = heap->next;
			}
		} else if (strcmp(argv[1], "stats") == 0) {
			for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
				uint32 heapCount = 0;
				heap_allocator *heap = sHeapList[i];
				while (heap) {
					heapCount++;
					heap = heap->next;
				}

				dprintf("current %s heap count: %ld\n", sHeapClasses[i].name,
					heapCount);
			}
		} else
			print_debugger_command_usage(argv[0]);

		return 0;
	}

	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		dprintf("dumping list of %s heaps\n", sHeapClasses[i].name);
		heap_allocator *heap = sHeapList[i];
		while (heap) {
			dump_allocator(heap);
			heap = heap->next;
		}
	}

	return 0;
}


#if KERNEL_HEAP_LEAK_CHECK

static int
dump_allocations(int argc, char **argv)
{
	team_id team = -1;
	thread_id thread = -1;
	addr_t caller = 0;
	bool statsOnly = false;
	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "team") == 0)
			team = strtoul(argv[++i], NULL, 0);
		else if (strcmp(argv[i], "thread") == 0)
			thread = strtoul(argv[++i], NULL, 0);
		else if (strcmp(argv[i], "caller") == 0)
			caller = strtoul(argv[++i], NULL, 0);
		else if (strcmp(argv[i], "stats") == 0)
			statsOnly = true;
		else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	size_t totalSize = 0;
	uint32 totalCount = 0;
	uint32 heapClassIndex = 0;
	heap_allocator *heap = sHeapList[0];
	while (heap) {
		// go through all the pages
		heap_leak_check_info *info = NULL;
		for (uint32 i = 0; i < heap->page_count; i++) {
			heap_page *page = &heap->page_table[i];
			if (!page->in_use)
				continue;

			addr_t base = heap->base + i * heap->page_size;
			if (page->bin_index < heap->bin_count) {
				// page is used by a small allocation bin
				uint32 elementCount = page->empty_index;
				size_t elementSize = heap->bins[page->bin_index].element_size;
				for (uint32 j = 0; j < elementCount; j++, base += elementSize) {
					// walk the free list to see if this element is in use
					bool elementInUse = true;
					for (addr_t *temp = page->free_list; temp != NULL; temp = (addr_t *)*temp) {
						if ((addr_t)temp == base) {
							elementInUse = false;
							break;
						}
					}

					if (!elementInUse)
						continue;

					info = (heap_leak_check_info *)(base + elementSize
						- sizeof(heap_leak_check_info));

					if ((team == -1 || info->team == team)
						&& (thread == -1 || info->thread == thread)
						&& (caller == 0 || info->caller == caller)) {
						// interesting...
						if (!statsOnly) {
							dprintf("team: % 6ld; thread: % 6ld; "
								"address: 0x%08lx; size: %lu bytes, "
								"caller: %#lx\n", info->team, info->thread,
								base, info->size, info->caller);
						}

						totalSize += info->size;
						totalCount++;
					}
				}
			} else {
				// page is used by a big allocation, find the page count
				uint32 pageCount = 1;
				while (i + pageCount < heap->page_count
					&& heap->page_table[i + pageCount].in_use
					&& heap->page_table[i + pageCount].bin_index == heap->bin_count
					&& heap->page_table[i + pageCount].allocation_id == page->allocation_id)
					pageCount++;

				info = (heap_leak_check_info *)(base + pageCount
					* heap->page_size - sizeof(heap_leak_check_info));

				if ((team == -1 || info->team == team)
					&& (thread == -1 || info->thread == thread)
					&& (caller == 0 || info->caller == caller)) {
					// interesting...
					if (!statsOnly) {
						dprintf("team: % 6ld; thread: % 6ld; address: 0x%08lx;"
							" size: %lu bytes, caller: %#lx\n", info->team,
							info->thread, base, info->size, info->caller);
					}

					totalSize += info->size;
					totalCount++;
				}

				// skip the allocated pages
				i += pageCount - 1;
			}
		}

		heap = heap->next;
		if (heap == NULL && ++heapClassIndex < HEAP_CLASS_COUNT)
			heap = sHeapList[heapClassIndex];
	}

	dprintf("total allocations: %lu; total bytes: %lu\n", totalCount, totalSize);
	return 0;
}


static caller_info*
get_caller_info(addr_t caller)
{
	// find the caller info
	for (int32 i = 0; i < sCallerInfoCount; i++) {
		if (caller == sCallerInfoTable[i].caller)
			return &sCallerInfoTable[i];
	}

	// not found, add a new entry, if there are free slots
	if (sCallerInfoCount >= kCallerInfoTableSize)
		return NULL;

	caller_info* info = &sCallerInfoTable[sCallerInfoCount++];
	info->caller = caller;
	info->count = 0;
	info->size = 0;

	return info;
}


static int
caller_info_compare_size(const void* _a, const void* _b)
{
	const caller_info* a = (const caller_info*)_a;
	const caller_info* b = (const caller_info*)_b;
	return (int)(b->size - a->size);
}


static int
caller_info_compare_count(const void* _a, const void* _b)
{
	const caller_info* a = (const caller_info*)_a;
	const caller_info* b = (const caller_info*)_b;
	return (int)(b->count - a->count);
}


static int
dump_allocations_per_caller(int argc, char **argv)
{
	bool sortBySize = true;
	
	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0) {
			sortBySize = false;
		} else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	sCallerInfoCount = 0;

	uint32 heapClassIndex = 0;
	heap_allocator *heap = sHeapList[0];
	while (heap) {
		// go through all the pages
		heap_leak_check_info *info = NULL;
		for (uint32 i = 0; i < heap->page_count; i++) {
			heap_page *page = &heap->page_table[i];
			if (!page->in_use)
				continue;

			addr_t base = heap->base + i * heap->page_size;
			if (page->bin_index < heap->bin_count) {
				// page is used by a small allocation bin
				uint32 elementCount = page->empty_index;
				size_t elementSize = heap->bins[page->bin_index].element_size;
				for (uint32 j = 0; j < elementCount; j++, base += elementSize) {
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

					caller_info* callerInfo = get_caller_info(info->caller);
					if (callerInfo == NULL) {
						kprintf("out of space for caller infos\n");
						return 0;
					}

					callerInfo->count++;
					callerInfo->size += info->size;
				}
			} else {
				// page is used by a big allocation, find the page count
				uint32 pageCount = 1;
				while (i + pageCount < heap->page_count
					&& heap->page_table[i + pageCount].in_use
					&& heap->page_table[i + pageCount].bin_index == heap->bin_count
					&& heap->page_table[i + pageCount].allocation_id == page->allocation_id)
					pageCount++;

				info = (heap_leak_check_info *)(base + pageCount
					* heap->page_size - sizeof(heap_leak_check_info));

				caller_info* callerInfo = get_caller_info(info->caller);
				if (callerInfo == NULL) {
					kprintf("out of space for caller infos\n");
					return 0;
				}

				callerInfo->count++;
				callerInfo->size += info->size;

				// skip the allocated pages
				i += pageCount - 1;
			}
		}

		heap = heap->next;
		if (heap == NULL && ++heapClassIndex < HEAP_CLASS_COUNT)
			heap = sHeapList[heapClassIndex];
	}

	// sort the array
	qsort(sCallerInfoTable, sCallerInfoCount, sizeof(caller_info),
		sortBySize ? &caller_info_compare_size : &caller_info_compare_count);

	kprintf("%ld different callers, sorted by %s...\n\n", sCallerInfoCount,
		sortBySize ? "size" : "count");

	kprintf("     count        size      caller\n");
	kprintf("----------------------------------\n");
	for (int32 i = 0; i < sCallerInfoCount; i++) {
		caller_info& info = sCallerInfoTable[i];
		kprintf("%10ld  %10ld  %#08lx", info.count, info.size, info.caller);

		const char* symbol;
		const char* imageName;
		bool exactMatch;
		addr_t baseAddress;

		if (elf_debug_lookup_symbol_address(info.caller, &baseAddress, &symbol,
				&imageName, &exactMatch) == B_OK) {
			kprintf("  %s + 0x%lx (%s)%s\n", symbol,
				info.caller - baseAddress, imageName,
				exactMatch ? "" : " (nearest)");
		} else
			kprintf("\n");
	}

	return 0;
}

#endif // KERNEL_HEAP_LEAK_CHECK


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

	if (heap->free_page_count != freePageCount)
		panic("free page count doesn't match free page list\n");

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
			addr_t pageBase = heap->base + page->index * heap->page_size;
			while (element) {
				if ((addr_t)element < pageBase
					|| (addr_t)element >= pageBase + heap->page_size)
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
#endif // PARANOID_VALIDATION


// #pragma mark - Heap functions


static heap_allocator *
heap_attach(addr_t base, size_t size, uint32 heapClass)
{
	heap_allocator *heap = (heap_allocator *)base;
	base += sizeof(heap_allocator);
	size -= sizeof(heap_allocator);

	heap->page_size = sHeapClasses[heapClass].page_size;
	heap->bins = (heap_bin *)base;

	heap->bin_count = 0;
	while (true) {
		size_t binSize = sHeapClasses[heapClass].bin_sizes[heap->bin_count];
		if (binSize == 0)
			break;

		heap_bin *bin = &heap->bins[heap->bin_count];
		bin->element_size = binSize;
		bin->max_free_count = heap->page_size / binSize;
		bin->page_list = NULL;
		heap->bin_count++;
	};

	base += heap->bin_count * sizeof(heap_bin);
	size -= heap->bin_count * sizeof(heap_bin);

	uint32 pageCount = size / heap->page_size;
	size_t pageTableSize = pageCount * sizeof(heap_page);
	heap->page_table = (heap_page *)base;
	base += pageTableSize;
	size -= pageTableSize;

	// the rest is now actually usable memory (rounded to the next page)
	heap->base = ROUNDUP(base, B_PAGE_SIZE);
	heap->size = (size / B_PAGE_SIZE) * B_PAGE_SIZE;

	// now we know the real page count
	pageCount = heap->size / heap->page_size;
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
	heap->free_page_count = pageCount;
	heap->page_table[0].prev = NULL;

	mutex_init(&heap->lock, "heap_mutex");

	heap->next = NULL;
	dprintf("heap_attach: %s heap attached to %p - usable range 0x%08lx - 0x%08lx\n",
		sHeapClasses[heapClass].name, heap, heap->base, heap->base + heap->size);
	return heap;
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
			address = (void *)(heap->base + page->index * heap->page_size
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

#if KERNEL_HEAP_LEAK_CHECK
		heap_leak_check_info *info = (heap_leak_check_info *)((addr_t)address
			+ bin->element_size - sizeof(heap_leak_check_info));
		info->size = size - sizeof(heap_leak_check_info);
		info->thread = (kernel_startup ? 0 : thread_get_current_thread_id());
		info->team = (kernel_startup ? 0 : team_get_current_team_id());
		info->caller = get_caller();
#endif
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
		heap->free_page_count--;
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

#if KERNEL_HEAP_LEAK_CHECK
		heap_leak_check_info *info = (heap_leak_check_info *)(heap->base
			+ page->index * heap->page_size + bin->element_size
			- sizeof(heap_leak_check_info));
		info->size = size - sizeof(heap_leak_check_info);
		info->thread = (kernel_startup ? 0 : thread_get_current_thread_id());
		info->team = (kernel_startup ? 0 : team_get_current_team_id());
		info->caller = get_caller();
#endif

		// we return the first slot in this page
		return (void *)(heap->base + page->index * heap->page_size);
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
			if ((1 + i - first) * heap->page_size >= size) {
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

	uint32 pageCount = (size + heap->page_size - 1) / heap->page_size;
	for (uint32 i = first; i < first + pageCount; i++) {
		heap_page *page = &heap->page_table[i];
		page->in_use = 1;
		page->bin_index = binIndex;

		heap_unlink_page(page, &heap->free_pages);

		page->next = page->prev = NULL;
		page->free_list = NULL;
		page->allocation_id = (uint16)first;
	}
	heap->free_page_count -= pageCount;

#if KERNEL_HEAP_LEAK_CHECK
	heap_leak_check_info *info = (heap_leak_check_info *)(heap->base
		+ (first + pageCount) * heap->page_size - sizeof(heap_leak_check_info));
	info->size = size - sizeof(heap_leak_check_info);
	info->thread = (kernel_startup ? 0 : thread_get_current_thread_id());
	info->team = (kernel_startup ? 0 : team_get_current_team_id());
	info->caller = get_caller();
#endif
	return (void *)(heap->base + first * heap->page_size);
}


#if DEBUG
static bool
is_valid_alignment(size_t number)
{
	// this cryptic line accepts zero and all powers of two
	return ((~number + 1) | ((number << 1) - 1)) == ~0UL;
}
#endif


inline bool
heap_should_grow(heap_allocator *heap)
{
	// suggest growing if it is the last heap and has less than 10% free pages
	return heap->next == NULL
		&& heap->free_page_count < heap->page_count / 10;
}


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

#if KERNEL_HEAP_LEAK_CHECK
	size += sizeof(heap_leak_check_info);
#endif

	uint32 binIndex;
	for (binIndex = 0; binIndex < heap->bin_count; binIndex++) {
		// TODO: The alignment is done by ensuring that the element size
		// of the target bin is aligned with the requested alignment. This
		// has the problem that it wastes space because a better (smaller) bin
		// could possibly be selected. We should pick the best bin and check
		// if there is an aligned block in the free list or if a new (page
		// aligned) page has to be allocated anyway.
		size_t elementSize = heap->bins[binIndex].element_size;
		if (size <= elementSize && (alignment == 0
			|| (elementSize & (alignment - 1)) == 0))
			break;
	}

	void *address = heap_raw_alloc(heap, size, binIndex);
	if (alignment != 0 && ((addr_t)address & (alignment - 1)))
		panic("alignment not met at %p with 0x%08lx\n", address, alignment);

	TRACE(("memalign(): asked to allocate %lu bytes, returning pointer %p\n", size, address));

	if (shouldGrow)
		*shouldGrow = heap_should_grow(heap);

#if KERNEL_HEAP_LEAK_CHECK
	size -= sizeof(heap_leak_check_info);
#endif

	T(Allocate((addr_t)address, size));
	mutex_unlock(&heap->lock);
	if (address == NULL)
		return address;

#if PARANOID_KFREE
	// make sure 0xdeadbeef is cleared if we do not overwrite the memory
	// and the user does not clear it
	if (((uint32 *)address)[1] == 0xdeadbeef)
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

	heap_page *page = &heap->page_table[((addr_t)address - heap->base)
		/ heap->page_size];

	TRACE(("free(): page %p: bin_index %d, free_count %d\n", page, page->bin_index, page->free_count));

	if (page->bin_index > heap->bin_count) {
		panic("free(): page %p: invalid bin_index %d\n", page, page->bin_index);
		mutex_unlock(&heap->lock);
		return B_ERROR;
	}

	if (page->bin_index < heap->bin_count) {
		// small allocation
		heap_bin *bin = &heap->bins[page->bin_index];
		if (((addr_t)address - heap->base - page->index
			* heap->page_size) % bin->element_size != 0) {
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
			heap->free_page_count++;
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
			heap->free_page_count++;
		}
	}

	if (heap->free_page_count == heap->page_count)
		dprintf("heap_free: heap %p is completely empty and could be freed\n", heap);

	T(Free((addr_t)address));
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

	heap_page *page = &heap->page_table[((addr_t)address - heap->base)
		/ heap->page_size];
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
		maxSize = heap->page_size;
		for (uint32 i = 1; i < maxPages; i++) {
			if (!page[i].in_use || page[i].bin_index != heap->bin_count
				|| page[i].allocation_id != allocationID)
				break;

			minSize += heap->page_size;
			maxSize += heap->page_size;
		}
	}

	mutex_unlock(&heap->lock);

#if KERNEL_HEAP_LEAK_CHECK
	newSize += sizeof(heap_leak_check_info);
#endif

	// does the new allocation simply fit in the old allocation?
	if (newSize > minSize && newSize <= maxSize) {
#if KERNEL_HEAP_LEAK_CHECK
		// update the size info (the info is at the end so stays where it is)
		heap_leak_check_info *info = (heap_leak_check_info *)((addr_t)address
			+ maxSize - sizeof(heap_leak_check_info));
		info->size = newSize - sizeof(heap_leak_check_info);
		newSize -= sizeof(heap_leak_check_info);
#endif

		T(Reallocate((addr_t)address, (addr_t)address, newSize));
		*newAddress = address;
		return B_OK;
	}

#if KERNEL_HEAP_LEAK_CHECK
	// new leak check info will be created with the malloc below
	newSize -= sizeof(heap_leak_check_info);
#endif

	// if not, allocate a new chunk of memory
	*newAddress = malloc(newSize);
	T(Reallocate((addr_t)address, (addr_t)*newAddress, newSize));
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
#if KERNEL_HEAP_LEAK_CHECK
	// take the extra info size into account
	size += sizeof(heap_leak_check_info_s);
#endif

	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		if (size <= sHeapClasses[i].max_allocation_size)
			return i;
	}

	return HEAP_CLASS_COUNT - 1;
}


static void
deferred_deleter(void *arg, int iteration)
{
	// move entries to on-stack list
	InterruptsSpinLocker locker(sDeferredFreeListLock);
	if (sDeferredFreeList.IsEmpty())
		return;

	DeferredFreeList entries;
	entries.MoveFrom(&sDeferredFreeList);

	locker.Unlock();

	// free the entries
	while (DeferredFreeListEntry* entry = entries.RemoveHead())
		free(entry);
}


//	#pragma mark -


static heap_allocator *
heap_create_new_heap(const char *name, size_t size, uint32 heapClass)
{
	void *heapAddress = NULL;
	area_id heapArea = create_area(name, &heapAddress,
		B_ANY_KERNEL_BLOCK_ADDRESS, size, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (heapArea < B_OK) {
		TRACE(("heap: couldn't allocate heap \"%s\"\n", name));
		return NULL;
	}

	heap_allocator *newHeap = heap_attach((addr_t)heapAddress, size, heapClass);
	if (newHeap == NULL) {
		panic("heap: could not attach heap to area!\n");
		delete_area(heapArea);
		return NULL;
	}

#if PARANOID_VALIDATION
	heap_validate_heap(newHeap);
#endif
	return newHeap;
}


static int32
heap_grow_thread(void *)
{
	heap_allocator *growHeap = sGrowHeapList;
	while (true) {
		// wait for a request to grow the heap list
		if (acquire_sem(sHeapGrowSem) < B_OK)
			continue;

		if (sAddGrowHeap) {
			while (growHeap->next)
				growHeap = growHeap->next;

			// the last grow heap is going to run full soon, try to allocate
			// a new one to make some room.
			TRACE(("heap_grower: grow heaps will run out of memory soon\n"));
			heap_allocator *newHeap = heap_create_new_heap(
				"additional grow heap", HEAP_DEDICATED_GROW_SIZE, 0);
			if (newHeap != NULL) {
#if PARANOID_VALIDATION
				heap_validate_heap(newHeap);
#endif
				growHeap->next = newHeap;
				sAddGrowHeap = false;
				TRACE(("heap_grower: new grow heap %p linked in\n", newHeap));
			} else
				dprintf("heap_grower: failed to create new grow heap\n");
		}

		for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
			// find the last heap
			heap_allocator *heap = sHeapList[i];
			while (heap->next)
				heap = heap->next;

			if (sLastGrowRequest[i] == heap || heap_should_grow(heap)) {
				// grow this heap if it is nearly full or if a grow was
				// explicitly requested for this heap (happens when a large
				// allocation cannot be fulfilled due to lack of contiguous
				// pages)
				heap_allocator *newHeap = heap_create_new_heap("additional heap",
					HEAP_GROW_SIZE, i);
				if (newHeap != NULL) {
#if PARANOID_VALIDATION
					heap_validate_heap(newHeap);
#endif
					heap->next = newHeap;
					TRACE(("heap_grower: new %s heap linked in\n",
						sHeapClasses[i].name));
				} else
					dprintf("heap_grower: failed to create new heap\n");
			}
		}

		// notify anyone waiting for this request
		release_sem_etc(sHeapGrownNotify, -1, B_RELEASE_ALL);
	}

	return 0;
}


status_t
heap_init(addr_t base, size_t size)
{
	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		size_t partSize = size * sHeapClasses[i].initial_percentage / 100;
		sHeapList[i] = heap_attach(base, partSize, i);
		sLastGrowRequest[i] = sHeapList[i];
		base += partSize;
	}

	// set up some debug commands
	add_debugger_command_etc("heap", &dump_heap_list,
		"Dump infos about the kernel heap(s)", "[(\"grow\" | \"stats\")]\n"
		"Dump infos about the kernel heap(s). If \"grow\" is specified, only\n"
		"infos about the dedicated grow heap are printed. If \"stats\" is\n"
		"given as the argument, currently only the heap count is printed\n", 0);
#if KERNEL_HEAP_LEAK_CHECK
	add_debugger_command_etc("allocations", &dump_allocations,
		"Dump current allocations",
		"[(\"team\" | \"thread\") <id>] [ \"caller\" <address> ] [\"stats\"]\n"
		"If no parameters are given, all current alloactions are dumped.\n"
		"If \"team\", \"thread\", and/or \"caller\" is specified as the first\n"
		"argument, only allocations matching the team id, thread id, or \n"
		"caller address given in the second argument are printed.\n"
		"If the optional argument \"stats\" is specified, only the allocation\n"
		"counts and no individual allocations are printed\n", 0);
	add_debugger_command_etc("allocations_per_caller",
		&dump_allocations_per_caller,
		"Dump current allocations summed up per caller",
		"[ \"-c\" ]\n"
		"The current allocations will by summed up by caller (their count and\n"
		"size) printed in decreasing order by size or, if \"-c\" is\n"
		"specified, by allocation count.\n", 0);
#endif
	return B_OK;
}


status_t
heap_init_post_sem()
{
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
	sGrowHeapList = heap_create_new_heap("dedicated grow heap",
		HEAP_DEDICATED_GROW_SIZE, 0);
	if (sGrowHeapList == NULL) {
		panic("heap_init_post_thread(): failed to attach dedicated grow heap\n");
		return B_ERROR;
	}

	sHeapGrowThread = spawn_kernel_thread(heap_grow_thread, "heap grower",
		B_URGENT_PRIORITY, NULL);
	if (sHeapGrowThread < 0) {
		panic("heap_init_post_thread(): cannot create heap grow thread\n");
		return sHeapGrowThread;
	}

	if (register_kernel_daemon(deferred_deleter, NULL, 50) != B_OK)
		panic("heap_init_post_thread(): failed to init deferred deleter");

	send_signal_etc(sHeapGrowThread, SIGCONT, B_DO_NOT_RESCHEDULE);
	return B_OK;
}


//	#pragma mark - Public API


void *
memalign(size_t alignment, size_t size)
{
	if (!kernel_startup && !are_interrupts_enabled()) {
		panic("memalign(): called with interrupts disabled\n");
		return NULL;
	}

	if (!kernel_startup && size > HEAP_AREA_USE_THRESHOLD) {
		// don't even attempt such a huge allocation - use areas instead
		dprintf("heap: using area for huge allocation of %lu bytes!\n", size);
		size_t areaSize = size + sizeof(area_allocation_info);
		if (alignment != 0)
			areaSize = ROUNDUP(areaSize, alignment);

		void *address = NULL;
		areaSize = ROUNDUP(areaSize, B_PAGE_SIZE);
		area_id allocationArea = create_area("memalign area", &address,
			B_ANY_KERNEL_BLOCK_ADDRESS, areaSize, B_FULL_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (allocationArea < B_OK) {
			dprintf("heap: failed to create area for huge allocation\n");
			return NULL;
		}

		area_allocation_info *info = (area_allocation_info *)address;
		info->magic = kAreaAllocationMagic;
		info->area = allocationArea;
		info->base = address;
		info->size = areaSize;
		info->allocation_size = size;
		info->allocation_alignment = alignment;

		address = (void *)((addr_t)address + sizeof(area_allocation_info));
		if (alignment != 0)
			address = (void *)ROUNDUP((addr_t)address, alignment);

		dprintf("heap: allocated area %ld for huge allocation returning %p\n",
			allocationArea, address);

		info->allocation_base = address;
		return address;
	}

	heap_allocator *heap = sHeapList[heap_class_for(size)];
	while (heap) {
		bool shouldGrow = false;
		void *result = heap_memalign(heap, alignment, size, &shouldGrow);
		if (heap->next == NULL && (shouldGrow || result == NULL)) {
			// the last heap will or has run out of memory, notify the grower
			sLastGrowRequest[heap_class_for(size)] = heap;
			if (result == NULL) {
				// urgent request, do the request and wait
				switch_sem(sHeapGrowSem, sHeapGrownNotify);
				if (heap->next == NULL) {
					// the grower didn't manage to add a new heap
					return NULL;
				}
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
malloc_nogrow(size_t size)
{
	// use dedicated memory in the grow thread by default
	if (thread_get_current_thread_id() == sHeapGrowThread) {
		bool shouldGrow = false;
		heap_allocator *heap = sGrowHeapList;
		while (heap) {
			void *result = heap_memalign(heap, 0, size, &shouldGrow);
			if (shouldGrow && heap->next == NULL && !sAddGrowHeap) {
				// hopefully the heap grower will manage to create a new heap
				// before running out of private memory...
				dprintf("heap: requesting new grow heap\n");
				sAddGrowHeap = true;
				release_sem_etc(sHeapGrowSem, 1, B_DO_NOT_RESCHEDULE);
			}

			if (result != NULL)
				return result;

			heap = heap->next;
		}
	}

	// try public memory, there might be something available
	heap_allocator *heap = sHeapList[heap_class_for(size)];
	while (heap) {
		void *result = heap_memalign(heap, 0, size, NULL);
		if (result != NULL)
			return result;

		heap = heap->next;
	}

	// no memory available
	if (thread_get_current_thread_id() == sHeapGrowThread)
		panic("heap: all heaps have run out of memory while growing\n");
	else
		dprintf("heap: all heaps have run out of memory\n");
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

	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		heap_allocator *heap = sHeapList[i];
		while (heap) {
			if (heap_free(heap, address) == B_OK) {
#if PARANOID_VALIDATION
				heap_validate_heap(heap);
#endif
				return;
			}

			heap = heap->next;
		}
	}

	// maybe it was allocated from a dedicated grow heap
	heap_allocator *heap = sGrowHeapList;
	while (heap) {
		if (heap_free(heap, address) == B_OK)
			return;

		heap = heap->next;
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
			dprintf("free(): freed huge allocation by deleting area %ld\n",
				area);
			return;
		}
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
		return memalign(0, newSize);

	if (newSize == 0) {
		free(address);
		return NULL;
	}

	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		heap_allocator *heap = sHeapList[i];
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
	}

	// maybe a huge allocation using an area
	area_info areaInfo;
	area_id area = area_for(address);
	if (area >= B_OK && get_area_info(area, &areaInfo) == B_OK) {
		area_allocation_info *info = (area_allocation_info *)areaInfo.address;

		// just make extra sure it was allocated by us
		if (info->magic == kAreaAllocationMagic && info->area == area
			&& info->size == areaInfo.size && info->base == areaInfo.address
			&& info->allocation_size < areaInfo.size) {
			size_t available = info->size - ((addr_t)info->allocation_base
				- (addr_t)info->base);

			if (available >= newSize) {
				// there is enough room available for the newSize
				dprintf("realloc(): new size %ld fits in old area %ld with %ld available",
					newSize, area, available);
				return address;
			}

			// have to allocate/copy/free - TODO maybe resize the area instead?
			void *newAddress = memalign(0, newSize);
			if (newAddress == NULL) {
				dprintf("realloc(): failed to allocate new block of %ld bytes\n",
					newSize);
				return NULL;
			}

			memcpy(newAddress, address, min_c(newSize, info->allocation_size));
			delete_area(area);
			dprintf("realloc(): allocated new block %p for size %ld and deleted old area %ld\n",
				newAddress, newSize, area);
			return newAddress;
		}
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


void
deferred_free(void* block)
{
	if (block == NULL)
		return;

	// TODO: Use SinglyLinkedList, so that we only need sizeof(void*).
	DeferredFreeListEntry* entry = new(block) DeferredFreeListEntry;

	InterruptsSpinLocker _(sDeferredFreeListLock);
	sDeferredFreeList.Add(entry);
}


void*
malloc_referenced(size_t size)
{
	int32* referencedData = (int32*)malloc(size + 4);
	if (referencedData == NULL)
		return NULL;

	*referencedData = 1;

	return referencedData + 1;
}


void*
malloc_referenced_acquire(void* data)
{
	if (data != NULL) {
		int32* referencedData = (int32*)data - 1;
		atomic_add(referencedData, 1);
	}

	return data;
}


void
malloc_referenced_release(void* data)
{
	if (data == NULL)
		return;

	int32* referencedData = (int32*)data - 1;
	if (atomic_add(referencedData, -1) < 1)
		free(referencedData);
}
