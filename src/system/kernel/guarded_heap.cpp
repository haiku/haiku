/*
 * Copyright 2011, Michael Lotz <mmlr@mlotz.ch>.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>

#include <arch/debug.h>
#include <elf.h>
#include <debug.h>
#include <heap.h>
#include <malloc.h>
#include <slab/Slab.h>
#include <tracing.h>
#include <util/list.h>
#include <util/AutoLock.h>
#include <vm/vm.h>


#if USE_GUARDED_HEAP_FOR_MALLOC


#define GUARDED_HEAP_PAGE_FLAG_USED		0x01
#define GUARDED_HEAP_PAGE_FLAG_FIRST	0x02
#define GUARDED_HEAP_PAGE_FLAG_GUARD	0x04
#define GUARDED_HEAP_PAGE_FLAG_DEAD		0x08

#define GUARDED_HEAP_STACK_TRACE_DEPTH	0


struct guarded_heap;

struct guarded_heap_page {
	uint8				flags;
	size_t				allocation_size;
	void*				allocation_base;
	size_t				alignment;
	thread_id			thread;
#if GUARDED_HEAP_STACK_TRACE_DEPTH > 0
	size_t				stack_trace_depth;
	addr_t				stack_trace[GUARDED_HEAP_STACK_TRACE_DEPTH];
#endif
	list_link			free_list_link;
};

struct guarded_heap_area {
	guarded_heap*		heap;
	guarded_heap_area*	next;
	area_id				area;
	addr_t				base;
	size_t				size;
	size_t				page_count;
	size_t				used_pages;
	void*				protection_cookie;
	mutex				lock;
	struct list			free_list;
	guarded_heap_page	pages[0];
};

struct guarded_heap {
	rw_lock				lock;
	size_t				page_count;
	size_t				used_pages;
	vint32				area_creation_counter;
	guarded_heap_area*	areas;
};


static guarded_heap sGuardedHeap = {
	RW_LOCK_INITIALIZER("guarded heap lock"),
	0, 0, 0, NULL
};


#if GUARDED_HEAP_TRACING

namespace GuardedHeapTracing {


class GuardedHeapTraceEntry
	: public TRACE_ENTRY_SELECTOR(GUARDED_HEAP_TRACING_STACK_TRACE) {
	public:
		GuardedHeapTraceEntry(guarded_heap* heap)
			:
			TraceEntryBase(GUARDED_HEAP_TRACING_STACK_TRACE, 0, true),
			fHeap(heap)
		{
		}

	protected:
		guarded_heap*	fHeap;
};


class Allocate : public GuardedHeapTraceEntry {
	public:
		Allocate(guarded_heap* heap, void* pageBase, uint32 flags)
			:
			GuardedHeapTraceEntry(heap),
			fPageBase(pageBase),
			fFlags(flags)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("guarded heap allocate: heap: %p; page: %p; "
				"flags:%s%s%s%s", fHeap, fPageBase,
				(fFlags & GUARDED_HEAP_PAGE_FLAG_USED) != 0 ? " used" : "",
				(fFlags & GUARDED_HEAP_PAGE_FLAG_FIRST) != 0 ? " first" : "",
				(fFlags & GUARDED_HEAP_PAGE_FLAG_GUARD) != 0 ? " guard" : "",
				(fFlags & GUARDED_HEAP_PAGE_FLAG_DEAD) != 0 ? " dead" : "");
		}

	private:
		void*		fPageBase;
		uint32		fFlags;
};


class Free : public GuardedHeapTraceEntry {
	public:
		Free(guarded_heap* heap, void* pageBase)
			:
			GuardedHeapTraceEntry(heap),
			fPageBase(pageBase)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("guarded heap free: heap: %p; page: %p", fHeap,
				fPageBase);
		}

	private:
		void*		fPageBase;
};


}	// namespace GuardedHeapTracing

#	define T(x)	new(std::nothrow) GuardedHeapTracing::x
#else
#	define T(x)
#endif	// GUARDED_HEAP_TRACING


static void
guarded_heap_page_protect(guarded_heap_area& area, size_t pageIndex,
	uint32 protection)
{
	if (area.area < 0)
		return;

	addr_t address = area.base + pageIndex * B_PAGE_SIZE;
	vm_set_kernel_area_debug_protection(area.protection_cookie, (void*)address,
		B_PAGE_SIZE, protection);
}


static void
guarded_heap_page_allocate(guarded_heap_area& area, size_t startPageIndex,
	size_t pagesNeeded, size_t allocationSize, size_t alignment,
	void* allocationBase)
{
	if (pagesNeeded < 2) {
		panic("need to allocate at least 2 pages, one for guard\n");
		return;
	}

	guarded_heap_page* firstPage = NULL;
	for (size_t i = 0; i < pagesNeeded; i++) {
		guarded_heap_page& page = area.pages[startPageIndex + i];
		page.flags = GUARDED_HEAP_PAGE_FLAG_USED;
		if (i == 0) {
			page.thread = find_thread(NULL);
#if GUARDED_HEAP_STACK_TRACE_DEPTH > 0
			page.stack_trace_depth = arch_debug_get_stack_trace(
				page.stack_trace, GUARDED_HEAP_STACK_TRACE_DEPTH, 0, 4,
				STACK_TRACE_KERNEL);
#endif
			page.allocation_size = allocationSize;
			page.allocation_base = allocationBase;
			page.alignment = alignment;
			page.flags |= GUARDED_HEAP_PAGE_FLAG_FIRST;
			firstPage = &page;
		} else {
			page.thread = firstPage->thread;
#if GUARDED_HEAP_STACK_TRACE_DEPTH > 0
			page.stack_trace_depth = 0;
#endif
			page.allocation_size = allocationSize;
			page.allocation_base = allocationBase;
			page.alignment = alignment;
		}

		list_remove_item(&area.free_list, &page);

		if (i == pagesNeeded - 1) {
			page.flags |= GUARDED_HEAP_PAGE_FLAG_GUARD;
			guarded_heap_page_protect(area, startPageIndex + i, 0);
		} else {
			guarded_heap_page_protect(area, startPageIndex + i,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}

		T(Allocate(area.heap,
			(void*)(area.base + (startPageIndex + i) * B_PAGE_SIZE),
			page.flags));
	}
}


static void
guarded_heap_free_page(guarded_heap_area& area, size_t pageIndex,
	bool force = false)
{
	guarded_heap_page& page = area.pages[pageIndex];

#if DEBUG_GUARDED_HEAP_DISABLE_MEMORY_REUSE
	if (force || area.area < 0)
		page.flags = 0;
	else
		page.flags |= GUARDED_HEAP_PAGE_FLAG_DEAD;
#else
	page.flags = 0;
#endif

	page.allocation_size = 0;
	page.thread = find_thread(NULL);

#if GUARDED_HEAP_STACK_TRACE_DEPTH > 0
	page.stack_trace_depth = arch_debug_get_stack_trace(page.stack_trace,
		GUARDED_HEAP_STACK_TRACE_DEPTH, 0, 3, STACK_TRACE_KERNEL);
#endif

	list_add_item(&area.free_list, &page);

	guarded_heap_page_protect(area, pageIndex, 0);

	T(Free(area.heap, (void*)(area.base + pageIndex * B_PAGE_SIZE)));
}


static bool
guarded_heap_pages_allocated(guarded_heap& heap, size_t pagesAllocated)
{
	return (atomic_add((vint32*)&heap.used_pages, pagesAllocated)
			+ pagesAllocated)
		>= heap.page_count - HEAP_GROW_SIZE / B_PAGE_SIZE / 2;
}


static void*
guarded_heap_area_allocate(guarded_heap_area& area, size_t size,
	size_t alignment, uint32 flags, bool& grow)
{
	if (alignment > B_PAGE_SIZE) {
		panic("alignment of %" B_PRIuSIZE " not supported", alignment);
		return NULL;
	}

	size_t pagesNeeded = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE + 1;
	if (pagesNeeded > area.page_count - area.used_pages)
		return NULL;

	if (pagesNeeded > area.page_count)
		return NULL;

	// We use the free list this way so that the page that has been free for
	// the longest time is allocated. This keeps immediate re-use (that may
	// hide bugs) to a minimum.
	guarded_heap_page* page
		= (guarded_heap_page*)list_get_first_item(&area.free_list);

	for (; page != NULL;
		page = (guarded_heap_page*)list_get_next_item(&area.free_list, page)) {

		if ((page->flags & GUARDED_HEAP_PAGE_FLAG_USED) != 0)
			continue;

		size_t pageIndex = page - area.pages;
		if (pageIndex > area.page_count - pagesNeeded)
			continue;

		// Candidate, check if we have enough pages going forward
		// (including the guard page).
		bool candidate = true;
		for (size_t j = 1; j < pagesNeeded; j++) {
			if ((area.pages[pageIndex + j].flags & GUARDED_HEAP_PAGE_FLAG_USED)
					!= 0) {
				candidate = false;
				break;
			}
		}

		if (!candidate)
			continue;

		if (alignment == 0)
			alignment = 1;

		size_t offset = size & (B_PAGE_SIZE - 1);
		void* result = (void*)((area.base + pageIndex * B_PAGE_SIZE
			+ (offset > 0 ? B_PAGE_SIZE - offset : 0)) & ~(alignment - 1));

		guarded_heap_page_allocate(area, pageIndex, pagesNeeded, size,
			alignment, result);

		area.used_pages += pagesNeeded;
		grow = guarded_heap_pages_allocated(*area.heap, pagesNeeded);
		return result;
	}

	return NULL;
}


static bool
guarded_heap_area_init(guarded_heap& heap, area_id id, void* baseAddress,
	size_t size, uint32 flags)
{
	guarded_heap_area* area = (guarded_heap_area*)baseAddress;
	area->heap = &heap;
	area->area = id;
	area->size = size;
	area->page_count = area->size / B_PAGE_SIZE;
	area->used_pages = 0;

	size_t pagesNeeded = (sizeof(guarded_heap_area)
		+ area->page_count * sizeof(guarded_heap_page)) / B_PAGE_SIZE;

	area->page_count -= pagesNeeded;
	area->size = area->page_count * B_PAGE_SIZE;
	area->base = (addr_t)baseAddress + pagesNeeded * B_PAGE_SIZE;

	if (area->area >= 0 && vm_prepare_kernel_area_debug_protection(area->area,
			&area->protection_cookie) != B_OK) {
		return false;
	}

	mutex_init(&area->lock, "guarded_heap_area_lock");

	list_init_etc(&area->free_list,
		offsetof(guarded_heap_page, free_list_link));

	for (size_t i = 0; i < area->page_count; i++)
		guarded_heap_free_page(*area, i, true);

	WriteLocker areaListWriteLocker(heap.lock);
	area->next = heap.areas;
	heap.areas = area;
	heap.page_count += area->page_count;

	return true;
}


static bool
guarded_heap_area_create(guarded_heap& heap, uint32 flags)
{
	for (size_t trySize = HEAP_GROW_SIZE; trySize >= 1 * 1024 * 1024;
		trySize /= 2) {

		void* baseAddress = NULL;
		area_id id = create_area("guarded_heap_area", &baseAddress,
			B_ANY_KERNEL_ADDRESS, trySize, B_FULL_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

		if (id < 0)
			continue;

		if (guarded_heap_area_init(heap, id, baseAddress, trySize, flags))
			return true;

		delete_area(id);
	}

	panic("failed to allocate a new heap area");
	return false;
}


static bool
guarded_heap_add_area(guarded_heap& heap, int32 counter, uint32 flags)
{
	if ((flags & (HEAP_DONT_LOCK_KERNEL_SPACE | HEAP_DONT_WAIT_FOR_MEMORY))
			!= 0) {
		return false;
	}

	if (atomic_test_and_set((vint32*)&heap.area_creation_counter,
			counter + 1, counter) == counter) {
		return guarded_heap_area_create(heap, flags);
	}

	return false;
}


static void*
guarded_heap_allocate(guarded_heap& heap, size_t size, size_t alignment,
	uint32 flags)
{
	bool grow = false;
	void* result = NULL;
	ReadLocker areaListReadLocker(heap.lock);
	for (guarded_heap_area* area = heap.areas; area != NULL;
			area = area->next) {

		MutexLocker locker(area->lock);
		result = guarded_heap_area_allocate(*area, size, alignment, flags,
			grow);
		if (result != NULL)
			break;
	}

	int32 counter = atomic_get(&heap.area_creation_counter);
	areaListReadLocker.Unlock();

	if (result == NULL || grow) {
		bool added = guarded_heap_add_area(heap, counter, flags);
		if (result == NULL && added)
			return guarded_heap_allocate(heap, size, alignment, flags);
	}

	if (result == NULL)
		panic("ran out of memory");

	return result;
}


static guarded_heap_area*
guarded_heap_get_locked_area_for(guarded_heap& heap, void* address)
{
	ReadLocker areaListReadLocker(heap.lock);
	for (guarded_heap_area* area = heap.areas; area != NULL;
			area = area->next) {
		if ((addr_t)address < area->base)
			continue;

		if ((addr_t)address >= area->base + area->size)
			continue;

		mutex_lock(&area->lock);
		return area;
	}

	panic("guarded heap area for address %p not found", address);
	return NULL;
}


static size_t
guarded_heap_area_page_index_for(guarded_heap_area& area, void* address)
{
	size_t pageIndex = ((addr_t)address - area.base) / B_PAGE_SIZE;
	guarded_heap_page& page = area.pages[pageIndex];
	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_USED) == 0) {
		panic("tried to free %p which points at page %" B_PRIuSIZE
			" which is not marked in use", address, pageIndex);
		return area.page_count;
	}

	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_GUARD) != 0) {
		panic("tried to free %p which points at page %" B_PRIuSIZE
			" which is a guard page", address, pageIndex);
		return area.page_count;
	}

	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_FIRST) == 0) {
		panic("tried to free %p which points at page %" B_PRIuSIZE
			" which is not an allocation first page", address, pageIndex);
		return area.page_count;
	}

	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_DEAD) != 0) {
		panic("tried to free %p which points at page %" B_PRIuSIZE
			" which is a dead page", address, pageIndex);
		return area.page_count;
	}

	return pageIndex;
}


static void
guarded_heap_area_free(guarded_heap_area& area, void* address, uint32 flags)
{
	size_t pageIndex = guarded_heap_area_page_index_for(area, address);
	if (pageIndex >= area.page_count)
		return;

	size_t pagesFreed = 0;
	guarded_heap_page* page = &area.pages[pageIndex];
	while ((page->flags & GUARDED_HEAP_PAGE_FLAG_GUARD) == 0) {
		// Mark the allocation page as free.
		guarded_heap_free_page(area, pageIndex);

		pagesFreed++;
		pageIndex++;
		page = &area.pages[pageIndex];
	}

	// Mark the guard page as free as well.
	guarded_heap_free_page(area, pageIndex);
	pagesFreed++;

#if !DEBUG_GUARDED_HEAP_DISABLE_MEMORY_REUSE
	area.used_pages -= pagesFreed;
	atomic_add((vint32*)&area.heap->used_pages, -pagesFreed);
#endif
}


static void
guarded_heap_free(void* address, uint32 flags)
{
	if (address == NULL)
		return;

	guarded_heap_area* area = guarded_heap_get_locked_area_for(sGuardedHeap,
		address);
	if (area == NULL)
		return;

	MutexLocker locker(area->lock, true);
	guarded_heap_area_free(*area, address, flags);
}


static void*
guarded_heap_realloc(void* address, size_t newSize)
{
	guarded_heap_area* area = guarded_heap_get_locked_area_for(sGuardedHeap,
		address);
	if (area == NULL)
		return NULL;

	MutexLocker locker(area->lock, true);

	size_t pageIndex = guarded_heap_area_page_index_for(*area, address);
	if (pageIndex >= area->page_count)
		return NULL;

	guarded_heap_page& page = area->pages[pageIndex];
	size_t oldSize = page.allocation_size;
	locker.Unlock();

	if (oldSize == newSize)
		return address;

	void* newBlock = memalign(0, newSize);
	if (newBlock == NULL)
		return NULL;

	memcpy(newBlock, address, min_c(oldSize, newSize));

	free(address);

	return newBlock;
}


// #pragma mark - Debugger commands


static int
dump_guarded_heap_page(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	addr_t address = parse_expression(argv[1]);

	// Find the area that contains this page.
	guarded_heap_area* area = NULL;
	for (guarded_heap_area* candidate = sGuardedHeap.areas; candidate != NULL;
			candidate = candidate->next) {

		if (address < candidate->base)
			continue;
		if (address >= candidate->base + candidate->size)
			continue;

		area = candidate;
		break;
	}

	if (area == NULL) {
		kprintf("didn't find area for address\n");
		return 1;
	}

	size_t pageIndex = ((addr_t)address - area->base) / B_PAGE_SIZE;
	guarded_heap_page& page = area->pages[pageIndex];

	kprintf("page index: %" B_PRIuSIZE "\n", pageIndex);
	kprintf("flags:");
	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_USED) != 0)
		kprintf(" used");
	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_FIRST) != 0)
		kprintf(" first");
	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_GUARD) != 0)
		kprintf(" guard");
	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_DEAD) != 0)
		kprintf(" dead");
	kprintf("\n");

	kprintf("allocation size: %" B_PRIuSIZE "\n", page.allocation_size);
	kprintf("allocation base: %p\n", page.allocation_base);
	kprintf("alignment: %" B_PRIuSIZE "\n", page.alignment);
	kprintf("allocating thread: %" B_PRId32 "\n", page.thread);

#if GUARDED_HEAP_STACK_TRACE_DEPTH > 0
	kprintf("stack trace:\n");
	for (size_t i = 0; i < page.stack_trace_depth; i++) {
		addr_t address = page.stack_trace[i];

		const char* symbol;
		const char* imageName;
		bool exactMatch;
		addr_t baseAddress;

		if (elf_debug_lookup_symbol_address(address, &baseAddress, &symbol,
				&imageName, &exactMatch) == B_OK) {
			kprintf("  %p  %s + 0x%lx (%s)%s\n", (void*)address, symbol,
				address - baseAddress, imageName,
				exactMatch ? "" : " (nearest)");
		} else
			kprintf("  %p\n", (void*)address);
	}
#endif

	return 0;
}


static int
dump_guarded_heap_area(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	addr_t address = parse_expression(argv[1]);

	// Find the area that contains this page.
	guarded_heap_area* area = NULL;
	for (guarded_heap_area* candidate = sGuardedHeap.areas; candidate != NULL;
			candidate = candidate->next) {

		if ((addr_t)candidate != address) {
			if (address < candidate->base)
				continue;
			if (address >= candidate->base + candidate->size)
				continue;
		}

		area = candidate;
		break;
	}

	if (area == NULL) {
		kprintf("didn't find area for address\n");
		return 1;
	}

	kprintf("guarded heap area: %p\n", area);
	kprintf("next heap area: %p\n", area->next);
	kprintf("guarded heap: %p\n", area->heap);
	kprintf("area id: %" B_PRId32 "\n", area->area);
	kprintf("base: 0x%" B_PRIxADDR "\n", area->base);
	kprintf("size: %" B_PRIuSIZE "\n", area->size);
	kprintf("page count: %" B_PRIuSIZE "\n", area->page_count);
	kprintf("used pages: %" B_PRIuSIZE "\n", area->used_pages);
	kprintf("protection cookie: %p\n", area->protection_cookie);
	kprintf("lock: %p\n", &area->lock);

	size_t freeCount = 0;
	void* item = list_get_first_item(&area->free_list);
	while (item != NULL) {
		freeCount++;

		if ((((guarded_heap_page*)item)->flags & GUARDED_HEAP_PAGE_FLAG_USED)
				!= 0) {
			kprintf("free list broken, page %p not actually free\n", item);
		}

		item = list_get_next_item(&area->free_list, item);
	}

	kprintf("free_list: %p (%" B_PRIuSIZE " free)\n", &area->free_list,
		freeCount);

	freeCount = 0;
	size_t runLength = 0;
	size_t longestRun = 0;
	for (size_t i = 0; i <= area->page_count; i++) {
		guarded_heap_page& page = area->pages[i];
		if (i == area->page_count
			|| (page.flags & GUARDED_HEAP_PAGE_FLAG_USED) != 0) {
			freeCount += runLength;
			if (runLength > longestRun)
				longestRun = runLength;
			runLength = 0;
			continue;
		}

		runLength = 1;
		for (size_t j = 1; j < area->page_count - i; j++) {
			if ((area->pages[i + j].flags & GUARDED_HEAP_PAGE_FLAG_USED) != 0)
				break;

			runLength++;
		}

		i += runLength - 1;
	}

	kprintf("longest free run: %" B_PRIuSIZE " (%" B_PRIuSIZE " free)\n",
		longestRun, freeCount);

	kprintf("pages: %p\n", area->pages);

	return 0;
}


static int
dump_guarded_heap(int argc, char** argv)
{
	guarded_heap* heap = &sGuardedHeap;
	if (argc != 1) {
		if (argc == 2)
			heap = (guarded_heap*)parse_expression(argv[1]);
		else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	kprintf("guarded heap: %p\n", heap);
	kprintf("rw lock: %p\n", &heap->lock);
	kprintf("page count: %" B_PRIuSIZE "\n", heap->page_count);
	kprintf("used pages: %" B_PRIuSIZE "\n", heap->used_pages);
	kprintf("area creation counter: %" B_PRId32 "\n",
		(int32)heap->area_creation_counter);

	size_t areaCount = 0;
	guarded_heap_area* area = heap->areas;
	while (area != NULL) {
		areaCount++;
		area = area->next;
	}

	kprintf("areas: %p (%" B_PRIuSIZE ")\n", heap->areas, areaCount);

	return 0;
}


// #pragma mark - Malloc API


status_t
heap_init(addr_t address, size_t size)
{
	return guarded_heap_area_init(sGuardedHeap, -1, (void*)address, size, 0)
		? B_OK : B_ERROR;
}


status_t
heap_init_post_area()
{
	return B_OK;
}


status_t
heap_init_post_sem()
{
	for (guarded_heap_area* area = sGuardedHeap.areas; area != NULL;
			area = area->next) {
		if (area->area >= 0)
			continue;

		area_id id = area_for((void*)area->base);
		if (id < 0 || vm_prepare_kernel_area_debug_protection(id,
				&area->protection_cookie) != B_OK) {
			panic("failed to prepare initial guarded heap for protection");
			continue;
		}

		area->area = id;
		for (size_t i = 0; i < area->page_count; i++) {
			guarded_heap_page& page = area->pages[i];
			if ((page.flags & GUARDED_HEAP_PAGE_FLAG_USED) != 0
				&& (page.flags & GUARDED_HEAP_PAGE_FLAG_GUARD) == 0
				&& (page.flags & GUARDED_HEAP_PAGE_FLAG_DEAD) == 0) {
				guarded_heap_page_protect(*area, i,
					B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
			} else
				guarded_heap_page_protect(*area, i, 0);
		}
	}

	add_debugger_command("guarded_heap", &dump_guarded_heap,
		"Dump info about the guarded heap");
	add_debugger_command_etc("guarded_heap_area", &dump_guarded_heap_area,
		"Dump info about a guarded heap area",
		"<address>\nDump info about guarded heap area containing address.\n",
		0);
	add_debugger_command_etc("guarded_heap_page", &dump_guarded_heap_page,
		"Dump info about a guarded heap page",
		"<address>\nDump info about guarded heap page containing address.\n",
		0);

	return B_OK;
}


void*
memalign(size_t alignment, size_t size)
{
	return memalign_etc(alignment, size, 0);
}


void *
memalign_etc(size_t alignment, size_t size, uint32 flags)
{
	if (size == 0)
		size = 1;

	return guarded_heap_allocate(sGuardedHeap, size, alignment, flags);
}


void
free_etc(void *address, uint32 flags)
{
	guarded_heap_free(address, flags);
}


void*
malloc(size_t size)
{
	return memalign_etc(0, size, 0);
}


void
free(void* address)
{
	free_etc(address, 0);
}


void*
realloc(void* address, size_t newSize)
{
	if (newSize == 0) {
		free(address);
		return NULL;
	}

	if (address == NULL)
		return memalign(0, newSize);

	return guarded_heap_realloc(address, newSize);
}


#if USE_GUARDED_HEAP_FOR_OBJECT_CACHE


// #pragma mark - Slab API


void
request_memory_manager_maintenance()
{
}


object_cache*
create_object_cache(const char*, size_t objectSize, size_t, void*,
	object_cache_constructor, object_cache_destructor)
{
	return (object_cache*)objectSize;
}


object_cache*
create_object_cache_etc(const char*, size_t objectSize, size_t, size_t, size_t,
	size_t, uint32, void*, object_cache_constructor, object_cache_destructor,
	object_cache_reclaimer)
{
	return (object_cache*)objectSize;
}


void
delete_object_cache(object_cache* cache)
{
}


status_t
object_cache_set_minimum_reserve(object_cache* cache, size_t objectCount)
{
	return B_OK;
}


void*
object_cache_alloc(object_cache* cache, uint32 flags)
{
	return memalign_etc(0, (size_t)cache, flags);
}


void
object_cache_free(object_cache* cache, void* object, uint32 flags)
{
	return free_etc(object, flags);
}


status_t
object_cache_reserve(object_cache* cache, size_t objectCount, uint32 flags)
{
	return B_OK;
}


void
object_cache_get_usage(object_cache* cache, size_t* _allocatedMemory)
{
	*_allocatedMemory = 0;
}


void
slab_init(kernel_args* args)
{
}


void
slab_init_post_area()
{
}


void
slab_init_post_sem()
{
}


void
slab_init_post_thread()
{
}


#endif	// USE_GUARDED_HEAP_FOR_OBJECT_CACHE


#endif	// USE_GUARDED_HEAP_FOR_MALLOC
