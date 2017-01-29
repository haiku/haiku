/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "generic_vm_physical_page_mapper.h"

#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <thread.h>
#include <util/queue.h>

#include <string.h>
#include <stdlib.h>

//#define TRACE_VM_PHYSICAL_PAGE_MAPPER
#ifdef TRACE_VM_PHYSICAL_PAGE_MAPPER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define DEBUG_IO_SPACE

// data and structures used to represent physical pages mapped into iospace
typedef struct paddr_chunk_descriptor {
	struct paddr_chunk_descriptor *next_q;
		// must remain first in structure, queue code uses it
	int32	ref_count;
	addr_t	va;
#ifdef DEBUG_IO_SPACE
	thread_id last_ref;
#endif
} paddr_chunk_desc;

static paddr_chunk_desc *paddr_desc;         // will be one per physical chunk
static paddr_chunk_desc **virtual_pmappings; // will be one ptr per virtual chunk in iospace
static int first_free_vmapping;
static int num_virtual_chunks;
static queue mapped_paddr_lru;
static mutex sMutex = MUTEX_INITIALIZER("iospace_mutex");
static sem_id sChunkAvailableSem;
static int32 sChunkAvailableWaitingCounter;

static generic_map_iospace_chunk_func sMapIOSpaceChunk;
static addr_t sIOSpaceBase;
static size_t sIOSpaceSize;
static size_t sIOSpaceChunkSize;


status_t
generic_get_physical_page(phys_addr_t pa, addr_t *va, uint32 flags)
{
	int index;
	paddr_chunk_desc *replaced_pchunk;

restart:
	mutex_lock(&sMutex);

	// see if the page is already mapped
	index = pa / sIOSpaceChunkSize;
	if (paddr_desc[index].va != 0) {
		if (paddr_desc[index].ref_count++ == 0) {
			// pull this descriptor out of the lru list
			queue_remove_item(&mapped_paddr_lru, &paddr_desc[index]);
		}
		*va = paddr_desc[index].va + pa % sIOSpaceChunkSize;
		mutex_unlock(&sMutex);
		return B_OK;
	}

	// map it
	if (first_free_vmapping < num_virtual_chunks) {
		// there's a free hole
		paddr_desc[index].va = first_free_vmapping * sIOSpaceChunkSize
			+ sIOSpaceBase;
		*va = paddr_desc[index].va + pa % sIOSpaceChunkSize;
		virtual_pmappings[first_free_vmapping] = &paddr_desc[index];
		paddr_desc[index].ref_count++;

		// push up the first_free_vmapping pointer
		for (; first_free_vmapping < num_virtual_chunks;
			 first_free_vmapping++) {
			if (virtual_pmappings[first_free_vmapping] == NULL)
				break;
		}

		sMapIOSpaceChunk(paddr_desc[index].va, index * sIOSpaceChunkSize,
			flags);
		mutex_unlock(&sMutex);

		return B_OK;
	}

	// replace an earlier mapping
	if (queue_peek(&mapped_paddr_lru) == NULL) {
		// no free slots available
		if ((flags & PHYSICAL_PAGE_DONT_WAIT) != 0) {
			// put back to the caller and let them handle this
			mutex_unlock(&sMutex);
			return B_NO_MEMORY;
		} else {
			sChunkAvailableWaitingCounter++;

			mutex_unlock(&sMutex);
			acquire_sem(sChunkAvailableSem);
			goto restart;
		}
	}

	replaced_pchunk = (paddr_chunk_desc*)queue_dequeue(&mapped_paddr_lru);
	paddr_desc[index].va = replaced_pchunk->va;
	replaced_pchunk->va = 0;
	*va = paddr_desc[index].va + pa % sIOSpaceChunkSize;
	paddr_desc[index].ref_count++;
#ifdef DEBUG_IO_SPACE
	paddr_desc[index].last_ref = thread_get_current_thread_id();
#endif
	virtual_pmappings[(*va - sIOSpaceBase) / sIOSpaceChunkSize]
		= paddr_desc + index;

	sMapIOSpaceChunk(paddr_desc[index].va, index * sIOSpaceChunkSize, flags);

	mutex_unlock(&sMutex);
	return B_OK;
}


status_t
generic_put_physical_page(addr_t va)
{
	paddr_chunk_desc *desc;

	if (va < sIOSpaceBase || va >= sIOSpaceBase + sIOSpaceSize)
		panic("someone called put_physical_page on an invalid va 0x%lx\n", va);
	va -= sIOSpaceBase;

	mutex_lock(&sMutex);

	desc = virtual_pmappings[va / sIOSpaceChunkSize];
	if (desc == NULL) {
		mutex_unlock(&sMutex);
		panic("put_physical_page called on page at va 0x%lx which is not checked out\n",
			va);
		return B_ERROR;
	}

	if (--desc->ref_count == 0) {
		// put it on the mapped lru list
		queue_enqueue(&mapped_paddr_lru, desc);

		if (sChunkAvailableWaitingCounter > 0) {
			sChunkAvailableWaitingCounter--;
			release_sem_etc(sChunkAvailableSem, 1, B_DO_NOT_RESCHEDULE);
		}
	}

	mutex_unlock(&sMutex);

	return B_OK;
}


#ifdef DEBUG_IO_SPACE
static int
dump_iospace(int argc, char** argv)
{
	if (argc < 2) {
		kprintf("usage: iospace <physical|virtual|queue>\n");
		return 0;
	}

	int32 i;

	if (strchr(argv[1], 'p')) {
		// physical address descriptors
		kprintf("I/O space physical descriptors (%p)\n", paddr_desc);

		int32 max = vm_page_num_pages() / (sIOSpaceChunkSize / B_PAGE_SIZE);
		if (argc == 3)
			max = strtol(argv[2], NULL, 0);

		for (i = 0; i < max; i++) {
			kprintf("[%03lx %p %3ld %3ld] ", i, (void *)paddr_desc[i].va,
				paddr_desc[i].ref_count, paddr_desc[i].last_ref);
			if (i % 4 == 3)
				kprintf("\n");
		}
		if (i % 4)
			kprintf("\n");
	}

	if (strchr(argv[1], 'v')) {
		// virtual mappings
		kprintf("I/O space virtual chunk mappings (%p, first free: %d)\n",
			virtual_pmappings, first_free_vmapping);

		for (i = 0; i < num_virtual_chunks; i++) {
			kprintf("[%2ld. %03lx] ", i, virtual_pmappings[i] - paddr_desc);
			if (i % 8 == 7)
				kprintf("\n");
		}
		if (i % 8)
			kprintf("\n");
	}

	if (strchr(argv[1], 'q')) {
		// unused queue
		kprintf("I/O space mapped queue:\n");

		paddr_chunk_descriptor* descriptor
			= (paddr_chunk_descriptor *)queue_peek(&mapped_paddr_lru);
		i = 0;

		while (descriptor != NULL) {
			kprintf("[%03lx %p] ",
				(descriptor - paddr_desc) / sizeof(paddr_desc[0]), descriptor);
			if (i++ % 8 == 7)
				kprintf("\n");

			descriptor = descriptor->next_q;
		}
		if (i % 8)
			kprintf("\n");
	}

	return 0;
}
#endif


//	#pragma mark -


status_t
generic_vm_physical_page_mapper_init(kernel_args *args,
	generic_map_iospace_chunk_func mapIOSpaceChunk, addr_t *ioSpaceBase,
	size_t ioSpaceSize, size_t ioSpaceChunkSize)
{
	TRACE(("generic_vm_physical_page_mapper_init: entry\n"));

	sMapIOSpaceChunk = mapIOSpaceChunk;
	sIOSpaceSize = ioSpaceSize;
	sIOSpaceChunkSize = ioSpaceChunkSize;

	// reserve virtual space for the IO space
	sIOSpaceBase = vm_allocate_early(args, sIOSpaceSize, 0, 0,
		ioSpaceChunkSize);
	if (sIOSpaceBase == 0) {
		panic("generic_vm_physical_page_mapper_init(): Failed to reserve IO "
			"space in virtual address space!");
		return B_ERROR;
	}

	*ioSpaceBase = sIOSpaceBase;

	// allocate some space to hold physical page mapping info
	paddr_desc = (paddr_chunk_desc *)vm_allocate_early(args,
		sizeof(paddr_chunk_desc) * 1024, ~0L,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0);
	num_virtual_chunks = sIOSpaceSize / sIOSpaceChunkSize;
	virtual_pmappings = (paddr_chunk_desc **)vm_allocate_early(args,
		sizeof(paddr_chunk_desc *) * num_virtual_chunks, ~0L,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0);

	TRACE(("paddr_desc %p, virtual_pmappings %p"/*", iospace_pgtables %p"*/"\n",
		paddr_desc, virtual_pmappings/*, iospace_pgtables*/));

	// initialize our data structures
	memset(paddr_desc, 0, sizeof(paddr_chunk_desc) * 1024);
	memset(virtual_pmappings, 0, sizeof(paddr_chunk_desc *) * num_virtual_chunks);
	first_free_vmapping = 0;
	queue_init(&mapped_paddr_lru);
	sChunkAvailableSem = -1;

	TRACE(("generic_vm_physical_page_mapper_init: done\n"));

	return B_OK;
}


status_t
generic_vm_physical_page_mapper_init_post_area(kernel_args *args)
{
	void *temp;

	TRACE(("generic_vm_physical_page_mapper_init_post_area: entry\n"));

	temp = (void *)paddr_desc;
	create_area("physical_page_mapping_descriptors", &temp, B_EXACT_ADDRESS,
		ROUNDUP(sizeof(paddr_chunk_desc) * 1024, B_PAGE_SIZE),
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	temp = (void *)virtual_pmappings;
	create_area("iospace_virtual_chunk_descriptors", &temp, B_EXACT_ADDRESS,
		ROUNDUP(sizeof(paddr_chunk_desc *) * num_virtual_chunks, B_PAGE_SIZE),
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	TRACE(("generic_vm_physical_page_mapper_init_post_area: creating iospace\n"));
	temp = (void *)sIOSpaceBase;
	area_id ioSpaceArea = vm_create_null_area(VMAddressSpace::KernelID(),
		"iospace", &temp, B_EXACT_ADDRESS, sIOSpaceSize,
		CREATE_AREA_PRIORITY_VIP);
	if (ioSpaceArea < 0) {
		panic("generic_vm_physical_page_mapper_init_post_area(): Failed to "
			"create null area for IO space!\n");
		return ioSpaceArea;
	}

	TRACE(("generic_vm_physical_page_mapper_init_post_area: done\n"));

#ifdef DEBUG_IO_SPACE
	add_debugger_command("iospace", &dump_iospace, "Shows info about the I/O space area.");
#endif

	return B_OK;
}


status_t
generic_vm_physical_page_mapper_init_post_sem(kernel_args *args)
{
	sChunkAvailableSem = create_sem(1, "iospace chunk available");

	return sChunkAvailableSem >= B_OK ? B_OK : sChunkAvailableSem;
}
