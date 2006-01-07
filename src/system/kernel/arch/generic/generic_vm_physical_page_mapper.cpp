/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "generic_vm_physical_page_mapper.h"

#include <vm_address_space.h>
#include <vm_page.h>
#include <vm_priv.h>
#include <smp.h>
#include <util/queue.h>
#include <memheap.h>

#include <string.h>
#include <stdlib.h>

//#define TRACE_VM_PHYSICAL_PAGE_MAPPER
#ifdef TRACE_VM_PHYSICAL_PAGE_MAPPER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// data and structures used to represent physical pages mapped into iospace
typedef struct paddr_chunk_descriptor {
	struct paddr_chunk_descriptor *next_q;
		// must remain first in structure, queue code uses it
	int ref_count;
	addr_t va;
} paddr_chunk_desc;

static paddr_chunk_desc *paddr_desc;         // will be one per physical chunk
static paddr_chunk_desc **virtual_pmappings; // will be one ptr per virtual chunk in iospace
static int first_free_vmapping;
static int num_virtual_chunks;
static queue mapped_paddr_lru;
static mutex iospace_mutex;
static sem_id iospace_full_sem;

static generic_map_iospace_chunk_func sMapIOSpaceChunk;
static addr_t sIOSpaceBase;
static size_t sIOSpaceSize;
static size_t sIOSpaceChunkSize;


status_t
generic_get_physical_page(addr_t pa, addr_t *va, uint32 flags)
{
	int index;
	paddr_chunk_desc *replaced_pchunk;

restart:
	mutex_lock(&iospace_mutex);

	// see if the page is already mapped
	index = pa / sIOSpaceChunkSize;
	if (paddr_desc[index].va != 0) {
		if (paddr_desc[index].ref_count++ == 0) {
			// pull this descriptor out of the lru list
			queue_remove_item(&mapped_paddr_lru, &paddr_desc[index]);
		}
		*va = paddr_desc[index].va + pa % sIOSpaceChunkSize;
		mutex_unlock(&iospace_mutex);
		return B_OK;
	}

	// map it
	if (first_free_vmapping < num_virtual_chunks) {
		// there's a free hole
		paddr_desc[index].va = first_free_vmapping * sIOSpaceChunkSize + sIOSpaceBase;
		*va = paddr_desc[index].va + pa % sIOSpaceChunkSize;
		virtual_pmappings[first_free_vmapping] = &paddr_desc[index];
		paddr_desc[index].ref_count++;

		// push up the first_free_vmapping pointer
		for (; first_free_vmapping < num_virtual_chunks; first_free_vmapping++) {
			if(virtual_pmappings[first_free_vmapping] == NULL)
				break;
		}

		sMapIOSpaceChunk(paddr_desc[index].va,
			index * sIOSpaceChunkSize);
		mutex_unlock(&iospace_mutex);

		return B_OK;
	}

	// replace an earlier mapping
	if (queue_peek(&mapped_paddr_lru) == NULL) {
		// no free slots available
		if (flags == PHYSICAL_PAGE_NO_WAIT) {
			// punt back to the caller and let them handle this
			mutex_unlock(&iospace_mutex);
			return B_NO_MEMORY;
		} else {
			mutex_unlock(&iospace_mutex);
			acquire_sem(iospace_full_sem);
			goto restart;
		}
	}

	replaced_pchunk = (paddr_chunk_desc*)queue_dequeue(&mapped_paddr_lru);
	paddr_desc[index].va = replaced_pchunk->va;
	replaced_pchunk->va = 0;
	*va = paddr_desc[index].va + pa % sIOSpaceChunkSize;
	paddr_desc[index].ref_count++;

	sMapIOSpaceChunk(paddr_desc[index].va, index * sIOSpaceChunkSize);

	mutex_unlock(&iospace_mutex);
	return B_OK;
}


status_t
generic_put_physical_page(addr_t va)
{
	paddr_chunk_desc *desc;

	if (va < sIOSpaceBase || va >= sIOSpaceBase + sIOSpaceSize)
		panic("someone called put_physical_page on an invalid va 0x%lx\n", va);
	va -= sIOSpaceBase;

	mutex_lock(&iospace_mutex);

	desc = virtual_pmappings[va / sIOSpaceChunkSize];
	if (desc == NULL) {
		mutex_unlock(&iospace_mutex);
		panic("put_physical_page called on page at va 0x%lx which is not checked out\n", va);
		return B_ERROR;
	}

	if (--desc->ref_count == 0) {
		// put it on the mapped lru list
		queue_enqueue(&mapped_paddr_lru, desc);
		// no sense rescheduling on this one, there's likely a race in the waiting
		// thread to grab the iospace_mutex, which would block and eventually get back to
		// this thread. waste of time.
		release_sem_etc(iospace_full_sem, 1, B_DO_NOT_RESCHEDULE);
	}

	mutex_unlock(&iospace_mutex);

	return B_OK;
}


//	#pragma mark -
//	VM API


status_t
generic_vm_physical_page_mapper_init(kernel_args *args,
	generic_map_iospace_chunk_func mapIOSpaceChunk, addr_t *ioSpaceBase,
	size_t ioSpaceSize, size_t ioSpaceChunkSize)
{
	TRACE(("generic_vm_physical_page_mapper_init: entry\n"));

	sMapIOSpaceChunk = mapIOSpaceChunk;
	sIOSpaceBase = *ioSpaceBase;
	sIOSpaceSize = ioSpaceSize;
	sIOSpaceChunkSize = ioSpaceChunkSize;

	// allocate some space to hold physical page mapping info
	paddr_desc = (paddr_chunk_desc *)vm_alloc_from_kernel_args(args,
		sizeof(paddr_chunk_desc) * 1024, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	num_virtual_chunks = sIOSpaceSize / sIOSpaceChunkSize;
	virtual_pmappings = (paddr_chunk_desc **)vm_alloc_from_kernel_args(args,
		sizeof(paddr_chunk_desc *) * num_virtual_chunks, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	TRACE(("paddr_desc %p, virtual_pmappings %p"/*", iospace_pgtables %p"*/"\n",
		paddr_desc, virtual_pmappings/*, iospace_pgtables*/));

	// initialize our data structures
	memset(paddr_desc, 0, sizeof(paddr_chunk_desc) * 1024);
	memset(virtual_pmappings, 0, sizeof(paddr_chunk_desc *) * num_virtual_chunks);
	first_free_vmapping = 0;
	queue_init(&mapped_paddr_lru);
	iospace_mutex.sem = -1;
	iospace_mutex.holder = -1;
	iospace_full_sem = -1;

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
	area_id ioSpaceArea = vm_create_null_area(vm_kernel_address_space_id(),
		"iospace", &temp, B_EXACT_ADDRESS, sIOSpaceSize);
	// TODO: We don't reserve the virtual address space for the IO space in
	// generic_vm_physical_page_mapper_init() yet. So theoretically it could
	// happen that a part of that space has been reserved by someone else in
	// the meantime.
	if (ioSpaceArea < 0) {
		panic("generic_vm_physical_page_mapper_init_post_area(): Failed to "
			"create null area for IO space!\n");
		return ioSpaceArea;
	}

	TRACE(("generic_vm_physical_page_mapper_init_post_area: done\n"));

	return B_OK;
}


status_t
generic_vm_physical_page_mapper_init_post_sem(kernel_args *args)
{
	mutex_init(&iospace_mutex, "iospace_mutex");
	iospace_full_sem = create_sem(1, "iospace_full_sem");

	return iospace_full_sem >= B_OK ? B_OK : iospace_full_sem;
}
