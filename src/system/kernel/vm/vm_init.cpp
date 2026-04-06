/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Copyright 2010-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <vm/vm_page.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>
#include <vm/VMCache.h>


//#define TRACE_VM_INIT
#ifdef TRACE_VM_INIT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/*!	Frees physical pages that were used during the boot process.
	\a end is inclusive.
*/
static void
unmap_and_free_physical_pages(VMTranslationMap* map, addr_t start, addr_t end)
{
	// free all physical pages in the specified range

	vm_page_reservation reservation = {};
	for (addr_t current = start; current < end; current += B_PAGE_SIZE) {
		phys_addr_t physicalAddress;
		uint32 flags;

		if (map->Query(current, &physicalAddress, &flags) == B_OK
				&& (flags & PAGE_PRESENT) != 0) {
			vm_page* page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
			if (page != NULL && page->State() != PAGE_STATE_FREE
					&& page->State() != PAGE_STATE_CLEAR
					&& page->State() != PAGE_STATE_UNUSED) {
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_free_etc(NULL, page, &reservation);
			}
		}
	}

	// unmap the memory
	map->Unmap(start, end);

	// unreserve the memory
	vm_unreserve_memory(reservation.count * B_PAGE_SIZE);
	vm_page_unreserve_pages(&reservation);
}


void
vm_free_unused_boot_loader_range(addr_t start, addr_t size)
{
	VMTranslationMap* map = VMAddressSpace::Kernel()->TranslationMap();
	addr_t end = start + (size - 1);
	addr_t lastEnd = start;

	TRACE(("vm_free_unused_boot_loader_range(): asked to free %p - %p\n",
		(void*)start, (void*)end));

	// The areas are sorted in virtual address space order, so
	// we just have to find the holes between them that fall
	// into the area we should dispose

	map->Lock();

	for (VMAddressSpace::AreaIterator it
				= VMAddressSpace::Kernel()->GetAreaIterator();
			VMArea* area = it.Next();) {
		addr_t areaStart = area->Base();
		addr_t areaEnd = areaStart + (area->Size() - 1);

		if (areaEnd < start)
			continue;

		if (areaStart > end) {
			// we are done, the area is already beyond of what we have to free
			break;
		}

		if (areaStart > lastEnd) {
			// this is something we can free
			TRACE(("free boot range: get rid of %p - %p\n", (void*)lastEnd,
				(void*)areaStart));
			unmap_and_free_physical_pages(map, lastEnd, areaStart - 1);
		}

		if (areaEnd >= end) {
			lastEnd = areaEnd;
				// no +1 to prevent potential overflow
			break;
		}

		lastEnd = areaEnd + 1;
	}

	if (lastEnd < end) {
		// we can also get rid of some space at the end of the area
		TRACE(("free boot range: also remove %p - %p\n", (void*)lastEnd,
			(void*)end));
		unmap_and_free_physical_pages(map, lastEnd, end);
	}

	map->Unlock();
}


static void
create_preloaded_image_areas(struct preloaded_image* _image)
{
	preloaded_elf_image* image = static_cast<preloaded_elf_image*>(_image);
	char name[B_OS_NAME_LENGTH];
	void* address;
	int32 length;

	// use file name to create a good area name
	char* fileName = strrchr(image->name, '/');
	if (fileName == NULL)
		fileName = image->name;
	else
		fileName++;

	length = strlen(fileName);
	// make sure there is enough space for the suffix
	if (length > 25)
		length = 25;

	memcpy(name, fileName, length);
	strcpy(name + length, "_text");
	address = (void*)ROUNDDOWN(image->text_region.start, B_PAGE_SIZE);
	image->text_region.id = create_area(name, &address, B_EXACT_ADDRESS,
		PAGE_ALIGN(image->text_region.size), B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		// this will later be remapped read-only/executable by the
		// ELF initialization code

	strcpy(name + length, "_data");
	address = (void*)ROUNDDOWN(image->data_region.start, B_PAGE_SIZE);
	image->data_region.id = create_area(name, &address, B_EXACT_ADDRESS,
		PAGE_ALIGN(image->data_region.size), B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
}


/*!	Frees all previously kernel arguments areas from the kernel_args structure.
	Any boot loader resources contained in that arguments must not be accessed
	anymore past this point.
*/
void
vm_free_kernel_args(kernel_args* args)
{
	TRACE(("vm_free_kernel_args()\n"));

	for (uint32 i = 0; i < args->num_kernel_args_ranges; i++) {
		area_id area = area_for((void*)(addr_t)args->kernel_args_range[i].start);
		if (area >= B_OK)
			delete_area(area);
	}
}


static void
allocate_kernel_args(kernel_args* args)
{
	TRACE(("allocate_kernel_args()\n"));

	for (uint32 i = 0; i < args->num_kernel_args_ranges; i++) {
		const addr_range& range = args->kernel_args_range[i];
		void* address = (void*)(addr_t)range.start;

		create_area("_kernel args_", &address, B_EXACT_ADDRESS,
			range.size, B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	}
}


static addr_t
allocate_early_virtual(kernel_args* args, size_t size, addr_t alignment)
{
	size = PAGE_ALIGN(size);
	if (alignment <= B_PAGE_SIZE) {
		// All allocations are naturally page-aligned.
		alignment = 0;
	} else {
		ASSERT((alignment % B_PAGE_SIZE) == 0);
	}

	// Find a slot in the virtual allocation ranges.
	for (uint32 i = 1; i < args->num_virtual_allocated_ranges; i++) {
		// Check if the space between this one and the previous is big enough.
		const addr_range& range = args->virtual_allocated_range[i];
		addr_range& previousRange = args->virtual_allocated_range[i - 1];
		const addr_t previousRangeEnd = previousRange.start + previousRange.size;

		addr_t base = alignment > 0
			? ROUNDUP(previousRangeEnd, alignment) : previousRangeEnd;

		if (base >= KERNEL_BASE && base < range.start && (range.start - base) >= size) {
			previousRange.size += base + size - previousRangeEnd;
			return base;
		}
	}

	// We didn't find one between allocation ranges. This is OK.
	// See if there's a gap after the last one.
	addr_range& lastRange
		= args->virtual_allocated_range[args->num_virtual_allocated_ranges - 1];
	const addr_t lastRangeEnd = lastRange.start + lastRange.size;
	addr_t base = alignment > 0
		? ROUNDUP(lastRangeEnd, alignment) : lastRangeEnd;
	if ((KERNEL_TOP - base) >= size) {
		lastRange.size += base + size - lastRangeEnd;
		return base;
	}

	// See if there's a gap before the first one.
	addr_range& firstRange = args->virtual_allocated_range[0];
	if (firstRange.start > KERNEL_BASE && (firstRange.start - KERNEL_BASE) >= size) {
		base = firstRange.start - size;
		if (alignment > 0)
			base = ROUNDDOWN(base, alignment);

		if (base >= KERNEL_BASE) {
			firstRange.size += firstRange.start - base;
			firstRange.start = base;
			return base;
		}
	}

	return 0;
}


static bool
is_page_in_physical_memory_range(kernel_args* args, phys_addr_t address)
{
	// TODO: horrible brute-force method of determining if the page can be
	// allocated
	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		const addr_range& range = args->physical_memory_range[i];
		if (address >= range.start && address < (range.start + range.size))
			return true;
	}
	return false;
}


page_num_t
vm_allocate_early_physical_page(kernel_args* args, phys_addr_t maxAddress)
{
	if (args->num_physical_allocated_ranges == 0) {
		panic("early physical page allocations no longer possible!");
		return 0;
	}
	if (maxAddress == 0)
		maxAddress = __HAIKU_PHYS_ADDR_MAX;

#if defined(B_HAIKU_PHYSICAL_64_BIT)
	// Check if the last physical range is above the 32-bit maximum.
	const addr_range& lastMemoryRange =
		args->physical_memory_range[args->num_physical_memory_ranges - 1];
	const uint64 post32bitAddr = 0x100000000LL;
	if ((lastMemoryRange.start + lastMemoryRange.size) > post32bitAddr
			&& args->num_physical_allocated_ranges < MAX_PHYSICAL_ALLOCATED_RANGE) {
		// To avoid consuming physical memory in the 32-bit range (which drivers may need),
		// ensure the last allocated range at least ends past the 32-bit boundary.
		const addr_range& lastAllocatedRange =
			args->physical_allocated_range[args->num_physical_allocated_ranges - 1];
		const phys_addr_t lastAllocatedPage = lastAllocatedRange.start + lastAllocatedRange.size;
		if (lastAllocatedPage < post32bitAddr) {
			// Create ranges until we have one at least starting at the first point past 4GB.
			// (Some of the logic here is similar to the new-range code at the end of the method.)
			for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
				addr_range& memoryRange = args->physical_memory_range[i];
				if ((memoryRange.start + memoryRange.size) < lastAllocatedPage)
					continue;
				if (memoryRange.size < (B_PAGE_SIZE * 128))
					continue;

				uint64 rangeStart = memoryRange.start;
				if ((memoryRange.start + memoryRange.size) <= post32bitAddr) {
					if (memoryRange.start < lastAllocatedPage)
						continue;

					// Range has no pages allocated and ends before the 32-bit boundary.
				} else {
					// Range ends past the 32-bit boundary. It could have some pages allocated,
					// but if we're here, we know that nothing is allocated above the boundary,
					// so we want to create a new range with it regardless.
					if (rangeStart < post32bitAddr)
						rangeStart = post32bitAddr;
				}

				addr_range& allocatedRange =
					args->physical_allocated_range[args->num_physical_allocated_ranges++];
				allocatedRange.start = rangeStart;
				allocatedRange.size = 0;

				if (rangeStart >= post32bitAddr)
					break;
				if (args->num_physical_allocated_ranges == MAX_PHYSICAL_ALLOCATED_RANGE)
					break;
			}
		}
	}
#endif

	// Try expanding the existing physical ranges upwards.
	for (int32 i = args->num_physical_allocated_ranges - 1; i >= 0; i--) {
		addr_range& range = args->physical_allocated_range[i];
		phys_addr_t nextPage = range.start + range.size;

		// check constraints
		if (nextPage > maxAddress)
			continue;

		// make sure the page does not collide with the next allocated range
		if ((i + 1) < (int32)args->num_physical_allocated_ranges) {
			addr_range& nextRange = args->physical_allocated_range[i + 1];
			if (nextRange.size != 0 && nextPage >= nextRange.start)
				continue;
		}
		// see if the next page fits in the memory block
		if (is_page_in_physical_memory_range(args, nextPage)) {
			// we got one!
			range.size += B_PAGE_SIZE;
			return nextPage / B_PAGE_SIZE;
		}
	}

	// Expanding upwards didn't work, try going downwards.
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		addr_range& range = args->physical_allocated_range[i];
		phys_addr_t nextPage = range.start - B_PAGE_SIZE;

		// check constraints
		if (nextPage > maxAddress)
			continue;

		// make sure the page does not collide with the previous allocated range
		if (i > 0) {
			addr_range& previousRange = args->physical_allocated_range[i - 1];
			if (previousRange.size != 0 && nextPage < (previousRange.start + previousRange.size))
				continue;
		}
		// see if the next physical page fits in the memory block
		if (is_page_in_physical_memory_range(args, nextPage)) {
			// we got one!
			range.start -= B_PAGE_SIZE;
			range.size += B_PAGE_SIZE;
			return nextPage / B_PAGE_SIZE;
		}
	}

	// Try starting a new range.
	if (args->num_physical_allocated_ranges < MAX_PHYSICAL_ALLOCATED_RANGE) {
		const addr_range& lastAllocatedRange =
			args->physical_allocated_range[args->num_physical_allocated_ranges - 1];
		const phys_addr_t lastAllocatedPage = lastAllocatedRange.start + lastAllocatedRange.size;

		phys_addr_t nextPage = 0;
		for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
			const addr_range& range = args->physical_memory_range[i];
			// Ignore everything before the last-allocated page, as well as small ranges.
			if (range.start < lastAllocatedPage || range.size < (B_PAGE_SIZE * 128))
				continue;
			if (range.start > maxAddress)
				break;

			nextPage = range.start;
			break;
		}

		if (nextPage != 0) {
			// we got one!
			addr_range& range =
				args->physical_allocated_range[args->num_physical_allocated_ranges++];
			range.start = nextPage;
			range.size = B_PAGE_SIZE;
			return nextPage / B_PAGE_SIZE;
		}
	}

	return 0;
		// could not allocate a block
}


/*!	This one uses the kernel_args' physical and virtual memory ranges to
	allocate some pages before the VM is completely up.
*/
addr_t
vm_allocate_early(kernel_args* args, size_t virtualSize, size_t physicalSize,
	uint32 attributes, addr_t alignment)
{
	if (physicalSize > virtualSize)
		physicalSize = virtualSize;

	// find the vaddr to allocate at
	addr_t virtualBase = allocate_early_virtual(args, virtualSize, alignment);
	//dprintf("vm_allocate_early: vaddr 0x%lx\n", virtualBase);
	if (virtualBase == 0) {
		panic("vm_allocate_early: could not allocate virtual address\n");
		return 0;
	}

	// map the pages
	for (uint32 i = 0; i < HOWMANY(physicalSize, B_PAGE_SIZE); i++) {
		page_num_t physicalAddress = vm_allocate_early_physical_page(args);
		if (physicalAddress == 0)
			panic("error allocating early page!\n");

		//dprintf("vm_allocate_early: paddr 0x%lx\n", physicalAddress);

		status_t status = arch_vm_translation_map_early_map(args,
			virtualBase + i * B_PAGE_SIZE,
			physicalAddress * B_PAGE_SIZE, attributes);
		if (status != B_OK)
			panic("error mapping early page!");
	}

	return virtualBase;
}


void
vm_kernel_args_init_post_area(kernel_args* args)
{
	// allocate areas to represent stuff that already exists

	allocate_kernel_args(args);

	create_preloaded_image_areas(args->kernel_image);

	// allocate areas for preloaded images
	struct preloaded_image* image;
	for (image = args->preloaded_images; image != NULL; image = image->next)
		create_preloaded_image_areas(image);

	// allocate kernel stacks
	for (uint32 i = 0; i < args->num_cpus; i++) {
		char name[64];

		sprintf(name, "idle thread %" B_PRIu32 " kstack", i + 1);
		void* address = (void*)args->cpu_kstack[i].start;
		create_area(name, &address, B_EXACT_ADDRESS, args->cpu_kstack[i].size,
			B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	}
}
