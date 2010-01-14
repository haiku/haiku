/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <arch/vm_translation_map.h>

#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>

#include <arch_system_info.h>
#include <heap.h>
#include <int.h>
#include <thread.h>
#include <smp.h>
#include <util/AutoLock.h>
#include <util/queue.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>

#include "x86_paging.h"
#include "x86_physical_page_mapper.h"
#include "X86VMTranslationMap.h"


//#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static page_table_entry *sPageHole = NULL;
static page_directory_entry *sPageHolePageDir = NULL;
static page_directory_entry *sKernelPhysicalPageDirectory = NULL;
static page_directory_entry *sKernelVirtualPageDirectory = NULL;

static X86PhysicalPageMapper* sPhysicalPageMapper;
static TranslationMapPhysicalPageMapper* sKernelPhysicalPageMapper;


// Accessor class to reuse the SinglyLinkedListLink of DeferredDeletable for
// vm_translation_map_arch_info.
struct ArchTMapGetLink {
private:
	typedef SinglyLinkedListLink<vm_translation_map_arch_info> Link;

public:
	inline Link* operator()(vm_translation_map_arch_info* element) const
	{
		return (Link*)element->GetSinglyLinkedListLink();
	}

	inline const Link* operator()(
		const vm_translation_map_arch_info* element) const
	{
		return (const Link*)element->GetSinglyLinkedListLink();
	}

};


typedef SinglyLinkedList<vm_translation_map_arch_info, ArchTMapGetLink>
	ArchTMapList;


static ArchTMapList sTMapList;
static spinlock sTMapListLock;

#define CHATTY_TMAP 0

#define FIRST_USER_PGDIR_ENT    (VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGDIR_ENTS     (VADDR_TO_PDENT(ROUNDUP(USER_SIZE, \
									B_PAGE_SIZE * 1024)))
#define FIRST_KERNEL_PGDIR_ENT  (VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGDIR_ENTS   (VADDR_TO_PDENT(KERNEL_SIZE))
#define IS_KERNEL_MAP(map)		(fArchData->pgdir_phys \
									== sKernelPhysicalPageDirectory)


vm_translation_map_arch_info::vm_translation_map_arch_info()
	:
	pgdir_virt(NULL),
	ref_count(1)
{
}


vm_translation_map_arch_info::~vm_translation_map_arch_info()
{
	// free the page dir
	free(pgdir_virt);
}


void
vm_translation_map_arch_info::Delete()
{
	// remove from global list
	InterruptsSpinLocker locker(sTMapListLock);
	sTMapList.Remove(this);
	locker.Unlock();

#if 0
	// this sanity check can be enabled when corruption due to
	// overwriting an active page directory is suspected
	addr_t activePageDirectory;
	read_cr3(activePageDirectory);
	if (activePageDirectory == (addr_t)pgdir_phys)
		panic("deleting a still active page directory\n");
#endif

	if (are_interrupts_enabled())
		delete this;
	else
		deferred_delete(this);
}


//	#pragma mark -


//! TODO: currently assumes this translation map is active
static status_t
early_query(addr_t va, addr_t *_physicalAddress)
{
	if ((sPageHolePageDir[VADDR_TO_PDENT(va)] & X86_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_ERROR;
	}

	page_table_entry* pentry = sPageHole + va / B_PAGE_SIZE;
	if ((*pentry & X86_PTE_PRESENT) == 0) {
		// page mapping not valid
		return B_ERROR;
	}

	*_physicalAddress = *pentry & X86_PTE_ADDRESS_MASK;
	return B_OK;
}


static void
put_page_table_entry_in_pgtable(page_table_entry* entry,
	addr_t physicalAddress, uint32 attributes, bool globalPage)
{
	page_table_entry page = (physicalAddress & X86_PTE_ADDRESS_MASK)
		| X86_PTE_PRESENT | (globalPage ? X86_PTE_GLOBAL : 0);

	// if the page is user accessible, it's automatically
	// accessible in kernel space, too (but with the same
	// protection)
	if ((attributes & B_USER_PROTECTION) != 0) {
		page |= X86_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			page |= X86_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		page |= X86_PTE_WRITABLE;

	// put it in the page table
	*(volatile page_table_entry*)entry = page;
}


//	#pragma mark -


void *
i386_translation_map_get_pgdir(VMTranslationMap* map)
{
	return static_cast<X86VMTranslationMap*>(map)->PhysicalPageDir();
}


void
x86_update_all_pgdirs(int index, page_directory_entry e)
{
	unsigned int state = disable_interrupts();

	acquire_spinlock(&sTMapListLock);

	ArchTMapList::Iterator it = sTMapList.GetIterator();
	while (vm_translation_map_arch_info* info = it.Next())
		info->pgdir_virt[index] = e;

	release_spinlock(&sTMapListLock);
	restore_interrupts(state);
}


void
x86_put_pgtable_in_pgdir(page_directory_entry *entry,
	addr_t pgtablePhysical, uint32 attributes)
{
	*entry = (pgtablePhysical & X86_PDE_ADDRESS_MASK)
		| X86_PDE_PRESENT
		| X86_PDE_WRITABLE
		| X86_PDE_USER;
		// TODO: we ignore the attributes of the page table - for compatibility
		// with BeOS we allow having user accessible areas in the kernel address
		// space. This is currently being used by some drivers, mainly for the
		// frame buffer. Our current real time data implementation makes use of
		// this fact, too.
		// We might want to get rid of this possibility one day, especially if
		// we intend to port it to a platform that does not support this.
}


void
x86_early_prepare_page_tables(page_table_entry* pageTables, addr_t address,
	size_t size)
{
	memset(pageTables, 0, B_PAGE_SIZE * (size / (B_PAGE_SIZE * 1024)));

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be easy to get
	// to
	{
		addr_t virtualTable = (addr_t)pageTables;

		for (size_t i = 0; i < (size / (B_PAGE_SIZE * 1024));
				i++, virtualTable += B_PAGE_SIZE) {
			addr_t physicalTable = 0;
			early_query(virtualTable, &physicalTable);
			page_directory_entry* entry = &sPageHolePageDir[
				(address / (B_PAGE_SIZE * 1024)) + i];
			x86_put_pgtable_in_pgdir(entry, physicalTable,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}
	}
}


// #pragma mark - VM ops


X86VMTranslationMap::X86VMTranslationMap()
{
}


X86VMTranslationMap::~X86VMTranslationMap()
{
	if (fArchData->page_mapper != NULL)
		fArchData->page_mapper->Delete();

	if (fArchData->pgdir_virt != NULL) {
		// cycle through and free all of the user space pgtables
		for (uint32 i = VADDR_TO_PDENT(USER_BASE);
				i <= VADDR_TO_PDENT(USER_BASE + (USER_SIZE - 1)); i++) {
			if ((fArchData->pgdir_virt[i] & X86_PDE_PRESENT) != 0) {
				addr_t address = fArchData->pgdir_virt[i]
					& X86_PDE_ADDRESS_MASK;
				vm_page* page = vm_lookup_page(address / B_PAGE_SIZE);
				if (!page)
					panic("destroy_tmap: didn't find pgtable page\n");
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
		}
	}

	fArchData->RemoveReference();
}


status_t
X86VMTranslationMap::Init(bool kernel)
{
	TRACE(("X86VMTranslationMap::Init()\n"));

	fArchData = new(std::nothrow) vm_translation_map_arch_info;
	if (fArchData == NULL)
		return B_NO_MEMORY;

	fArchData->active_on_cpus = 0;
	fArchData->num_invalidate_pages = 0;
	fArchData->page_mapper = NULL;

	if (!kernel) {
		// user
		// allocate a physical page mapper
		status_t error = sPhysicalPageMapper
			->CreateTranslationMapPhysicalPageMapper(
				&fArchData->page_mapper);
		if (error != B_OK)
			return error;

		// allocate a pgdir
		fArchData->pgdir_virt = (page_directory_entry *)memalign(
			B_PAGE_SIZE, B_PAGE_SIZE);
		if (fArchData->pgdir_virt == NULL) {
			fArchData->page_mapper->Delete();
			return B_NO_MEMORY;
		}
		vm_get_page_mapping(VMAddressSpace::KernelID(),
			(addr_t)fArchData->pgdir_virt,
			(addr_t*)&fArchData->pgdir_phys);
	} else {
		// kernel
		// get the physical page mapper
		fArchData->page_mapper = sKernelPhysicalPageMapper;

		// we already know the kernel pgdir mapping
		fArchData->pgdir_virt = sKernelVirtualPageDirectory;
		fArchData->pgdir_phys = sKernelPhysicalPageDirectory;
	}

	// zero out the bottom portion of the new pgdir
	memset(fArchData->pgdir_virt + FIRST_USER_PGDIR_ENT, 0,
		NUM_USER_PGDIR_ENTS * sizeof(page_directory_entry));

	// insert this new map into the map list
	{
		int state = disable_interrupts();
		acquire_spinlock(&sTMapListLock);

		// copy the top portion of the pgdir from the current one
		memcpy(fArchData->pgdir_virt + FIRST_KERNEL_PGDIR_ENT,
			sKernelVirtualPageDirectory + FIRST_KERNEL_PGDIR_ENT,
			NUM_KERNEL_PGDIR_ENTS * sizeof(page_directory_entry));

		sTMapList.Add(fArchData);

		release_spinlock(&sTMapListLock);
		restore_interrupts(state);
	}

	return B_OK;
}


status_t
X86VMTranslationMap::InitPostSem()
{
	return B_OK;
}


/*!	Acquires the map's recursive lock, and resets the invalidate pages counter
	in case it's the first locking recursion.
*/
bool
X86VMTranslationMap::Lock()
{
	TRACE(("lock_tmap: map %p\n", map));

	recursive_lock_lock(&fLock);
	if (recursive_lock_get_recursion(&fLock) == 1) {
		// we were the first one to grab the lock
		TRACE(("clearing invalidated page count\n"));
		fArchData->num_invalidate_pages = 0;
	}

	return true;
}


/*!	Unlocks the map, and, if we'll actually losing the recursive lock,
	flush all pending changes of this map (ie. flush TLB caches as
	needed).
*/
void
X86VMTranslationMap::Unlock()
{
	TRACE(("unlock_tmap: map %p\n", map));

	if (recursive_lock_get_recursion(&fLock) == 1) {
		// we're about to release it for the last time
		X86VMTranslationMap::Flush();
	}

	recursive_lock_unlock(&fLock);
}


size_t
X86VMTranslationMap::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
	// If start == 0, the actual base address is not yet known to the caller and
	// we shall assume the worst case.
	if (start == 0) {
		// offset the range so it has the worst possible alignment
		start = 1023 * B_PAGE_SIZE;
		end += 1023 * B_PAGE_SIZE;
	}

	return VADDR_TO_PDENT(end) + 1 - VADDR_TO_PDENT(start);
}


status_t
X86VMTranslationMap::Map(addr_t va, addr_t pa, uint32 attributes)
{
	TRACE(("map_tmap: entry pa 0x%lx va 0x%lx\n", pa, va));

/*
	dprintf("pgdir at 0x%x\n", pgdir);
	dprintf("index is %d\n", va / B_PAGE_SIZE / 1024);
	dprintf("final at 0x%x\n", &pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("value is 0x%x\n", *(int *)&pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("present bit is %d\n", pgdir[va / B_PAGE_SIZE / 1024].present);
	dprintf("addr is %d\n", pgdir[va / B_PAGE_SIZE / 1024].addr);
*/
	page_directory_entry* pd = fArchData->pgdir_virt;

	// check to see if a page table exists for this range
	uint32 index = VADDR_TO_PDENT(va);
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		addr_t pgtable;
		vm_page *page;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(PAGE_STATE_CLEAR);

		// mark the page WIRED
		vm_page_set_state(page, PAGE_STATE_WIRED);

		DEBUG_PAGE_ACCESS_END(page);

		pgtable = page->physical_page_number * B_PAGE_SIZE;

		TRACE(("map_tmap: asked for free page for pgtable. 0x%lx\n", pgtable));

		// put it in the pgdir
		x86_put_pgtable_in_pgdir(&pd[index], pgtable, attributes
			| ((attributes & B_USER_PROTECTION) != 0
					? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

		// update any other page directories, if it maps kernel space
		if (index >= FIRST_KERNEL_PGDIR_ENT
			&& index < (FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS))
			x86_update_all_pgdirs(index, pd[index]);

		fMapCount++;
	}

	// now, fill in the pentry
	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = fArchData->page_mapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	ASSERT((pt[index] & X86_PTE_PRESENT) == 0);

	put_page_table_entry_in_pgtable(&pt[index], pa, attributes,
		IS_KERNEL_MAP(map));

	pinner.Unlock();

	// Note: We don't need to invalidate the TLB for this address, as previously
	// the entry was not present and the TLB doesn't cache those entries.

	fMapCount++;

	return 0;
}


status_t
X86VMTranslationMap::Unmap(addr_t start, addr_t end)
{
	page_directory_entry *pd = fArchData->pgdir_virt;

	start = ROUNDDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE(("unmap_tmap: asked to free pages 0x%lx to 0x%lx\n", start, end));

restart:
	if (start >= end)
		return B_OK;

	int index = VADDR_TO_PDENT(start);
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		goto restart;
	}

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = fArchData->page_mapper->GetPageTableAt(
		pd[index]  & X86_PDE_ADDRESS_MASK);

	for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
			index++, start += B_PAGE_SIZE) {
		if ((pt[index] & X86_PTE_PRESENT) == 0) {
			// page mapping not valid
			continue;
		}

		TRACE(("unmap_tmap: removing page 0x%lx\n", start));

		page_table_entry oldEntry = clear_page_table_entry_flags(&pt[index],
			X86_PTE_PRESENT);
		fMapCount--;

		if ((oldEntry & X86_PTE_ACCESSED) != 0) {
			// Note, that we only need to invalidate the address, if the
			// accessed flags was set, since only then the entry could have been
			// in any TLB.
			if (fArchData->num_invalidate_pages
					< PAGE_INVALIDATE_CACHE_SIZE) {
				fArchData->pages_to_invalidate[
					fArchData->num_invalidate_pages] = start;
			}

			fArchData->num_invalidate_pages++;
		}
	}

	pinner.Unlock();

	goto restart;
}


status_t
X86VMTranslationMap::Query(addr_t va, addr_t *_physical, uint32 *_flags)
{
	// default the flags to not present
	*_flags = 0;
	*_physical = 0;

	int index = VADDR_TO_PDENT(va);
	page_directory_entry *pd = fArchData->pgdir_virt;
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = fArchData->page_mapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);
	page_table_entry entry = pt[VADDR_TO_PTENT(va)];

	*_physical = entry & X86_PDE_ADDRESS_MASK;

	// read in the page state flags
	if ((entry & X86_PTE_USER) != 0) {
		*_flags |= ((entry & X86_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & X86_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & X86_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & X86_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & X86_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	pinner.Unlock();

	TRACE(("query_tmap: returning pa 0x%lx for va 0x%lx\n", *_physical, va));

	return B_OK;
}


status_t
X86VMTranslationMap::QueryInterrupt(addr_t va, addr_t *_physical,
	uint32 *_flags)
{
	*_flags = 0;
	*_physical = 0;

	int index = VADDR_TO_PDENT(va);
	page_directory_entry* pd = fArchData->pgdir_virt;
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	// map page table entry
	page_table_entry* pt = sPhysicalPageMapper->InterruptGetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);
	page_table_entry entry = pt[VADDR_TO_PTENT(va)];

	*_physical = entry & X86_PDE_ADDRESS_MASK;

	// read in the page state flags
	if ((entry & X86_PTE_USER) != 0) {
		*_flags |= ((entry & X86_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & X86_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & X86_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & X86_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & X86_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	return B_OK;
}


addr_t
X86VMTranslationMap::MappedSize() const
{
	return fMapCount;
}


status_t
X86VMTranslationMap::Protect(addr_t start, addr_t end, uint32 attributes)
{
	page_directory_entry *pd = fArchData->pgdir_virt;

	start = ROUNDDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE(("protect_tmap: pages 0x%lx to 0x%lx, attributes %lx\n", start, end,
		attributes));

restart:
	if (start >= end)
		return B_OK;

	int index = VADDR_TO_PDENT(start);
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		goto restart;
	}

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = fArchData->page_mapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);

	for (index = VADDR_TO_PTENT(start); index < 1024 && start < end;
			index++, start += B_PAGE_SIZE) {
		page_table_entry entry = pt[index];
		if ((entry & X86_PTE_PRESENT) == 0) {
			// page mapping not valid
			continue;
		}

		TRACE(("protect_tmap: protect page 0x%lx\n", start));

		entry &= ~(X86_PTE_WRITABLE | X86_PTE_USER);
		if ((attributes & B_USER_PROTECTION) != 0) {
			entry |= X86_PTE_USER;
			if ((attributes & B_WRITE_AREA) != 0)
				entry |= X86_PTE_WRITABLE;
		} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
			entry |= X86_PTE_WRITABLE;

		page_table_entry oldEntry = set_page_table_entry(&pt[index], entry);
			// TODO: We might have cleared accessed/modified flags!

		if ((oldEntry & X86_PTE_ACCESSED) != 0) {
			// Note, that we only need to invalidate the address, if the
			// accessed flags was set, since only then the entry could have been
			// in any TLB.
			if (fArchData->num_invalidate_pages
					< PAGE_INVALIDATE_CACHE_SIZE) {
				fArchData->pages_to_invalidate[
					fArchData->num_invalidate_pages] = start;
			}

			fArchData->num_invalidate_pages++;
		}
	}

	pinner.Unlock();

	goto restart;
}


status_t
X86VMTranslationMap::ClearFlags(addr_t va, uint32 flags)
{
	int index = VADDR_TO_PDENT(va);
	page_directory_entry* pd = fArchData->pgdir_virt;
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	uint32 flagsToClear = ((flags & PAGE_MODIFIED) ? X86_PTE_DIRTY : 0)
		| ((flags & PAGE_ACCESSED) ? X86_PTE_ACCESSED : 0);

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = fArchData->page_mapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	// clear out the flags we've been requested to clear
	page_table_entry oldEntry
		= clear_page_table_entry_flags(&pt[index], flagsToClear);

	pinner.Unlock();

	if ((oldEntry & flagsToClear) != 0) {
		if (fArchData->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE) {
			fArchData->pages_to_invalidate[
				fArchData->num_invalidate_pages] = va;
		}

		fArchData->num_invalidate_pages++;
	}

	return B_OK;
}


void
X86VMTranslationMap::Flush()
{
	if (fArchData->num_invalidate_pages <= 0)
		return;

	struct thread* thread = thread_get_current_thread();
	thread_pin_to_current_cpu(thread);

	if (fArchData->num_invalidate_pages > PAGE_INVALIDATE_CACHE_SIZE) {
		// invalidate all pages
		TRACE(("flush_tmap: %d pages to invalidate, invalidate all\n",
			fArchData->num_invalidate_pages));

		if (IS_KERNEL_MAP(map)) {
			arch_cpu_global_TLB_invalidate();
			smp_send_broadcast_ici(SMP_MSG_GLOBAL_INVALIDATE_PAGES, 0, 0, 0,
				NULL, SMP_MSG_FLAG_SYNC);
		} else {
			cpu_status state = disable_interrupts();
			arch_cpu_user_TLB_invalidate();
			restore_interrupts(state);

			int cpu = smp_get_current_cpu();
			uint32 cpuMask = fArchData->active_on_cpus
				& ~((uint32)1 << cpu);
			if (cpuMask != 0) {
				smp_send_multicast_ici(cpuMask, SMP_MSG_USER_INVALIDATE_PAGES,
					0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
			}
		}
	} else {
		TRACE(("flush_tmap: %d pages to invalidate, invalidate list\n",
			fArchData->num_invalidate_pages));

		arch_cpu_invalidate_TLB_list(fArchData->pages_to_invalidate,
			fArchData->num_invalidate_pages);

		if (IS_KERNEL_MAP(map)) {
			smp_send_broadcast_ici(SMP_MSG_INVALIDATE_PAGE_LIST,
				(uint32)fArchData->pages_to_invalidate,
				fArchData->num_invalidate_pages, 0, NULL,
				SMP_MSG_FLAG_SYNC);
		} else {
			int cpu = smp_get_current_cpu();
			uint32 cpuMask = fArchData->active_on_cpus
				& ~((uint32)1 << cpu);
			if (cpuMask != 0) {
				smp_send_multicast_ici(cpuMask, SMP_MSG_INVALIDATE_PAGE_LIST,
					(uint32)fArchData->pages_to_invalidate,
					fArchData->num_invalidate_pages, 0, NULL,
					SMP_MSG_FLAG_SYNC);
			}
		}
	}
	fArchData->num_invalidate_pages = 0;

	thread_unpin_from_current_cpu(thread);
}


// #pragma mark - VM API


status_t
arch_vm_translation_map_create_map(bool kernel, VMTranslationMap** _map)
{
	X86VMTranslationMap* map = new(std::nothrow) X86VMTranslationMap;
	if (map == NULL)
		return B_NO_MEMORY;

	status_t error = map->Init(kernel);
	if (error != B_OK) {
		delete map;
		return error;
	}

	*_map = map;
	return B_OK;
}


status_t
arch_vm_translation_map_init(kernel_args *args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	TRACE(("vm_translation_map_init: entry\n"));

	// page hole set up in stage2
	sPageHole = (page_table_entry *)args->arch_args.page_hole;
	// calculate where the pgdir would be
	sPageHolePageDir = (page_directory_entry*)
		(((addr_t)args->arch_args.page_hole)
			+ (B_PAGE_SIZE * 1024 - B_PAGE_SIZE));
	// clear out the bottom 2 GB, unmap everything
	memset(sPageHolePageDir + FIRST_USER_PGDIR_ENT, 0,
		sizeof(page_directory_entry) * NUM_USER_PGDIR_ENTS);

	sKernelPhysicalPageDirectory = (page_directory_entry*)
		args->arch_args.phys_pgdir;
	sKernelVirtualPageDirectory = (page_directory_entry*)
		args->arch_args.vir_pgdir;

	B_INITIALIZE_SPINLOCK(&sTMapListLock);
	new (&sTMapList) ArchTMapList;

	large_memory_physical_page_ops_init(args, sPhysicalPageMapper,
		sKernelPhysicalPageMapper);
		// TODO: Select the best page mapper!

	// enable global page feature if available
	if (x86_check_feature(IA32_FEATURE_PGE, FEATURE_COMMON)) {
		// this prevents kernel pages from being flushed from TLB on
		// context-switch
		x86_write_cr4(x86_read_cr4() | IA32_CR4_GLOBAL_PAGES);
	}

	TRACE(("vm_translation_map_init: done\n"));

	*_physicalPageMapper = sPhysicalPageMapper;
	return B_OK;
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	return B_OK;
}


status_t
arch_vm_translation_map_init_post_area(kernel_args *args)
{
	// now that the vm is initialized, create a region that represents
	// the page hole
	void *temp;
	status_t error;
	area_id area;

	TRACE(("vm_translation_map_init_post_area: entry\n"));

	// unmap the page hole hack we were using before
	sKernelVirtualPageDirectory[1023] = 0;
	sPageHolePageDir = NULL;
	sPageHole = NULL;

	temp = (void *)sKernelVirtualPageDirectory;
	area = create_area("kernel_pgdir", &temp, B_EXACT_ADDRESS, B_PAGE_SIZE,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	error = sPhysicalPageMapper->InitPostArea(args);
	if (error != B_OK)
		return error;

	TRACE(("vm_translation_map_init_post_area: done\n"));
	return B_OK;
}


// XXX horrible back door to map a page quickly regardless of translation map
// object, etc.
// used only during VM setup.
// uses a 'page hole' set up in the stage 2 bootloader. The page hole is created
// by pointing one of the pgdir entries back at itself, effectively mapping the
// contents of all of the 4MB of pagetables into a 4 MB region. It's only used
// here, and is later unmapped.

status_t
arch_vm_translation_map_early_map(kernel_args *args, addr_t va, addr_t pa,
	uint8 attributes, addr_t (*get_free_page)(kernel_args *))
{
	int index;

	TRACE(("early_tmap: entry pa 0x%lx va 0x%lx\n", pa, va));

	// check to see if a page table exists for this range
	index = VADDR_TO_PDENT(va);
	if ((sPageHolePageDir[index] & X86_PDE_PRESENT) == 0) {
		addr_t pgtable;
		page_directory_entry *e;
		// we need to allocate a pgtable
		pgtable = get_free_page(args);
		// pgtable is in pages, convert to physical address
		pgtable *= B_PAGE_SIZE;

		TRACE(("early_map: asked for free page for pgtable. 0x%lx\n", pgtable));

		// put it in the pgdir
		e = &sPageHolePageDir[index];
		x86_put_pgtable_in_pgdir(e, pgtable, attributes);

		// zero it out in it's new mapping
		memset((unsigned int*)((addr_t)sPageHole
			+ (va / B_PAGE_SIZE / 1024) * B_PAGE_SIZE), 0, B_PAGE_SIZE);
	}

	ASSERT((sPageHole[va / B_PAGE_SIZE] & X86_PTE_PRESENT) == 0);

	// now, fill in the pentry
	put_page_table_entry_in_pgtable(sPageHole + va / B_PAGE_SIZE, pa,
		attributes, IS_KERNEL_ADDRESS(va));

	return B_OK;
}


/*!	Verifies that the page at the given virtual address can be accessed in the
	current context.

	This function is invoked in the kernel debugger. Paranoid checking is in
	order.

	\param virtualAddress The virtual address to be checked.
	\param protection The area protection for which to check. Valid is a bitwise
		or of one or more of \c B_KERNEL_READ_AREA or \c B_KERNEL_WRITE_AREA.
	\return \c true, if the address can be accessed in all ways specified by
		\a protection, \c false otherwise.
*/
bool
arch_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection)
{
	// We only trust the kernel team's page directory. So switch to it first.
	// Always set it to make sure the TLBs don't contain obsolete data.
	addr_t physicalPageDirectory;
	read_cr3(physicalPageDirectory);
	write_cr3(sKernelPhysicalPageDirectory);

	// get the page directory entry for the address
	page_directory_entry pageDirectoryEntry;
	uint32 index = VADDR_TO_PDENT(virtualAddress);

	if (physicalPageDirectory == (addr_t)sKernelPhysicalPageDirectory) {
		pageDirectoryEntry = sKernelVirtualPageDirectory[index];
	} else {
		// map the original page directory and get the entry
		void* handle;
		addr_t virtualPageDirectory;
		status_t error = sPhysicalPageMapper->GetPageDebug(
			physicalPageDirectory, &virtualPageDirectory, &handle);
		if (error == B_OK) {
			pageDirectoryEntry
				= ((page_directory_entry*)virtualPageDirectory)[index];
			sPhysicalPageMapper->PutPageDebug(virtualPageDirectory,
				handle);
		} else
			pageDirectoryEntry = 0;
	}

	// map the page table and get the entry
	page_table_entry pageTableEntry;
	index = VADDR_TO_PTENT(virtualAddress);

	if ((pageDirectoryEntry & X86_PDE_PRESENT) != 0) {
		void* handle;
		addr_t virtualPageTable;
		status_t error = sPhysicalPageMapper->GetPageDebug(
			pageDirectoryEntry & X86_PDE_ADDRESS_MASK, &virtualPageTable,
			&handle);
		if (error == B_OK) {
			pageTableEntry = ((page_table_entry*)virtualPageTable)[index];
			sPhysicalPageMapper->PutPageDebug(virtualPageTable, handle);
		} else
			pageTableEntry = 0;
	} else
		pageTableEntry = 0;

	// switch back to the original page directory
	if (physicalPageDirectory != (addr_t)sKernelPhysicalPageDirectory)
		write_cr3(physicalPageDirectory);

	if ((pageTableEntry & X86_PTE_PRESENT) == 0)
		return false;

	// present means kernel-readable, so check for writable
	return (protection & B_KERNEL_WRITE_AREA) == 0
		|| (pageTableEntry & X86_PTE_WRITABLE) != 0;
}
