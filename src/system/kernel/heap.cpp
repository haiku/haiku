/*
 * Copyright 2008-2010, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
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
#include <string.h>
#include <team.h>
#include <thread.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_page.h>


//#define TRACE_HEAP
#ifdef TRACE_HEAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#if USE_SLAB_ALLOCATOR_FOR_MALLOC
#	undef KERNEL_HEAP_LEAK_CHECK
#endif


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
#endif	// KERNEL_HEAP_LEAK_CHECK


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


#define MAX_BIN_COUNT	31	// depends on the size of the bin_index field

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


struct heap_allocator_s {
	rw_lock		area_lock;
	mutex		page_lock;

	const char *name;
	uint32		bin_count;
	uint32		page_size;

	uint32		total_pages;
	uint32		total_free_pages;
	uint32		empty_areas;

#if KERNEL_HEAP_LEAK_CHECK
	addr_t		(*get_caller)();
#endif

	heap_bin *	bins;
	heap_area *	areas; // sorted so that the desired area is always first
	heap_area *	all_areas; // all areas including full ones
};


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


struct DeferredFreeListEntry : SinglyLinkedListLinkImpl<DeferredFreeListEntry> {
};


typedef SinglyLinkedList<DeferredFreeListEntry> DeferredFreeList;
typedef SinglyLinkedList<DeferredDeletable> DeferredDeletableList;


#if !USE_SLAB_ALLOCATOR_FOR_MALLOC

#define VIP_HEAP_SIZE	1024 * 1024

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


static uint32 sHeapCount;
static heap_allocator *sHeaps[HEAP_CLASS_COUNT * B_MAX_CPU_COUNT];
static uint32 *sLastGrowRequest[HEAP_CLASS_COUNT * B_MAX_CPU_COUNT];
static uint32 *sLastHandledGrowRequest[HEAP_CLASS_COUNT * B_MAX_CPU_COUNT];

static heap_allocator *sVIPHeap;
static heap_allocator *sGrowHeap = NULL;
static thread_id sHeapGrowThread = -1;
static sem_id sHeapGrowSem = -1;
static sem_id sHeapGrownNotify = -1;
static bool sAddGrowHeap = false;

#endif	// !USE_SLAB_ALLOCATOR_FOR_MALLOC

static DeferredFreeList sDeferredFreeList;
static DeferredDeletableList sDeferredDeletableList;
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

#	define T(x)	if (!gKernelStartup) new(std::nothrow) KernelHeapTracing::x;
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
	int32 depth = arch_debug_get_stack_trace(returnAddresses, 5, 0, 1,
		STACK_TRACE_KERNEL);
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

	kprintf("\t\tpage %p: bin_index: %u; free_count: %u; empty_index: %u; "
		"free_list %p (%lu entr%s)\n", page, page->bin_index, page->free_count,
		page->empty_index, page->free_list, count, count == 1 ? "y" : "ies");
}


static void
dump_bin(heap_bin *bin)
{
	uint32 count = 0;
	for (heap_page *page = bin->page_list; page != NULL; page = page->next)
		count++;

	kprintf("\telement_size: %lu; max_free_count: %u; page_list %p (%lu pages"
		");\n", bin->element_size, bin->max_free_count, bin->page_list, count);

	for (heap_page *page = bin->page_list; page != NULL; page = page->next)
		dump_page(page);
}


static void
dump_bin_list(heap_allocator *heap)
{
	for (uint32 i = 0; i < heap->bin_count; i++)
		dump_bin(&heap->bins[i]);
	kprintf("\n");
}


static void
dump_allocator_areas(heap_allocator *heap)
{
	heap_area *area = heap->all_areas;
	while (area) {
		kprintf("\tarea %p: area: %ld; base: 0x%08lx; size: %lu; page_count: "
			"%lu; free_pages: %p (%lu entr%s)\n", area, area->area, area->base,
			area->size, area->page_count, area->free_pages,
			area->free_page_count, area->free_page_count == 1 ? "y" : "ies");
		area = area->all_next;
	}

	kprintf("\n");
}


static void
dump_allocator(heap_allocator *heap, bool areas, bool bins)
{
	kprintf("allocator %p: name: %s; page_size: %lu; bin_count: %lu; pages: "
		"%lu; free_pages: %lu; empty_areas: %lu\n", heap, heap->name,
		heap->page_size, heap->bin_count, heap->total_pages,
		heap->total_free_pages, heap->empty_areas);

	if (areas)
		dump_allocator_areas(heap);
	if (bins)
		dump_bin_list(heap);
}


static int
dump_heap_list(int argc, char **argv)
{
#if !USE_SLAB_ALLOCATOR_FOR_MALLOC
	if (argc == 2 && strcmp(argv[1], "grow") == 0) {
		// only dump dedicated grow heap info
		kprintf("dedicated grow heap:\n");
		dump_allocator(sGrowHeap, true, true);
		return 0;
	}
#endif

	bool stats = false;
	int i = 1;

	if (strcmp(argv[1], "stats") == 0) {
		stats = true;
		i++;
	}

	uint64 heapAddress = 0;
	if (i < argc && !evaluate_debug_expression(argv[i], &heapAddress, true)) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (heapAddress == 0) {
#if !USE_SLAB_ALLOCATOR_FOR_MALLOC
		// dump default kernel heaps
		for (uint32 i = 0; i < sHeapCount; i++)
			dump_allocator(sHeaps[i], !stats, !stats);
#else
		print_debugger_command_usage(argv[0]);
#endif
	} else {
		// dump specified heap
		dump_allocator((heap_allocator*)(addr_t)heapAddress, !stats, !stats);
	}

	return 0;
}


#if !KERNEL_HEAP_LEAK_CHECK

static int
dump_allocations(int argc, char **argv)
{
	uint64 heapAddress = 0;
	bool statsOnly = false;
	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "stats") == 0)
			statsOnly = true;
		else if (!evaluate_debug_expression(argv[i], &heapAddress, true)) {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	size_t totalSize = 0;
	uint32 totalCount = 0;
#if !USE_SLAB_ALLOCATOR_FOR_MALLOC
	for (uint32 heapIndex = 0; heapIndex < sHeapCount; heapIndex++) {
		heap_allocator *heap = sHeaps[heapIndex];
		if (heapAddress != 0)
			heap = (heap_allocator *)(addr_t)heapAddress;
#else
	while (true) {
		heap_allocator *heap = (heap_allocator *)(addr_t)heapAddress;
		if (heap == NULL) {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
#endif
#if 0
	}
#endif

		// go through all the pages in all the areas
		heap_area *area = heap->all_areas;
		while (area) {
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

						if (!statsOnly) {
							kprintf("address: 0x%08lx; size: %lu bytes\n",
								base, elementSize);
						}

						totalSize += elementSize;
						totalCount++;
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

					size_t size = pageCount * heap->page_size;

					if (!statsOnly) {
						kprintf("address: 0x%08lx; size: %lu bytes\n", base,
							size);
					}

					totalSize += size;
					totalCount++;

					// skip the allocated pages
					i += pageCount - 1;
				}
			}

			area = area->all_next;
		}

		if (heapAddress != 0)
			break;
	}

	kprintf("total allocations: %lu; total bytes: %lu\n", totalCount, totalSize);
	return 0;
}

#else // !KERNEL_HEAP_LEAK_CHECK

static int
dump_allocations(int argc, char **argv)
{
	team_id team = -1;
	thread_id thread = -1;
	addr_t caller = 0;
	addr_t address = 0;
	bool statsOnly = false;

	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "team") == 0)
			team = parse_expression(argv[++i]);
		else if (strcmp(argv[i], "thread") == 0)
			thread = parse_expression(argv[++i]);
		else if (strcmp(argv[i], "caller") == 0)
			caller = parse_expression(argv[++i]);
		else if (strcmp(argv[i], "address") == 0)
			address = parse_expression(argv[++i]);
		else if (strcmp(argv[i], "stats") == 0)
			statsOnly = true;
		else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	size_t totalSize = 0;
	uint32 totalCount = 0;
	for (uint32 heapIndex = 0; heapIndex < sHeapCount; heapIndex++) {
		heap_allocator *heap = sHeaps[heapIndex];

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

						if ((team == -1 || info->team == team)
							&& (thread == -1 || info->thread == thread)
							&& (caller == 0 || info->caller == caller)
							&& (address == 0 || base == address)) {
							// interesting...
							if (!statsOnly) {
								kprintf("team: % 6ld; thread: % 6ld; "
									"address: 0x%08lx; size: %lu bytes; "
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
					while (i + pageCount < area->page_count
						&& area->page_table[i + pageCount].in_use
						&& area->page_table[i + pageCount].bin_index
							== heap->bin_count
						&& area->page_table[i + pageCount].allocation_id
							== page->allocation_id)
						pageCount++;

					info = (heap_leak_check_info *)(base + pageCount
						* heap->page_size - sizeof(heap_leak_check_info));

					if ((team == -1 || info->team == team)
						&& (thread == -1 || info->thread == thread)
						&& (caller == 0 || info->caller == caller)
						&& (address == 0 || base == address)) {
						// interesting...
						if (!statsOnly) {
							kprintf("team: % 6ld; thread: % 6ld;"
								" address: 0x%08lx; size: %lu bytes;"
								" caller: %#lx\n", info->team, info->thread,
								base, info->size, info->caller);
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

	kprintf("total allocations: %lu; total bytes: %lu\n", totalCount,
		totalSize);
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


static bool
analyze_allocation_callers(heap_allocator *heap)
{
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

					caller_info *callerInfo = get_caller_info(info->caller);
					if (callerInfo == NULL) {
						kprintf("out of space for caller infos\n");
						return false;
					}

					callerInfo->count++;
					callerInfo->size += info->size;
				}
			} else {
				// page is used by a big allocation, find the page count
				uint32 pageCount = 1;
				while (i + pageCount < area->page_count
					&& area->page_table[i + pageCount].in_use
					&& area->page_table[i + pageCount].bin_index
						== heap->bin_count
					&& area->page_table[i + pageCount].allocation_id
						== page->allocation_id) {
					pageCount++;
				}

				info = (heap_leak_check_info *)(base + pageCount
					* heap->page_size - sizeof(heap_leak_check_info));

				caller_info *callerInfo = get_caller_info(info->caller);
				if (callerInfo == NULL) {
					kprintf("out of space for caller infos\n");
					return false;
				}

				callerInfo->count++;
				callerInfo->size += info->size;

				// skip the allocated pages
				i += pageCount - 1;
			}
		}

		area = area->all_next;
	}

	return true;
}


static int
dump_allocations_per_caller(int argc, char **argv)
{
	bool sortBySize = true;
	heap_allocator *heap = NULL;

	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0) {
			sortBySize = false;
		} else if (strcmp(argv[i], "-h") == 0) {
			uint64 heapAddress;
			if (++i >= argc
				|| !evaluate_debug_expression(argv[i], &heapAddress, true)) {
				print_debugger_command_usage(argv[0]);
				return 0;
			}

			heap = (heap_allocator*)(addr_t)heapAddress;
		} else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	sCallerInfoCount = 0;

	if (heap != NULL) {
		if (!analyze_allocation_callers(heap))
			return 0;
	} else {
		for (uint32 heapIndex = 0; heapIndex < sHeapCount; heapIndex++) {
			if (!analyze_allocation_callers(sHeaps[heapIndex]))
				return 0;
		}
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

		const char *symbol;
		const char *imageName;
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


#if PARANOID_HEAP_VALIDATION
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
		if (area->free_page_count != freePageCount)
			panic("free page count doesn't match free page list\n");

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
				panic("used page has invalid index\n");

			if ((addr_t)&area->page_table[page->index] != (addr_t)page)
				panic("used page index does not lead to target page\n");

			if (page->prev != lastPage) {
				panic("used page entry has invalid prev link (%p vs %p bin "
					"%lu)\n", page->prev, lastPage, i);
			}

			if (!page->in_use)
				panic("used page marked as not in use\n");

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
					panic("free list entry out of page range\n");

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
	areaReadLocker.Unlock();
}
#endif // PARANOID_HEAP_VALIDATION


// #pragma mark - Heap functions


void
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

	dprintf("heap_add_area: area %ld added to %s heap %p - usable range 0x%08lx "
		"- 0x%08lx\n", area->area, heap->name, heap, area->base,
		area->base + area->size);
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

	dprintf("heap_remove_area: area %ld with range 0x%08lx - 0x%08lx removed "
		"from %s heap %p\n", area->area, area->base, area->base + area->size,
		heap->name, heap);

	return B_OK;
}


heap_allocator *
heap_create_allocator(const char *name, addr_t base, size_t size,
	const heap_class *heapClass, bool allocateOnHeap)
{
	heap_allocator *heap;
	if (allocateOnHeap) {
		// allocate seperately on the heap
		heap = (heap_allocator *)malloc(sizeof(heap_allocator)
			+ sizeof(heap_bin) * MAX_BIN_COUNT);
	} else {
		// use up the first part of the area
		heap = (heap_allocator *)base;
		base += sizeof(heap_allocator);
		size -= sizeof(heap_allocator);
	}

	heap->name = name;
	heap->page_size = heapClass->page_size;
	heap->total_pages = heap->total_free_pages = heap->empty_areas = 0;
	heap->areas = heap->all_areas = NULL;
	heap->bins = (heap_bin *)((addr_t)heap + sizeof(heap_allocator));

#if KERNEL_HEAP_LEAK_CHECK
	heap->get_caller = &get_caller;
#endif

	heap->bin_count = 0;
	size_t binSize = 0, lastSize = 0;
	uint32 count = heap->page_size / heapClass->min_bin_size;
	for (; count >= heapClass->min_count_per_page; count--, lastSize = binSize) {
		if (heap->bin_count >= MAX_BIN_COUNT)
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

	if (!allocateOnHeap) {
		base += heap->bin_count * sizeof(heap_bin);
		size -= heap->bin_count * sizeof(heap_bin);
	}

	rw_lock_init(&heap->area_lock, "heap area rw lock");
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
	MutexLocker pageLocker(heap->page_lock);
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


#if KERNEL_HEAP_LEAK_CHECK
static void
heap_add_leak_check_info(heap_allocator *heap, addr_t address, size_t allocated,
	size_t size)
{
	heap_leak_check_info *info = (heap_leak_check_info *)(address + allocated
		- sizeof(heap_leak_check_info));
	info->size = size - sizeof(heap_leak_check_info);
	info->thread = (gKernelStartup ? 0 : thread_get_current_thread_id());
	info->team = (gKernelStartup ? 0 : team_get_current_team_id());
	info->caller = heap->get_caller();
}
#endif


static void *
heap_raw_alloc(heap_allocator *heap, size_t size, size_t alignment)
{
	TRACE(("heap %p: allocate %lu bytes from raw pages with alignment %lu\n",
		heap, size, alignment));

	uint32 pageCount = (size + heap->page_size - 1) / heap->page_size;
	heap_page *firstPage = heap_allocate_contiguous_pages(heap, pageCount,
		alignment);
	if (firstPage == NULL) {
		TRACE(("heap %p: found no contiguous pages to allocate %ld bytes\n",
			heap, size));
		return NULL;
	}

	addr_t address = firstPage->area->base + firstPage->index * heap->page_size;
#if KERNEL_HEAP_LEAK_CHECK
	heap_add_leak_check_info(heap, address, pageCount * heap->page_size, size);
#endif
	return (void *)address;
}


static void *
heap_allocate_from_bin(heap_allocator *heap, uint32 binIndex, size_t size)
{
	heap_bin *bin = &heap->bins[binIndex];
	TRACE(("heap %p: allocate %lu bytes from bin %lu with element_size %lu\n",
		heap, size, binIndex, bin->element_size));

	MutexLocker binLocker(bin->lock);
	heap_page *page = bin->page_list;
	if (page == NULL) {
		MutexLocker pageLocker(heap->page_lock);
		heap_area *area = heap->areas;
		if (area == NULL) {
			TRACE(("heap %p: no free pages to allocate %lu bytes\n", heap,
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
			panic("got an in use page %p from the free pages list\n", page);
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

#if KERNEL_HEAP_LEAK_CHECK
	binLocker.Unlock();
	heap_add_leak_check_info(heap, (addr_t)address, bin->element_size, size);
#endif
	return address;
}


static bool
is_valid_alignment(size_t number)
{
	// this cryptic line accepts zero and all powers of two
	return ((~number + 1) | ((number << 1) - 1)) == ~0UL;
}


inline bool
heap_should_grow(heap_allocator *heap)
{
	// suggest growing if there is less than 20% of a grow size available
	return heap->total_free_pages * heap->page_size < HEAP_GROW_SIZE / 5;
}


void *
heap_memalign(heap_allocator *heap, size_t alignment, size_t size)
{
	TRACE(("memalign(alignment = %lu, size = %lu)\n", alignment, size));

#if DEBUG
	if (!is_valid_alignment(alignment))
		panic("memalign() with an alignment which is not a power of 2\n");
#endif

#if KERNEL_HEAP_LEAK_CHECK
	size += sizeof(heap_leak_check_info);
#endif

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

#if KERNEL_HEAP_LEAK_CHECK
	size -= sizeof(heap_leak_check_info);
#endif

	TRACE(("memalign(): asked to allocate %lu bytes, returning pointer %p\n",
		size, address));

	T(Allocate((addr_t)address, size));
	if (address == NULL)
		return address;

#if PARANOID_KERNEL_MALLOC
	memset(address, 0xcc, size);
#endif

#if PARANOID_KERNEL_FREE
	// make sure 0xdeadbeef is cleared if we do not overwrite the memory
	// and the user does not clear it
	if (((uint32 *)address)[1] == 0xdeadbeef)
		((uint32 *)address)[1] = 0xcccccccc;
#endif

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

	TRACE(("free(): asked to free pointer %p\n", address));

	heap_page *page = &area->page_table[((addr_t)address - area->base)
		/ heap->page_size];

	TRACE(("free(): page %p: bin_index %d, free_count %d\n", page,
		page->bin_index, page->free_count));

	if (page->bin_index > heap->bin_count) {
		panic("free(): page %p: invalid bin_index %d\n", page, page->bin_index);
		return B_ERROR;
	}

	if (page->bin_index < heap->bin_count) {
		// small allocation
		heap_bin *bin = &heap->bins[page->bin_index];

#if PARANOID_KERNEL_FREE
		if (((uint32 *)address)[1] == 0xdeadbeef) {
			// This block looks like it was freed already, walk the free list
			// on this page to make sure this address doesn't exist.
			MutexLocker binLocker(bin->lock);
			for (addr_t *temp = page->free_list; temp != NULL;
					temp = (addr_t *)*temp) {
				if (temp == address) {
					panic("free(): address %p already exists in page free "
						"list\n", address);
					return B_ERROR;
				}
			}
		}

		// the first 4 bytes are overwritten with the next free list pointer
		// later
		uint32 *dead = (uint32 *)address;
		for (uint32 i = 1; i < bin->element_size / sizeof(uint32); i++)
			dead[i] = 0xdeadbeef;
#endif

		MutexLocker binLocker(bin->lock);
		if (((addr_t)address - area->base - page->index
			* heap->page_size) % bin->element_size != 0) {
			panic("free(): passed invalid pointer %p supposed to be in bin for "
				"element size %ld\n", address, bin->element_size);
			return B_ERROR;
		}

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
	} else {
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
			page[i].in_use = 0;
			page[i].allocation_id = 0;

			// return it to the free list
			heap_link_page(&page[i], &area->free_pages);
			pageCount++;
		}

		heap_free_pages_added(heap, area, pageCount);
	}

	T(Free((addr_t)address));
	areaReadLocker.Unlock();

	if (heap->empty_areas > 1) {
		WriteLocker areaWriteLocker(heap->area_lock);
		MutexLocker pageLocker(heap->page_lock);

		area_id areasToDelete[heap->empty_areas - 1];
		int32 areasToDeleteIndex = 0;

		area = heap->areas;
		while (area != NULL && heap->empty_areas > 1) {
			heap_area *next = area->next;
			if (area->area >= 0
				&& area->free_page_count == area->page_count
				&& heap_remove_area(heap, area) == B_OK) {
				areasToDelete[areasToDeleteIndex++] = area->area;
				heap->empty_areas--;
			}

			area = next;
		}

		pageLocker.Unlock();
		areaWriteLocker.Unlock();

		for (int32 i = 0; i < areasToDeleteIndex; i++)
			delete_area(areasToDelete[i]);
	}

	return B_OK;
}


#if KERNEL_HEAP_LEAK_CHECK
extern "C" void
heap_set_get_caller(heap_allocator* heap, addr_t (*getCaller)())
{
	heap->get_caller = getCaller;
}
#endif


#if !USE_SLAB_ALLOCATOR_FOR_MALLOC


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

	TRACE(("realloc(address = %p, newSize = %lu)\n", address, newSize));

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

	areaReadLocker.Unlock();

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
	*newAddress = memalign(0, newSize);
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
heap_index_for(size_t size, int32 cpu)
{
#if KERNEL_HEAP_LEAK_CHECK
	// take the extra info size into account
	size += sizeof(heap_leak_check_info_s);
#endif

	uint32 index = 0;
	for (; index < HEAP_CLASS_COUNT - 1; index++) {
		if (size <= sHeapClasses[index].max_allocation_size)
			break;
	}

	return (index + cpu * HEAP_CLASS_COUNT) % sHeapCount;
}


static void *
memalign_nogrow(size_t alignment, size_t size)
{
	// use dedicated memory in the grow thread by default
	if (thread_get_current_thread_id() == sHeapGrowThread) {
		void *result = heap_memalign(sGrowHeap, alignment, size);
		if (!sAddGrowHeap && heap_should_grow(sGrowHeap)) {
			// hopefully the heap grower will manage to create a new heap
			// before running out of private memory...
			dprintf("heap: requesting new grow heap\n");
			sAddGrowHeap = true;
			release_sem_etc(sHeapGrowSem, 1, B_DO_NOT_RESCHEDULE);
		}

		if (result != NULL)
			return result;
	}

	// try public memory, there might be something available
	void *result = NULL;
	int32 cpuCount = MIN(smp_get_num_cpus(),
		(int32)sHeapCount / HEAP_CLASS_COUNT);
	int32 cpuNumber = smp_get_current_cpu();
	for (int32 i = 0; i < cpuCount; i++) {
		uint32 heapIndex = heap_index_for(size, cpuNumber++ % cpuCount);
		heap_allocator *heap = sHeaps[heapIndex];
		result = heap_memalign(heap, alignment, size);
		if (result != NULL)
			return result;
	}

	// no memory available
	if (thread_get_current_thread_id() == sHeapGrowThread)
		panic("heap: all heaps have run out of memory while growing\n");
	else
		dprintf("heap: all heaps have run out of memory\n");

	return NULL;
}


static status_t
heap_create_new_heap_area(heap_allocator *heap, const char *name, size_t size)
{
	void *address = NULL;
	area_id heapArea = create_area(name, &address,
		B_ANY_KERNEL_BLOCK_ADDRESS, size, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (heapArea < B_OK) {
		TRACE(("heap: couldn't allocate heap area \"%s\"\n", name));
		return heapArea;
	}

	heap_add_area(heap, heapArea, (addr_t)address, size);
#if PARANOID_HEAP_VALIDATION
	heap_validate_heap(heap);
#endif
	return B_OK;
}


static int32
heap_grow_thread(void *)
{
	while (true) {
		// wait for a request to grow the heap list
		if (acquire_sem(sHeapGrowSem) < B_OK)
			continue;

		if (sAddGrowHeap) {
			// the grow heap is going to run full soon, try to allocate a new
			// one to make some room.
			TRACE(("heap_grower: grow heaps will run out of memory soon\n"));
			if (heap_create_new_heap_area(sGrowHeap, "additional grow heap",
					HEAP_DEDICATED_GROW_SIZE) != B_OK)
				dprintf("heap_grower: failed to create new grow heap area\n");
		}

		for (uint32 i = 0; i < sHeapCount; i++) {
			heap_allocator *heap = sHeaps[i];
			if (sLastGrowRequest[i] > sLastHandledGrowRequest[i]
				|| heap_should_grow(heap)) {
				// grow this heap if it is nearly full or if a grow was
				// explicitly requested for this heap (happens when a large
				// allocation cannot be fulfilled due to lack of contiguous
				// pages)
				if (heap_create_new_heap_area(heap, "additional heap",
						HEAP_GROW_SIZE) != B_OK)
					dprintf("heap_grower: failed to create new heap area\n");
				sLastHandledGrowRequest[i] = sLastGrowRequest[i];
			}
		}

		// notify anyone waiting for this request
		release_sem_etc(sHeapGrownNotify, -1, B_RELEASE_ALL);
	}

	return 0;
}


#endif	// !USE_SLAB_ALLOCATOR_FOR_MALLOC


static void
deferred_deleter(void *arg, int iteration)
{
	// move entries and deletables to on-stack lists
	InterruptsSpinLocker locker(sDeferredFreeListLock);
	if (sDeferredFreeList.IsEmpty() && sDeferredDeletableList.IsEmpty())
		return;

	DeferredFreeList entries;
	entries.MoveFrom(&sDeferredFreeList);

	DeferredDeletableList deletables;
	deletables.MoveFrom(&sDeferredDeletableList);

	locker.Unlock();

	// free the entries
	while (DeferredFreeListEntry* entry = entries.RemoveHead())
		free(entry);

	// delete the deletables
	while (DeferredDeletable* deletable = deletables.RemoveHead())
		delete deletable;
}


//	#pragma mark -


#if !USE_SLAB_ALLOCATOR_FOR_MALLOC


status_t
heap_init(addr_t base, size_t size)
{
	for (uint32 i = 0; i < HEAP_CLASS_COUNT; i++) {
		size_t partSize = size * sHeapClasses[i].initial_percentage / 100;
		sHeaps[i] = heap_create_allocator(sHeapClasses[i].name, base, partSize,
			&sHeapClasses[i], false);
		sLastGrowRequest[i] = sLastHandledGrowRequest[i] = 0;
		base += partSize;
		sHeapCount++;
	}

	// set up some debug commands
	add_debugger_command_etc("heap", &dump_heap_list,
		"Dump infos about the kernel heap(s)",
		"[(\"grow\" | \"stats\" | <heap>)]\n"
		"Dump infos about the kernel heap(s). If \"grow\" is specified, only\n"
		"infos about the dedicated grow heap are printed. If \"stats\" is\n"
		"given as the argument, currently only the heap count is printed.\n"
		"If <heap> is given, it is interpreted as the address of the heap to\n"
		"print infos about.\n", 0);
#if !KERNEL_HEAP_LEAK_CHECK
	add_debugger_command_etc("allocations", &dump_allocations,
		"Dump current heap allocations",
		"[\"stats\"] [<heap>]\n"
		"If no parameters are given, all current alloactions are dumped.\n"
		"If the optional argument \"stats\" is specified, only the allocation\n"
		"counts and no individual allocations are printed\n"
		"If a specific heap address is given, only allocations of this\n"
		"allocator are dumped\n", 0);
#else // !KERNEL_HEAP_LEAK_CHECK
	add_debugger_command_etc("allocations", &dump_allocations,
		"Dump current heap allocations",
		"[(\"team\" | \"thread\") <id>] [\"caller\" <address>] [\"address\" <address>] [\"stats\"]\n"
		"If no parameters are given, all current alloactions are dumped.\n"
		"If \"team\", \"thread\", \"caller\", and/or \"address\" is specified as the first\n"
		"argument, only allocations matching the team ID, thread ID, caller\n"
		"address or allocated address given in the second argument are printed.\n"
		"If the optional argument \"stats\" is specified, only the allocation\n"
		"counts and no individual allocations are printed.\n", 0);
	add_debugger_command_etc("allocations_per_caller",
		&dump_allocations_per_caller,
		"Dump current heap allocations summed up per caller",
		"[ \"-c\" ] [ -h <heap> ]\n"
		"The current allocations will by summed up by caller (their count and\n"
		"size) printed in decreasing order by size or, if \"-c\" is\n"
		"specified, by allocation count. If given <heap> specifies the\n"
		"address of the heap for which to print the allocations.\n", 0);
#endif // KERNEL_HEAP_LEAK_CHECK
	return B_OK;
}


status_t
heap_init_post_area()
{
	void *address = NULL;
	area_id growHeapArea = create_area("dedicated grow heap", &address,
		B_ANY_KERNEL_BLOCK_ADDRESS, HEAP_DEDICATED_GROW_SIZE, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (growHeapArea < 0) {
		panic("heap_init_post_area(): couldn't allocate dedicate grow heap "
			"area");
		return growHeapArea;
	}

	sGrowHeap = heap_create_allocator("grow", (addr_t)address,
		HEAP_DEDICATED_GROW_SIZE, &sHeapClasses[0], false);
	if (sGrowHeap == NULL) {
		panic("heap_init_post_area(): failed to create dedicated grow heap\n");
		return B_ERROR;
	}

	// create the VIP heap
	static const heap_class heapClass = {
		"VIP I/O",					/* name */
		100,						/* initial percentage */
		B_PAGE_SIZE / 8,			/* max allocation size */
		B_PAGE_SIZE,				/* page size */
		8,							/* min bin size */
		4,							/* bin alignment */
		8,							/* min count per page */
		16							/* max waste per page */
	};

	area_id vipHeapArea = create_area("VIP heap", &address,
		B_ANY_KERNEL_ADDRESS, VIP_HEAP_SIZE, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (vipHeapArea < 0) {
		panic("heap_init_post_area(): couldn't allocate VIP heap area");
		return B_ERROR;
	}

	sVIPHeap = heap_create_allocator("VIP heap", (addr_t)address,
		VIP_HEAP_SIZE, &heapClass, false);
	if (sVIPHeap == NULL) {
		panic("heap_init_post_area(): failed to create VIP heap\n");
		return B_ERROR;
	}

	dprintf("heap_init_post_area(): created VIP heap: %p\n", sVIPHeap);

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


#endif	// !USE_SLAB_ALLOCATOR_FOR_MALLOC


status_t
heap_init_post_thread()
{
#if	!USE_SLAB_ALLOCATOR_FOR_MALLOC
	sHeapGrowThread = spawn_kernel_thread(heap_grow_thread, "heap grower",
		B_URGENT_PRIORITY, NULL);
	if (sHeapGrowThread < 0) {
		panic("heap_init_post_thread(): cannot create heap grow thread\n");
		return sHeapGrowThread;
	}

	// create per-cpu heaps if there's enough memory
	int32 heapCount = MIN(smp_get_num_cpus(),
		(int32)vm_page_num_pages() / 60 / 1024);
	for (int32 i = 1; i < heapCount; i++) {
		addr_t base = 0;
		size_t size = HEAP_GROW_SIZE * HEAP_CLASS_COUNT;
		area_id perCPUHeapArea = create_area("per cpu initial heap",
			(void **)&base, B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (perCPUHeapArea < 0)
			break;

		for (uint32 j = 0; j < HEAP_CLASS_COUNT; j++) {
			int32 heapIndex = i * HEAP_CLASS_COUNT + j;
			size_t partSize = size * sHeapClasses[j].initial_percentage / 100;
			sHeaps[heapIndex] = heap_create_allocator(sHeapClasses[j].name,
				base, partSize, &sHeapClasses[j], false);
			sLastGrowRequest[heapIndex] = 0;
			sLastHandledGrowRequest[heapIndex] = 0;
			base += partSize;
			sHeapCount++;
		}
	}

	resume_thread(sHeapGrowThread);

#else	// !USE_SLAB_ALLOCATOR_FOR_MALLOC

	// set up some debug commands
	add_debugger_command_etc("heap", &dump_heap_list,
		"Dump infos about a specific heap",
		"[\"stats\"] <heap>\n"
		"Dump infos about the specified kernel heap. If \"stats\" is given\n"
		"as the argument, currently only the heap count is printed.\n", 0);
#if !KERNEL_HEAP_LEAK_CHECK
	add_debugger_command_etc("heap_allocations", &dump_allocations,
		"Dump current heap allocations",
		"[\"stats\"] <heap>\n"
		"If the optional argument \"stats\" is specified, only the allocation\n"
		"counts and no individual allocations are printed.\n", 0);
#endif	// KERNEL_HEAP_LEAK_CHECK
#endif	// USE_SLAB_ALLOCATOR_FOR_MALLOC

	// run the deferred deleter roughly once a second
	if (register_kernel_daemon(deferred_deleter, NULL, 10) != B_OK)
		panic("heap_init_post_thread(): failed to init deferred deleter");

	return B_OK;
}


//	#pragma mark - Public API


#if !USE_SLAB_ALLOCATOR_FOR_MALLOC


void *
memalign(size_t alignment, size_t size)
{
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("memalign(): called with interrupts disabled\n");
		return NULL;
	}

	if (!gKernelStartup && size > HEAP_AREA_USE_THRESHOLD) {
		// don't even attempt such a huge allocation - use areas instead
		size_t areaSize = ROUNDUP(size + sizeof(area_allocation_info)
			+ alignment, B_PAGE_SIZE);
		if (areaSize < size) {
			// the size overflowed
			return NULL;
		}

		void *address = NULL;
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
		if (alignment != 0) {
			address = (void *)ROUNDUP((addr_t)address, alignment);
			ASSERT((addr_t)address % alignment == 0);
			ASSERT((addr_t)address + size - 1 < (addr_t)info + areaSize - 1);
		}

		TRACE(("heap: allocated area %ld for huge allocation of %lu bytes\n",
			allocationArea, size));

		info->allocation_base = address;

#if PARANOID_KERNEL_MALLOC
		memset(address, 0xcc, size);
#endif
		return address;
	}

	void *result = NULL;
	bool shouldGrow = false;
	int32 cpuCount = MIN(smp_get_num_cpus(),
		(int32)sHeapCount / HEAP_CLASS_COUNT);
	int32 cpuNumber = smp_get_current_cpu();
	for (int32 i = 0; i < cpuCount; i++) {
		uint32 heapIndex = heap_index_for(size, cpuNumber++ % cpuCount);
		heap_allocator *heap = sHeaps[heapIndex];
		result = heap_memalign(heap, alignment, size);
		if (result != NULL) {
			shouldGrow = heap_should_grow(heap);
			break;
		}

#if PARANOID_HEAP_VALIDATION
		heap_validate_heap(heap);
#endif
	}

	if (result == NULL) {
		// request an urgent grow and wait - we don't do it ourselfs here to
		// serialize growing through the grow thread, as otherwise multiple
		// threads hitting this situation (likely when memory ran out) would
		// all add areas
		uint32 heapIndex = heap_index_for(size, smp_get_current_cpu());
		sLastGrowRequest[heapIndex]++;
		switch_sem(sHeapGrowSem, sHeapGrownNotify);

		// and then try again
		result = heap_memalign(sHeaps[heapIndex], alignment, size);
	} else if (shouldGrow) {
		// should grow sometime soon, notify the grower
		release_sem_etc(sHeapGrowSem, 1, B_DO_NOT_RESCHEDULE);
	}

	if (result == NULL)
		panic("heap: kernel heap has run out of memory\n");
	return result;
}


void *
memalign_etc(size_t alignment, size_t size, uint32 flags)
{
	if ((flags & HEAP_PRIORITY_VIP) != 0)
		return heap_memalign(sVIPHeap, alignment, size);

	if ((flags & (HEAP_DONT_WAIT_FOR_MEMORY | HEAP_DONT_LOCK_KERNEL_SPACE))
			!= 0) {
		return memalign_nogrow(alignment, size);
	}

	return memalign(alignment, size);
}


void
free_etc(void *address, uint32 flags)
{
	if ((flags & HEAP_PRIORITY_VIP) != 0)
		heap_free(sVIPHeap, address);
	else
		free(address);
}


void *
malloc(size_t size)
{
	return memalign(0, size);
}


void
free(void *address)
{
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("free(): called with interrupts disabled\n");
		return;
	}

	int32 offset = smp_get_current_cpu() * HEAP_CLASS_COUNT;
	for (uint32 i = 0; i < sHeapCount; i++) {
		heap_allocator *heap = sHeaps[(i + offset) % sHeapCount];
		if (heap_free(heap, address) == B_OK) {
#if PARANOID_HEAP_VALIDATION
			heap_validate_heap(heap);
#endif
			return;
		}
	}

	// maybe it was allocated from the dedicated grow heap
	if (heap_free(sGrowHeap, address) == B_OK)
		return;

	// or maybe it was allocated from the VIP heap
	if (heap_free(sVIPHeap, address) == B_OK)
		return;

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
			TRACE(("free(): freed huge allocation by deleting area %ld\n",
				area));
			return;
		}
	}

	panic("free(): free failed for address %p\n", address);
}


void *
realloc(void *address, size_t newSize)
{
	if (!gKernelStartup && !are_interrupts_enabled()) {
		panic("realloc(): called with interrupts disabled\n");
		return NULL;
	}

	if (address == NULL)
		return memalign(0, newSize);

	if (newSize == 0) {
		free(address);
		return NULL;
	}

	void *newAddress = NULL;
	int32 offset = smp_get_current_cpu() * HEAP_CLASS_COUNT;
	for (uint32 i = 0; i < sHeapCount; i++) {
		heap_allocator *heap = sHeaps[(i + offset) % sHeapCount];
		if (heap_realloc(heap, address, &newAddress, newSize) == B_OK) {
#if PARANOID_HEAP_VALIDATION
			heap_validate_heap(heap);
#endif
			return newAddress;
		}
	}

	// maybe it was allocated from the dedicated grow heap
	if (heap_realloc(sGrowHeap, address, &newAddress, newSize) == B_OK)
		return newAddress;

	// or maybe it was a huge allocation using an area
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
				TRACE(("realloc(): new size %ld fits in old area %ld with %ld "
					"available\n", newSize, area, available));
				info->allocation_size = newSize;
				return address;
			}

			// have to allocate/copy/free - TODO maybe resize the area instead?
			newAddress = memalign(0, newSize);
			if (newAddress == NULL) {
				dprintf("realloc(): failed to allocate new block of %ld bytes\n",
					newSize);
				return NULL;
			}

			memcpy(newAddress, address, min_c(newSize, info->allocation_size));
			delete_area(area);
			TRACE(("realloc(): allocated new block %p for size %ld and deleted "
				"old area %ld\n", newAddress, newSize, area));
			return newAddress;
		}
	}

	panic("realloc(): failed to realloc address %p to size %lu\n", address,
		newSize);
	return NULL;
}


#endif	// !USE_SLAB_ALLOCATOR_FOR_MALLOC


void *
calloc(size_t numElements, size_t size)
{
	void *address = memalign(0, numElements * size);
	if (address != NULL)
		memset(address, 0, numElements * size);

	return address;
}


void
deferred_free(void *block)
{
	if (block == NULL)
		return;

	DeferredFreeListEntry *entry = new(block) DeferredFreeListEntry;

	InterruptsSpinLocker _(sDeferredFreeListLock);
	sDeferredFreeList.Add(entry);
}


void *
malloc_referenced(size_t size)
{
	int32 *referencedData = (int32 *)malloc(size + 4);
	if (referencedData == NULL)
		return NULL;

	*referencedData = 1;
	return referencedData + 1;
}


void *
malloc_referenced_acquire(void *data)
{
	if (data != NULL) {
		int32 *referencedData = (int32 *)data - 1;
		atomic_add(referencedData, 1);
	}

	return data;
}


void
malloc_referenced_release(void *data)
{
	if (data == NULL)
		return;

	int32 *referencedData = (int32 *)data - 1;
	if (atomic_add(referencedData, -1) < 1)
		free(referencedData);
}


DeferredDeletable::~DeferredDeletable()
{
}


void
deferred_delete(DeferredDeletable *deletable)
{
	if (deletable == NULL)
		return;

	InterruptsSpinLocker _(sDeferredFreeListLock);
	sDeferredDeletableList.Add(deletable);
}
