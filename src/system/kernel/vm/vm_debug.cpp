/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Copyright 2010-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <ctype.h>

#include <team.h>

#include <vm/vm_page.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>
#include <vm/VMCache.h>


#if DEBUG_CACHE_LIST

struct cache_info {
	VMCache*	cache;
	addr_t		page_count;
	addr_t		committed;
};

static const uint32 kCacheInfoTableCount = 100 * 1024;
static cache_info* sCacheInfoTable;

#endif	// DEBUG_CACHE_LIST


static int
display_mem(int argc, char** argv)
{
	bool physical = false;
	addr_t copyAddress;
	int32 displayWidth;
	int32 itemSize;
	int32 num = -1;
	addr_t address;
	int i = 1, j;

	if (argc > 1 && argv[1][0] == '-') {
		if (!strcmp(argv[1], "-p") || !strcmp(argv[1], "--physical")) {
			physical = true;
			i++;
		} else
			i = 99;
	}

	if (argc < i + 1 || argc > i + 2) {
		kprintf("usage: dl/dw/ds/db/string [-p|--physical] <address> [num]\n"
			"\tdl - 8 bytes\n"
			"\tdw - 4 bytes\n"
			"\tds - 2 bytes\n"
			"\tdb - 1 byte\n"
			"\tstring - a whole string\n"
			"  -p or --physical only allows memory from a single page to be "
			"displayed.\n");
		return 0;
	}

	address = parse_expression(argv[i]);

	if (argc > i + 1)
		num = parse_expression(argv[i + 1]);

	// build the format string
	if (strcmp(argv[0], "db") == 0) {
		itemSize = 1;
		displayWidth = 16;
	} else if (strcmp(argv[0], "ds") == 0) {
		itemSize = 2;
		displayWidth = 8;
	} else if (strcmp(argv[0], "dw") == 0) {
		itemSize = 4;
		displayWidth = 4;
	} else if (strcmp(argv[0], "dl") == 0) {
		itemSize = 8;
		displayWidth = 2;
	} else if (strcmp(argv[0], "string") == 0) {
		itemSize = 1;
		displayWidth = -1;
	} else {
		kprintf("display_mem called in an invalid way!\n");
		return 0;
	}

	if (num <= 0)
		num = displayWidth;

	void* physicalPageHandle = NULL;

	if (physical) {
		int32 offset = address & (B_PAGE_SIZE - 1);
		if (num * itemSize + offset > B_PAGE_SIZE) {
			num = (B_PAGE_SIZE - offset) / itemSize;
			kprintf("NOTE: number of bytes has been cut to page size\n");
		}

		address = ROUNDDOWN(address, B_PAGE_SIZE);

		if (vm_get_physical_page_debug(address, &copyAddress,
				&physicalPageHandle) != B_OK) {
			kprintf("getting the hardware page failed.");
			return 0;
		}

		address += offset;
		copyAddress += offset;
	} else
		copyAddress = address;

	if (!strcmp(argv[0], "string")) {
		kprintf("%p \"", (char*)copyAddress);

		// string mode
		for (i = 0; true; i++) {
			char c;
			if (debug_memcpy(B_CURRENT_TEAM, &c, (char*)copyAddress + i, 1)
					!= B_OK
				|| c == '\0') {
				break;
			}

			if (c == '\n')
				kprintf("\\n");
			else if (c == '\t')
				kprintf("\\t");
			else {
				if (!isprint(c))
					c = '.';

				kprintf("%c", c);
			}
		}

		kprintf("\"\n");
	} else {
		// number mode
		for (i = 0; i < num; i++) {
			uint64 value;

			if ((i % displayWidth) == 0) {
				int32 displayed = min_c(displayWidth, (num-i)) * itemSize;
				if (i != 0)
					kprintf("\n");

				kprintf("[0x%lx]  ", address + i * itemSize);

				for (j = 0; j < displayed; j++) {
					char c;
					if (debug_memcpy(B_CURRENT_TEAM, &c,
							(char*)copyAddress + i * itemSize + j, 1) != B_OK) {
						displayed = j;
						break;
					}
					if (!isprint(c))
						c = '.';

					kprintf("%c", c);
				}
				if (num > displayWidth) {
					// make sure the spacing in the last line is correct
					for (j = displayed; j < displayWidth * itemSize; j++)
						kprintf(" ");
				}
				kprintf("  ");
			}

			if (debug_memcpy(B_CURRENT_TEAM, &value,
					(uint8*)copyAddress + i * itemSize, itemSize) != B_OK) {
				kprintf("read fault");
				break;
			}

			switch (itemSize) {
				case 1:
					kprintf(" %02" B_PRIx8, *(uint8*)&value);
					break;
				case 2:
					kprintf(" %04" B_PRIx16, *(uint16*)&value);
					break;
				case 4:
					kprintf(" %08" B_PRIx32, *(uint32*)&value);
					break;
				case 8:
					kprintf(" %016" B_PRIx64, *(uint64*)&value);
					break;
			}
		}

		kprintf("\n");
	}

	if (physical) {
		copyAddress = ROUNDDOWN(copyAddress, B_PAGE_SIZE);
		vm_put_physical_page_debug(copyAddress, physicalPageHandle);
	}
	return 0;
}


static void
dump_cache_tree_recursively(VMCache* cache, int level,
	VMCache* highlightCache)
{
	// print this cache
	for (int i = 0; i < level; i++)
		kprintf("  ");
	if (cache == highlightCache)
		kprintf("%p <--\n", cache);
	else
		kprintf("%p\n", cache);

	// recursively print its consumers
	for (VMCache::ConsumerList::Iterator it = cache->consumers.GetIterator();
			VMCache* consumer = it.Next();) {
		dump_cache_tree_recursively(consumer, level + 1, highlightCache);
	}
}


static int
dump_cache_tree(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <address>\n", argv[0]);
		return 0;
	}

	addr_t address = parse_expression(argv[1]);
	if (address == 0)
		return 0;

	VMCache* cache = (VMCache*)address;
	VMCache* root = cache;

	// find the root cache (the transitive source)
	while (root->source != NULL)
		root = root->source;

	dump_cache_tree_recursively(root, 0, cache);

	return 0;
}


const char*
vm_cache_type_to_string(int32 type)
{
	switch (type) {
		case CACHE_TYPE_RAM:
			return "RAM";
		case CACHE_TYPE_DEVICE:
			return "device";
		case CACHE_TYPE_VNODE:
			return "vnode";
		case CACHE_TYPE_NULL:
			return "null";

		default:
			return "unknown";
	}
}


#if DEBUG_CACHE_LIST

static void
update_cache_info_recursively(VMCache* cache, cache_info& info)
{
	info.page_count += cache->page_count;
	if (cache->type == CACHE_TYPE_RAM)
		info.committed += cache->committed_size;

	// recurse
	for (VMCache::ConsumerList::Iterator it = cache->consumers.GetIterator();
			VMCache* consumer = it.Next();) {
		update_cache_info_recursively(consumer, info);
	}
}


static int
cache_info_compare_page_count(const void* _a, const void* _b)
{
	const cache_info* a = (const cache_info*)_a;
	const cache_info* b = (const cache_info*)_b;
	if (a->page_count == b->page_count)
		return 0;
	return a->page_count < b->page_count ? 1 : -1;
}


static int
cache_info_compare_committed(const void* _a, const void* _b)
{
	const cache_info* a = (const cache_info*)_a;
	const cache_info* b = (const cache_info*)_b;
	if (a->committed == b->committed)
		return 0;
	return a->committed < b->committed ? 1 : -1;
}


static void
dump_caches_recursively(VMCache* cache, cache_info& info, int level)
{
	for (int i = 0; i < level; i++)
		kprintf("  ");

	kprintf("%p: type: %s, base: %" B_PRIdOFF ", size: %" B_PRIdOFF ", "
		"pages: %" B_PRIu32, cache, vm_cache_type_to_string(cache->type),
		cache->virtual_base, cache->virtual_end, cache->page_count);

	if (level == 0)
		kprintf("/%lu", info.page_count);

	if (cache->type == CACHE_TYPE_RAM || (level == 0 && info.committed > 0)) {
		kprintf(", committed: %" B_PRIdOFF, cache->committed_size);

		if (level == 0)
			kprintf("/%lu", info.committed);
	}

	// areas
	if (cache->areas != NULL) {
		VMArea* area = cache->areas;
		kprintf(", areas: %" B_PRId32 " (%s, team: %" B_PRId32 ")", area->id,
			area->name, area->address_space->ID());

		while (area->cache_next != NULL) {
			area = area->cache_next;
			kprintf(", %" B_PRId32, area->id);
		}
	}

	kputs("\n");

	// recurse
	for (VMCache::ConsumerList::Iterator it = cache->consumers.GetIterator();
			VMCache* consumer = it.Next();) {
		dump_caches_recursively(consumer, info, level + 1);
	}
}


static int
dump_caches(int argc, char** argv)
{
	if (sCacheInfoTable == NULL) {
		kprintf("No cache info table!\n");
		return 0;
	}

	bool sortByPageCount = true;

	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0) {
			sortByPageCount = false;
		} else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	uint32 totalCount = 0;
	uint32 rootCount = 0;
	off_t totalCommitted = 0;
	page_num_t totalPages = 0;

	VMCache* cache = gDebugCacheList;
	while (cache) {
		totalCount++;
		if (cache->source == NULL) {
			cache_info stackInfo;
			cache_info& info = rootCount < kCacheInfoTableCount
				? sCacheInfoTable[rootCount] : stackInfo;
			rootCount++;
			info.cache = cache;
			info.page_count = 0;
			info.committed = 0;
			update_cache_info_recursively(cache, info);
			totalCommitted += info.committed;
			totalPages += info.page_count;
		}

		cache = cache->debug_next;
	}

	kprintf("total committed memory: %" B_PRIdOFF ", total used pages: %"
		B_PRIuPHYSADDR "\n", totalCommitted, totalPages);
	kprintf("%" B_PRIu32 " caches (%" B_PRIu32 " root caches), sorted by %s "
		"per cache tree...\n\n", totalCount, rootCount, sortByPageCount ?
			"page count" : "committed size");

	if (rootCount > kCacheInfoTableCount) {
		kprintf("Cache info table too small! Can't sort and print caches!\n");
		return 0;
	}

	qsort(sCacheInfoTable, rootCount, sizeof(cache_info),
		sortByPageCount
			? &cache_info_compare_page_count
			: &cache_info_compare_committed);

	for (uint32 i = 0; i < rootCount; i++) {
		cache_info& info = sCacheInfoTable[i];
		dump_caches_recursively(info.cache, info, 0);
	}

	return 0;
}

#endif	// DEBUG_CACHE_LIST


static int
dump_cache(int argc, char** argv)
{
	VMCache* cache;
	bool showPages = false;
	int i = 1;

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s [-ps] <address>\n"
			"  if -p is specified, all pages are shown, if -s is used\n"
			"  only the cache info is shown respectively.\n", argv[0]);
		return 0;
	}
	while (argv[i][0] == '-') {
		char* arg = argv[i] + 1;
		while (arg[0]) {
			if (arg[0] == 'p')
				showPages = true;
			arg++;
		}
		i++;
	}
	if (argv[i] == NULL) {
		kprintf("%s: invalid argument, pass address\n", argv[0]);
		return 0;
	}

	addr_t address = parse_expression(argv[i]);
	if (address == 0)
		return 0;

	cache = (VMCache*)address;

	cache->Dump(showPages);

	set_debug_variable("_sourceCache", (addr_t)cache->source);

	return 0;
}


static void
dump_area_struct(VMArea* area, bool mappings)
{
	kprintf("AREA: %p\n", area);
	kprintf("name:\t\t'%s'\n", area->name);
	kprintf("owner:\t\t0x%" B_PRIx32 "\n", area->address_space->ID());
	kprintf("id:\t\t0x%" B_PRIx32 "\n", area->id);
	kprintf("base:\t\t0x%lx\n", area->Base());
	kprintf("size:\t\t0x%lx\n", area->Size());
	kprintf("protection:\t0x%" B_PRIx32 "\n", area->protection);
	kprintf("page_protection:%p\n", area->page_protections);
	kprintf("wiring:\t\t0x%x\n", area->wiring);
	kprintf("memory_type:\t%#" B_PRIx32 "\n", area->MemoryType());
	kprintf("cache:\t\t%p\n", area->cache);
	kprintf("cache_type:\t%s\n", vm_cache_type_to_string(area->cache_type));
	kprintf("cache_offset:\t0x%" B_PRIx64 "\n", area->cache_offset);
	kprintf("cache_next:\t%p\n", area->cache_next);
	kprintf("cache_prev:\t%p\n", area->cache_prev);

	VMAreaMappings::Iterator iterator = area->mappings.GetIterator();
	if (mappings) {
		kprintf("page mappings:\n");
		while (iterator.HasNext()) {
			vm_page_mapping* mapping = iterator.Next();
			kprintf("  %p", mapping->page);
		}
		kprintf("\n");
	} else {
		uint32 count = 0;
		while (iterator.Next() != NULL) {
			count++;
		}
		kprintf("page mappings:\t%" B_PRIu32 "\n", count);
	}
}


static int
dump_area(int argc, char** argv)
{
	bool mappings = false;
	bool found = false;
	int32 index = 1;
	VMArea* area;
	addr_t num;

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: area [-m] [id|contains|address|name] <id|address|name>\n"
			"All areas matching either id/address/name are listed. You can\n"
			"force to check only a specific item by prefixing the specifier\n"
			"with the id/contains/address/name keywords.\n"
			"-m shows the area's mappings as well.\n");
		return 0;
	}

	if (!strcmp(argv[1], "-m")) {
		mappings = true;
		index++;
	}

	int32 mode = 0xf;
	if (!strcmp(argv[index], "id"))
		mode = 1;
	else if (!strcmp(argv[index], "contains"))
		mode = 2;
	else if (!strcmp(argv[index], "name"))
		mode = 4;
	else if (!strcmp(argv[index], "address"))
		mode = 0;
	if (mode != 0xf)
		index++;

	if (index >= argc) {
		kprintf("No area specifier given.\n");
		return 0;
	}

	num = parse_expression(argv[index]);

	if (mode == 0) {
		dump_area_struct((struct VMArea*)num, mappings);
	} else {
		// walk through the area list, looking for the arguments as a name

		VMAreasTree::Iterator it = VMAreas::GetIterator();
		while ((area = it.Next()) != NULL) {
			if (((mode & 4) != 0
					&& !strcmp(argv[index], area->name))
				|| (num != 0 && (((mode & 1) != 0 && (addr_t)area->id == num)
					|| (((mode & 2) != 0 && area->Base() <= num
						&& area->Base() + area->Size() > num))))) {
				dump_area_struct(area, mappings);
				found = true;
			}
		}

		if (!found)
			kprintf("could not find area %s (%ld)\n", argv[index], num);
	}

	return 0;
}


static int
dump_area_list(int argc, char** argv)
{
	VMArea* area;
	const char* name = NULL;
	int32 id = 0;

	if (argc > 1) {
		id = parse_expression(argv[1]);
		if (id == 0)
			name = argv[1];
	}

	kprintf("%-*s      id  %-*s    %-*sprotect lock  name\n",
		B_PRINTF_POINTER_WIDTH, "addr", B_PRINTF_POINTER_WIDTH, "base",
		B_PRINTF_POINTER_WIDTH, "size");

	VMAreasTree::Iterator it = VMAreas::GetIterator();
	while ((area = it.Next()) != NULL) {
		if ((id != 0 && area->address_space->ID() != id)
			|| (name != NULL && strstr(area->name, name) == NULL))
			continue;

		kprintf("%p %5" B_PRIx32 "  %p  %p %4" B_PRIx32 " %4d  %s\n", area,
			area->id, (void*)area->Base(), (void*)area->Size(),
			area->protection, area->wiring, area->name);
	}
	return 0;
}


static int
dump_available_memory(int argc, char** argv)
{
	kprintf("Available memory: %" B_PRIdOFF "/%" B_PRIuPHYSADDR " bytes\n",
		vm_available_memory_debug(), (phys_addr_t)vm_page_num_pages() * B_PAGE_SIZE);
	return 0;
}


static int
dump_mapping_info(int argc, char** argv)
{
	bool reverseLookup = false;
	bool pageLookup = false;

	int argi = 1;
	for (; argi < argc && argv[argi][0] == '-'; argi++) {
		const char* arg = argv[argi];
		if (strcmp(arg, "-r") == 0) {
			reverseLookup = true;
		} else if (strcmp(arg, "-p") == 0) {
			reverseLookup = true;
			pageLookup = true;
		} else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	// We need at least one argument, the address. Optionally a thread ID can be
	// specified.
	if (argi >= argc || argi + 2 < argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	uint64 addressValue;
	if (!evaluate_debug_expression(argv[argi++], &addressValue, false))
		return 0;

	Team* team = NULL;
	if (argi < argc) {
		uint64 threadID;
		if (!evaluate_debug_expression(argv[argi++], &threadID, false))
			return 0;

		Thread* thread = Thread::GetDebug(threadID);
		if (thread == NULL) {
			kprintf("Invalid thread/team ID \"%s\"\n", argv[argi - 1]);
			return 0;
		}

		team = thread->team;
	}

	if (reverseLookup) {
		phys_addr_t physicalAddress;
		if (pageLookup) {
			vm_page* page = (vm_page*)(addr_t)addressValue;
			physicalAddress = page->physical_page_number * B_PAGE_SIZE;
		} else {
			physicalAddress = (phys_addr_t)addressValue;
			physicalAddress -= physicalAddress % B_PAGE_SIZE;
		}

		kprintf("    Team     Virtual Address      Area\n");
		kprintf("--------------------------------------\n");

		struct Callback : VMTranslationMap::ReverseMappingInfoCallback {
			Callback()
				:
				fAddressSpace(NULL)
			{
			}

			void SetAddressSpace(VMAddressSpace* addressSpace)
			{
				fAddressSpace = addressSpace;
			}

			virtual bool HandleVirtualAddress(addr_t virtualAddress)
			{
				kprintf("%8" B_PRId32 "  %#18" B_PRIxADDR, fAddressSpace->ID(),
					virtualAddress);
				if (VMArea* area = fAddressSpace->LookupArea(virtualAddress))
					kprintf("  %8" B_PRId32 " %s\n", area->id, area->name);
				else
					kprintf("\n");
				return false;
			}

		private:
			VMAddressSpace*	fAddressSpace;
		} callback;

		if (team != NULL) {
			// team specified -- get its address space
			VMAddressSpace* addressSpace = team->address_space;
			if (addressSpace == NULL) {
				kprintf("Failed to get address space!\n");
				return 0;
			}

			callback.SetAddressSpace(addressSpace);
			addressSpace->TranslationMap()->DebugGetReverseMappingInfo(
				physicalAddress, callback);
		} else {
			// no team specified -- iterate through all address spaces
			for (VMAddressSpace* addressSpace = VMAddressSpace::DebugFirst();
				addressSpace != NULL;
				addressSpace = VMAddressSpace::DebugNext(addressSpace)) {
				callback.SetAddressSpace(addressSpace);
				addressSpace->TranslationMap()->DebugGetReverseMappingInfo(
					physicalAddress, callback);
			}
		}
	} else {
		// get the address space
		addr_t virtualAddress = (addr_t)addressValue;
		virtualAddress -= virtualAddress % B_PAGE_SIZE;
		VMAddressSpace* addressSpace;
		if (IS_KERNEL_ADDRESS(virtualAddress)) {
			addressSpace = VMAddressSpace::Kernel();
		} else if (team != NULL) {
			addressSpace = team->address_space;
		} else {
			Thread* thread = debug_get_debugged_thread();
			if (thread == NULL || thread->team == NULL) {
				kprintf("Failed to get team!\n");
				return 0;
			}

			addressSpace = thread->team->address_space;
		}

		if (addressSpace == NULL) {
			kprintf("Failed to get address space!\n");
			return 0;
		}

		// let the translation map implementation do the job
		addressSpace->TranslationMap()->DebugPrintMappingInfo(virtualAddress);
	}

	return 0;
}


/*!	Copies a range of memory directly from/to a page that might not be mapped
	at the moment.

	For \a unsafeMemory the current mapping (if any is ignored). The function
	walks through the respective area's cache chain to find the physical page
	and copies from/to it directly.
	The memory range starting at \a unsafeMemory with a length of \a size bytes
	must not cross a page boundary.

	\param teamID The team ID identifying the address space \a unsafeMemory is
		to be interpreted in. Ignored, if \a unsafeMemory is a kernel address
		(the kernel address space is assumed in this case). If \c B_CURRENT_TEAM
		is passed, the address space of the thread returned by
		debug_get_debugged_thread() is used.
	\param unsafeMemory The start of the unsafe memory range to be copied
		from/to.
	\param buffer A safely accessible kernel buffer to be copied from/to.
	\param size The number of bytes to be copied.
	\param copyToUnsafe If \c true, memory is copied from \a buffer to
		\a unsafeMemory, the other way around otherwise.
*/
status_t
vm_debug_copy_page_memory(team_id teamID, void* unsafeMemory, void* buffer,
	size_t size, bool copyToUnsafe)
{
	if (size > B_PAGE_SIZE || ROUNDDOWN((addr_t)unsafeMemory, B_PAGE_SIZE)
			!= ROUNDDOWN((addr_t)unsafeMemory + size - 1, B_PAGE_SIZE)) {
		return B_BAD_VALUE;
	}

	// get the address space for the debugged thread
	VMAddressSpace* addressSpace;
	if (IS_KERNEL_ADDRESS(unsafeMemory)) {
		addressSpace = VMAddressSpace::Kernel();
	} else if (teamID == B_CURRENT_TEAM) {
		Thread* thread = debug_get_debugged_thread();
		if (thread == NULL || thread->team == NULL)
			return B_BAD_ADDRESS;

		addressSpace = thread->team->address_space;
	} else
		addressSpace = VMAddressSpace::DebugGet(teamID);

	if (addressSpace == NULL)
		return B_BAD_ADDRESS;

	// get the area
	VMArea* area = addressSpace->LookupArea((addr_t)unsafeMemory);
	if (area == NULL)
		return B_BAD_ADDRESS;

	// search the page
	off_t cacheOffset = (addr_t)unsafeMemory - area->Base()
		+ area->cache_offset;
	VMCache* cache = area->cache;
	vm_page* page = NULL;
	while (cache != NULL) {
		page = cache->DebugLookupPage(cacheOffset);
		if (page != NULL)
			break;

		// Page not found in this cache -- if it is paged out, we must not try
		// to get it from lower caches.
		if (cache->DebugHasPage(cacheOffset))
			break;

		cache = cache->source;
	}

	if (page == NULL)
		return B_UNSUPPORTED;

	// copy from/to physical memory
	phys_addr_t physicalAddress = page->physical_page_number * B_PAGE_SIZE
		+ (addr_t)unsafeMemory % B_PAGE_SIZE;

	if (copyToUnsafe) {
		if (page->Cache() != area->cache)
			return B_UNSUPPORTED;

		return vm_memcpy_to_physical(physicalAddress, buffer, size, false);
	}

	return vm_memcpy_from_physical(buffer, physicalAddress, size, false);
}


void
vm_debug_init()
{
#if DEBUG_CACHE_LIST
	if (vm_page_num_free_pages() >= 200 * 1024 * 1024 / B_PAGE_SIZE) {
		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
		physical_address_restrictions physicalRestrictions = {};
		create_area_etc(VMAddressSpace::KernelID(), "cache info table",
			ROUNDUP(kCacheInfoTableCount * sizeof(cache_info), B_PAGE_SIZE),
			B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
			CREATE_AREA_DONT_WAIT, 0, &virtualRestrictions,
			&physicalRestrictions, (void**)&sCacheInfoTable);
	}
#endif	// DEBUG_CACHE_LIST

	// add some debugger commands
	add_debugger_command("areas", &dump_area_list, "Dump a list of all areas");
	add_debugger_command("area", &dump_area,
		"Dump info about a particular area");
	add_debugger_command("cache", &dump_cache, "Dump VMCache");
	add_debugger_command("cache_tree", &dump_cache_tree, "Dump VMCache tree");
#if DEBUG_CACHE_LIST
	if (sCacheInfoTable != NULL) {
		add_debugger_command_etc("caches", &dump_caches,
			"List all VMCache trees",
			"[ \"-c\" ]\n"
			"All cache trees are listed sorted in decreasing order by number "
				"of\n"
			"used pages or, if \"-c\" is specified, by size of committed "
				"memory.\n",
			0);
	}
#endif
	add_debugger_command("avail", &dump_available_memory,
		"Dump available memory");
	add_debugger_command("dl", &display_mem, "dump memory long words (64-bit)");
	add_debugger_command("dw", &display_mem, "dump memory words (32-bit)");
	add_debugger_command("ds", &display_mem, "dump memory shorts (16-bit)");
	add_debugger_command("db", &display_mem, "dump memory bytes (8-bit)");
	add_debugger_command("string", &display_mem, "dump strings");

	add_debugger_command_etc("mapping", &dump_mapping_info,
		"Print address mapping information",
		"[ \"-r\" | \"-p\" ] <address> [ <thread ID> ]\n"
		"Prints low-level page mapping information for a given address. If\n"
		"neither \"-r\" nor \"-p\" are specified, <address> is a virtual\n"
		"address that is looked up in the translation map of the current\n"
		"team, respectively the team specified by thread ID <thread ID>. If\n"
		"\"-r\" is specified, <address> is a physical address that is\n"
		"searched in the translation map of all teams, respectively the team\n"
		"specified by thread ID <thread ID>. If \"-p\" is specified,\n"
		"<address> is the address of a vm_page structure. The behavior is\n"
		"equivalent to specifying \"-r\" with the physical address of that\n"
		"page.\n",
		0);
}
