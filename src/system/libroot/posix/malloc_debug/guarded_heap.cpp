/*
 * Copyright 2011, Michael Lotz <mmlr@mlotz.ch>.
 * Distributed under the terms of the MIT License.
 */


#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <sys/mman.h>

#include <locks.h>


// #pragma mark - Debug Helpers


static bool sDebuggerCalls = true;


static void
panic(const char* format, ...)
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


// #pragma mark - Linked List


#define GET_ITEM(list, item) ((void *)((uint8 *)item - list->offset))
#define GET_LINK(list, item) ((list_link *)((uint8 *)item + list->offset))


struct list_link {
	list_link*	next;
	list_link*	prev;
};

struct list {
	list_link	link;
	int32		offset;
};


static inline void
list_remove_item(struct list* list, void* item)
{
	list_link* link = GET_LINK(list, item);

	link->next->prev = link->prev;
	link->prev->next = link->next;
}


static inline void
list_add_item(struct list* list, void* item)
{
	list_link* link = GET_LINK(list, item);

	link->next = &list->link;
	link->prev = list->link.prev;

	list->link.prev->next = link;
	list->link.prev = link;
}


static inline void*
list_get_next_item(struct list* list, void* item)
{
	if (item == NULL) {
		if (list->link.next == (list_link *)list)
			return NULL;

		return GET_ITEM(list, list->link.next);
	}

	list_link* link = GET_LINK(list, item);
	if (link->next == &list->link)
		return NULL;

	return GET_ITEM(list, link->next);
}


static inline void
list_init_etc(struct list* list, int32 offset)
{
	list->link.next = list->link.prev = &list->link;
	list->offset = offset;
}


// #pragma mark - Guarded Heap


#define GUARDED_HEAP_PAGE_FLAG_USED			0x01
#define GUARDED_HEAP_PAGE_FLAG_FIRST		0x02
#define GUARDED_HEAP_PAGE_FLAG_GUARD		0x04
#define GUARDED_HEAP_PAGE_FLAG_DEAD			0x08
#define GUARDED_HEAP_PAGE_FLAG_AREA			0x10

#define GUARDED_HEAP_INITIAL_SIZE			1 * 1024 * 1024
#define GUARDED_HEAP_GROW_SIZE				2 * 1024 * 1024
#define GUARDED_HEAP_AREA_USE_THRESHOLD		1 * 1024 * 1024


struct guarded_heap;

struct guarded_heap_page {
	uint8				flags;
	size_t				allocation_size;
	void*				allocation_base;
	size_t				alignment;
	thread_id			thread;
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
	mutex				lock;
	struct list			free_list;
	guarded_heap_page	pages[0];
};

struct guarded_heap {
	rw_lock				lock;
	size_t				page_count;
	size_t				used_pages;
	uint32				area_creation_counter;
	bool				reuse_memory;
	guarded_heap_area*	areas;
};


static guarded_heap sGuardedHeap = {
	RW_LOCK_INITIALIZER("guarded heap lock"),
	0, 0, 0, true, NULL
};


static void dump_guarded_heap_page(void* address, bool doPanic = false);


static void
guarded_heap_segfault_handler(int signal, siginfo_t* signalInfo, void* vregs)
{
	if (signal != SIGSEGV)
		return;

	if (signalInfo->si_code != SEGV_ACCERR) {
		// Not ours.
		panic("generic segfault");
		return;
	}

	dump_guarded_heap_page(signalInfo->si_addr, true);

	exit(-1);
}


static void
guarded_heap_page_protect(guarded_heap_area& area, size_t pageIndex,
	uint32 protection)
{
	addr_t address = area.base + pageIndex * B_PAGE_SIZE;
	mprotect((void*)address, B_PAGE_SIZE, protection);
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
			page.allocation_size = allocationSize;
			page.allocation_base = allocationBase;
			page.alignment = alignment;
			page.flags |= GUARDED_HEAP_PAGE_FLAG_FIRST;
			firstPage = &page;
		} else {
			page.thread = firstPage->thread;
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
				B_READ_AREA | B_WRITE_AREA);
		}
	}
}


static void
guarded_heap_free_page(guarded_heap_area& area, size_t pageIndex,
	bool force = false)
{
	guarded_heap_page& page = area.pages[pageIndex];

	if (area.heap->reuse_memory || force)
		page.flags = 0;
	else
		page.flags |= GUARDED_HEAP_PAGE_FLAG_DEAD;

	page.thread = find_thread(NULL);

	list_add_item(&area.free_list, &page);

	guarded_heap_page_protect(area, pageIndex, 0);
}


static void
guarded_heap_pages_allocated(guarded_heap& heap, size_t pagesAllocated)
{
	atomic_add((vint32*)&heap.used_pages, pagesAllocated);
}


static void*
guarded_heap_area_allocate(guarded_heap_area& area, size_t pagesNeeded,
	size_t size, size_t alignment)
{
	if (pagesNeeded > area.page_count - area.used_pages)
		return NULL;

	// We use the free list this way so that the page that has been free for
	// the longest time is allocated. This keeps immediate re-use (that may
	// hide bugs) to a minimum.
	guarded_heap_page* page
		= (guarded_heap_page*)list_get_next_item(&area.free_list, NULL);

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

		size_t offset = size & (B_PAGE_SIZE - 1);
		void* result = (void*)((area.base + pageIndex * B_PAGE_SIZE
			+ (offset > 0 ? B_PAGE_SIZE - offset : 0)) & ~(alignment - 1));

		guarded_heap_page_allocate(area, pageIndex, pagesNeeded, size,
			alignment, result);

		area.used_pages += pagesNeeded;
		guarded_heap_pages_allocated(*area.heap, pagesNeeded);
		return result;
	}

	return NULL;
}


static bool
guarded_heap_area_init(guarded_heap& heap, area_id id, void* baseAddress,
	size_t size)
{
	guarded_heap_area* area = (guarded_heap_area*)baseAddress;
	area->heap = &heap;
	area->area = id;
	area->size = size;
	area->page_count = area->size / B_PAGE_SIZE;
	area->used_pages = 0;

	size_t pagesNeeded = (sizeof(guarded_heap_area)
		+ area->page_count * sizeof(guarded_heap_page)
		+ B_PAGE_SIZE - 1) / B_PAGE_SIZE;

	area->page_count -= pagesNeeded;
	area->size = area->page_count * B_PAGE_SIZE;
	area->base = (addr_t)baseAddress + pagesNeeded * B_PAGE_SIZE;

	mutex_init(&area->lock, "guarded_heap_area_lock");

	list_init_etc(&area->free_list,
		offsetof(guarded_heap_page, free_list_link));

	for (size_t i = 0; i < area->page_count; i++)
		guarded_heap_free_page(*area, i, true);

	area->next = heap.areas;
	heap.areas = area;
	heap.page_count += area->page_count;

	return true;
}


static bool
guarded_heap_area_create(guarded_heap& heap, size_t size)
{
	for (size_t trySize = size; trySize >= 1 * 1024 * 1024;
		trySize /= 2) {

		void* baseAddress = NULL;
		area_id id = create_area("guarded_heap_area", &baseAddress,
			B_ANY_ADDRESS, trySize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

		if (id < 0)
			continue;

		if (guarded_heap_area_init(heap, id, baseAddress, trySize))
			return true;

		delete_area(id);
	}

	panic("failed to allocate a new heap area");
	return false;
}


static bool
guarded_heap_add_area(guarded_heap& heap, uint32 counter)
{
	WriteLocker areaListWriteLocker(heap.lock);
	if (heap.area_creation_counter != counter)
		return false;

	return guarded_heap_area_create(heap, GUARDED_HEAP_GROW_SIZE);
}


static void*
guarded_heap_allocate(guarded_heap& heap, size_t size, size_t alignment)
{
	if (alignment == 0)
		alignment = 1;

	if (alignment > B_PAGE_SIZE) {
		panic("alignment of %" B_PRIuSIZE " not supported", alignment);
		return NULL;
	}

	size_t pagesNeeded = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE + 1;
	if (pagesNeeded * B_PAGE_SIZE >= GUARDED_HEAP_AREA_USE_THRESHOLD) {
		// Don't bother, use an area directly. Since it will also fault once
		// it is deleted, that fits our model quite nicely.

		pagesNeeded = (size + sizeof(guarded_heap_page) + B_PAGE_SIZE - 1)
			/ B_PAGE_SIZE;

		void* address = NULL;
		area_id area = create_area("guarded_heap_huge_allocation", &address,
			B_ANY_ADDRESS, (pagesNeeded + 1) * B_PAGE_SIZE, B_NO_LOCK,
			B_READ_AREA | B_WRITE_AREA);
		if (area < 0) {
			panic("failed to create area for allocation of %" B_PRIuSIZE
				" pages", pagesNeeded);
			return NULL;
		}

		// We just use a page object
		guarded_heap_page* page = (guarded_heap_page*)address;
		page->flags = GUARDED_HEAP_PAGE_FLAG_USED
			| GUARDED_HEAP_PAGE_FLAG_FIRST | GUARDED_HEAP_PAGE_FLAG_AREA;
		page->allocation_size = size;
		page->allocation_base = (void*)(((addr_t)address
			+ pagesNeeded * B_PAGE_SIZE - size) & ~(alignment - 1));
		page->alignment = alignment;
		page->thread = find_thread(NULL);

		mprotect((void*)((addr_t)address + pagesNeeded * B_PAGE_SIZE),
			B_PAGE_SIZE, 0);

		return page->allocation_base;
	}

	void* result = NULL;

	ReadLocker areaListReadLocker(heap.lock);
	for (guarded_heap_area* area = heap.areas; area != NULL;
			area = area->next) {

		MutexLocker locker(area->lock);
		result = guarded_heap_area_allocate(*area, pagesNeeded, size,
			alignment);
		if (result != NULL)
			break;
	}

	uint32 counter = heap.area_creation_counter;
	areaListReadLocker.Unlock();

	if (result == NULL) {
		guarded_heap_add_area(heap, counter);
		return guarded_heap_allocate(heap, size, alignment);
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


static bool
guarded_heap_area_free(guarded_heap_area& area, void* address)
{
	size_t pageIndex = guarded_heap_area_page_index_for(area, address);
	if (pageIndex >= area.page_count)
		return false;

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

	if (area.heap->reuse_memory) {
		area.used_pages -= pagesFreed;
		atomic_add((vint32*)&area.heap->used_pages, -pagesFreed);
	}

	return true;
}


static guarded_heap_page*
guarded_heap_area_allocation_for(void* address, area_id& allocationArea)
{
	allocationArea = area_for(address);
	if (allocationArea < 0)
		return NULL;

	area_info areaInfo;
	if (get_area_info(allocationArea, &areaInfo) != B_OK)
		return NULL;

	guarded_heap_page* page = (guarded_heap_page*)areaInfo.address;
	if (page->flags != (GUARDED_HEAP_PAGE_FLAG_USED
			| GUARDED_HEAP_PAGE_FLAG_FIRST | GUARDED_HEAP_PAGE_FLAG_AREA)) {
		return NULL;
	}

	if (page->allocation_base != address)
		return NULL;
	if (page->allocation_size >= areaInfo.size)
		return NULL;

	return page;
}


static bool
guarded_heap_free_area_allocation(void* address)
{
	area_id allocationArea;
	if (guarded_heap_area_allocation_for(address, allocationArea) == NULL)
		return false;

	delete_area(allocationArea);
	return true;
}


static bool
guarded_heap_free(void* address)
{
	if (address == NULL)
		return true;

	guarded_heap_area* area = guarded_heap_get_locked_area_for(sGuardedHeap,
		address);
	if (area == NULL)
		return guarded_heap_free_area_allocation(address);

	MutexLocker locker(area->lock, true);
	return guarded_heap_area_free(*area, address);
}


static void*
guarded_heap_realloc(void* address, size_t newSize)
{
	guarded_heap_area* area = guarded_heap_get_locked_area_for(sGuardedHeap,
		address);

	size_t oldSize;
	area_id allocationArea = -1;
	if (area != NULL) {
		MutexLocker locker(area->lock, true);
		size_t pageIndex = guarded_heap_area_page_index_for(*area, address);
		if (pageIndex >= area->page_count)
			return NULL;

		guarded_heap_page& page = area->pages[pageIndex];
		oldSize = page.allocation_size;
		locker.Unlock();
	} else {
		guarded_heap_page* page = guarded_heap_area_allocation_for(address,
			allocationArea);
		if (page == NULL)
			return NULL;

		oldSize = page->allocation_size;
	}

	if (oldSize == newSize)
		return address;

	void* newBlock = guarded_heap_allocate(sGuardedHeap, newSize, 0);
	if (newBlock == NULL)
		return NULL;

	memcpy(newBlock, address, min_c(oldSize, newSize));

	if (allocationArea >= 0)
		delete_area(allocationArea);
	else {
		MutexLocker locker(area->lock);
		guarded_heap_area_free(*area, address);
	}

	return newBlock;
}


// #pragma mark - Debugger commands


static void
dump_guarded_heap_page(guarded_heap_page& page)
{
	printf("flags:");
	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_USED) != 0)
		printf(" used");
	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_FIRST) != 0)
		printf(" first");
	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_GUARD) != 0)
		printf(" guard");
	if ((page.flags & GUARDED_HEAP_PAGE_FLAG_DEAD) != 0)
		printf(" dead");
	printf("\n");

	printf("allocation size: %" B_PRIuSIZE "\n", page.allocation_size);
	printf("allocation base: %p\n", page.allocation_base);
	printf("alignment: %" B_PRIuSIZE "\n", page.alignment);
	printf("allocating thread: %" B_PRId32 "\n", page.thread);
}


static void
dump_guarded_heap_page(void* address, bool doPanic)
{
	// Find the area that contains this page.
	guarded_heap_area* area = NULL;
	for (guarded_heap_area* candidate = sGuardedHeap.areas; candidate != NULL;
			candidate = candidate->next) {

		if ((addr_t)address < candidate->base)
			continue;
		if ((addr_t)address >= candidate->base + candidate->size)
			continue;

		area = candidate;
		break;
	}

	if (area == NULL) {
		panic("didn't find area for address %p\n", address);
		return;
	}

	size_t pageIndex = ((addr_t)address - area->base) / B_PAGE_SIZE;
	guarded_heap_page& page = area->pages[pageIndex];
	dump_guarded_heap_page(page);

	if (doPanic) {
		// Note: we do this the complicated way because we absolutely don't
		// want any character conversion to happen that might provoke other
		// segfaults in the locale backend. Therefore we avoid using any string
		// formats, resulting in the mess below.

		#define DO_PANIC(state) \
			panic("thread %" B_PRId32 " tried accessing address %p which is " \
				state " (base: 0x%" B_PRIxADDR ", size: %" B_PRIuSIZE \
				", alignment: %" B_PRIuSIZE ", allocated by thread: %" \
				B_PRId32 ")", find_thread(NULL), address, \
				page.allocation_base, page.allocation_size, page.alignment, \
				page.thread)

		if ((page.flags & GUARDED_HEAP_PAGE_FLAG_USED) == 0)
			DO_PANIC("not allocated");
		else if ((page.flags & GUARDED_HEAP_PAGE_FLAG_GUARD) != 0)
			DO_PANIC("a guard page");
		else if ((page.flags & GUARDED_HEAP_PAGE_FLAG_DEAD) != 0)
			DO_PANIC("a dead page");
		else
			DO_PANIC("in an unknown state");

		#undef DO_PANIC
	}
}


static void
dump_guarded_heap_area(guarded_heap_area& area)
{
	printf("guarded heap area: %p\n", &area);
	printf("next heap area: %p\n", area.next);
	printf("guarded heap: %p\n", area.heap);
	printf("area id: %" B_PRId32 "\n", area.area);
	printf("base: 0x%" B_PRIxADDR "\n", area.base);
	printf("size: %" B_PRIuSIZE "\n", area.size);
	printf("page count: %" B_PRIuSIZE "\n", area.page_count);
	printf("used pages: %" B_PRIuSIZE "\n", area.used_pages);
	printf("lock: %p\n", &area.lock);

	size_t freeCount = 0;
	void* item = list_get_next_item(&area.free_list, NULL);
	while (item != NULL) {
		freeCount++;

		if ((((guarded_heap_page*)item)->flags & GUARDED_HEAP_PAGE_FLAG_USED)
				!= 0) {
			printf("free list broken, page %p not actually free\n", item);
		}

		item = list_get_next_item(&area.free_list, item);
	}

	printf("free_list: %p (%" B_PRIuSIZE " free)\n", &area.free_list,
		freeCount);

	freeCount = 0;
	size_t runLength = 0;
	size_t longestRun = 0;
	for (size_t i = 0; i <= area.page_count; i++) {
		guarded_heap_page& page = area.pages[i];
		if (i == area.page_count
			|| (page.flags & GUARDED_HEAP_PAGE_FLAG_USED) != 0) {
			freeCount += runLength;
			if (runLength > longestRun)
				longestRun = runLength;
			runLength = 0;
			continue;
		}

		runLength = 1;
		for (size_t j = 1; j < area.page_count - i; j++) {
			if ((area.pages[i + j].flags & GUARDED_HEAP_PAGE_FLAG_USED) != 0)
				break;

			runLength++;
		}

		i += runLength - 1;
	}

	printf("longest free run: %" B_PRIuSIZE " (%" B_PRIuSIZE " free)\n",
		longestRun, freeCount);

	printf("pages: %p\n", area.pages);
}


static void
dump_guarded_heap(guarded_heap& heap)
{
	printf("guarded heap: %p\n", &heap);
	printf("rw lock: %p\n", &heap.lock);
	printf("page count: %" B_PRIuSIZE "\n", heap.page_count);
	printf("used pages: %" B_PRIuSIZE "\n", heap.used_pages);
	printf("area creation counter: %" B_PRIu32 "\n",
		heap.area_creation_counter);

	size_t areaCount = 0;
	guarded_heap_area* area = heap.areas;
	while (area != NULL) {
		areaCount++;
		area = area->next;
	}

	printf("areas: %p (%" B_PRIuSIZE ")\n", heap.areas, areaCount);
}


// #pragma mark - Heap Debug API


extern "C" status_t
heap_debug_start_wall_checking(int msInterval)
{
	return B_NOT_SUPPORTED;
}


extern "C" status_t
heap_debug_stop_wall_checking()
{
	return B_NOT_SUPPORTED;
}


extern "C" void
heap_debug_set_paranoid_validation(bool enabled)
{
}


extern "C" void
heap_debug_set_memory_reuse(bool enabled)
{
	sGuardedHeap.reuse_memory = enabled;
}


extern "C" void
heap_debug_set_debugger_calls(bool enabled)
{
	sDebuggerCalls = enabled;
}


extern "C" void
heap_debug_validate_heaps()
{
}


extern "C" void
heap_debug_validate_walls()
{
}


extern "C" void
heap_debug_dump_allocations(bool statsOnly, thread_id thread)
{
}


extern "C" void
heap_debug_dump_heaps(bool dumpAreas, bool dumpBins)
{
	WriteLocker heapLocker(sGuardedHeap.lock);
	dump_guarded_heap(sGuardedHeap);
	if (!dumpAreas)
		return;

	for (guarded_heap_area* area = sGuardedHeap.areas; area != NULL;
			area = area->next) {
		MutexLocker areaLocker(area->lock);
		dump_guarded_heap_area(*area);

		if (!dumpBins)
			continue;

		for (size_t i = 0; i < area->page_count; i++)
			dump_guarded_heap_page(area->pages[i]);
	}
}


extern "C" void *
heap_debug_malloc_with_guard_page(size_t size)
{
	return malloc(size);
}


extern "C" status_t
heap_debug_get_allocation_info(void *address, size_t *size,
	thread_id *thread)
{
	return B_NOT_SUPPORTED;
}


// #pragma mark - Init


static void
init_after_fork()
{
	// The memory has actually been copied (or is in a copy on write state) but
	// but the area ids have changed.
	for (guarded_heap_area* area = sGuardedHeap.areas; area != NULL;
			area = area->next) {
		area->area = area_for(area);
		if (area->area < 0)
			panic("failed to find area for heap area %p after fork", area);
	}
}


extern "C" status_t
__init_heap(void)
{
	if (!guarded_heap_area_create(sGuardedHeap, GUARDED_HEAP_INITIAL_SIZE))
		return B_ERROR;

	// Install a segfault handler so we can print some info before going down.
	struct sigaction action;
	action.sa_handler = (__sighandler_t)guarded_heap_segfault_handler;
	action.sa_flags = SA_SIGINFO;
	action.sa_userdata = NULL;
	sigemptyset(&action.sa_mask);
	sigaction(SIGSEGV, &action, NULL);

	atfork(&init_after_fork);
		// Note: Needs malloc(). Hence we need to be fully initialized.
		// TODO: We should actually also install a hook that is called before
		// fork() is being executed. In a multithreaded app it would need to
		// acquire *all* allocator locks, so that we don't fork() an
		// inconsistent state.

	return B_OK;
}


extern "C" void
__init_heap_post_env(void)
{
	const char *mode = getenv("MALLOC_DEBUG");
	if (mode != NULL) {
		if (strchr(mode, 'r'))
			heap_debug_set_memory_reuse(false);
	}
}


// #pragma mark - Public API


extern "C" void*
sbrk_hook(long)
{
	debug_printf("sbrk not supported on malloc debug\n");
	panic("sbrk not supported on malloc debug\n");
	return NULL;
}


extern "C" void*
memalign(size_t alignment, size_t size)
{
	if (size == 0)
		size = 1;

	return guarded_heap_allocate(sGuardedHeap, size, alignment);
}


extern "C" void*
malloc(size_t size)
{
	return memalign(0, size);
}


extern "C" void
free(void* address)
{
	if (!guarded_heap_free(address))
		panic("free failed for address %p", address);
}


extern "C" void*
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


extern "C" void*
calloc(size_t numElements, size_t size)
{
	void* address = malloc(numElements * size);
	if (address != NULL)
		memset(address, 0, numElements * size);

	return address;
}


extern "C" void*
valloc(size_t size)
{
	return memalign(B_PAGE_SIZE, size);
}


extern "C" int
posix_memalign(void **pointer, size_t alignment, size_t size)
{
	// this cryptic line accepts zero and all powers of two
	if (((~alignment + 1) | ((alignment << 1) - 1)) != ~0UL)
		return EINVAL;

	*pointer = memalign(alignment, size);
	if (*pointer == NULL)
		return ENOMEM;

	return 0;
}
