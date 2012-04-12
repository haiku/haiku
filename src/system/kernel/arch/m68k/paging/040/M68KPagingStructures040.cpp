/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/040/M68KPagingStructures040.h"

#include <stdlib.h>

#include <heap.h>
#include <util/AutoLock.h>


// Accessor class to reuse the SinglyLinkedListLink of DeferredDeletable for
// M68KPagingStructures040.
struct PagingStructuresGetLink {
private:
	typedef SinglyLinkedListLink<M68KPagingStructures040> Link;

public:
	inline Link* operator()(M68KPagingStructures040* element) const
	{
		return (Link*)element->GetSinglyLinkedListLink();
	}

	inline const Link* operator()(
		const M68KPagingStructures040* element) const
	{
		return (const Link*)element->GetSinglyLinkedListLink();
	}
};


typedef SinglyLinkedList<M68KPagingStructures040, PagingStructuresGetLink>
	PagingStructuresList;


static PagingStructuresList sPagingStructuresList;
static spinlock sPagingStructuresListLock;


M68KPagingStructures040::M68KPagingStructures040()
	:
	pgroot_virt(NULL)
{
}


M68KPagingStructures040::~M68KPagingStructures040()
{
	// free the page dir
	free(pgroot_virt);
}


void
M68KPagingStructures040::Init(page_root_entry* virtualPageRoot,
	phys_addr_t physicalPageRoot, page_root_entry* kernelPageRoot)
{
	pgroot_virt = virtualPageRoot;
	pgroot_phys = physicalPageRoot;

	// zero out the bottom portion of the new pgroot
	memset(pgroot_virt + FIRST_USER_PGROOT_ENT, 0,
		NUM_USER_PGROOT_ENTS * sizeof(page_root_entry));

	// insert this new map into the map list
	{
		int state = disable_interrupts();
		acquire_spinlock(&sPagingStructuresListLock);

		// copy the top portion of the page dir from the kernel page dir
		if (kernelPageRoot != NULL) {
			memcpy(pgroot_virt + FIRST_KERNEL_PGROOT_ENT,
				kernelPageRoot + FIRST_KERNEL_PGROOT_ENT,
				NUM_KERNEL_PGROOT_ENTS * sizeof(page_root_entry));
		}

		sPagingStructuresList.Add(this);

		release_spinlock(&sPagingStructuresListLock);
		restore_interrupts(state);
	}
}


void
M68KPagingStructures040::Delete()
{
	// remove from global list
	InterruptsSpinLocker locker(sPagingStructuresListLock);
	sPagingStructuresList.Remove(this);
	locker.Unlock();

#if 0
	// this sanity check can be enabled when corruption due to
	// overwriting an active page directory is suspected
	uint32 activePageDirectory;
	read_cr3(activePageDirectory);
	if (activePageDirectory == pgdir_phys)
		panic("deleting a still active page directory\n");
#endif

	if (are_interrupts_enabled())
		delete this;
	else
		deferred_delete(this);
}


/*static*/ void
M68KPagingStructures040::StaticInit()
{
	B_INITIALIZE_SPINLOCK(&sPagingStructuresListLock);
	new (&sPagingStructuresList) PagingStructuresList;
}


/*static*/ void
M68KPagingStructures040::UpdateAllPageDirs(int index,
	page_root_entry entry)
{
#warning M68K: TODO: allocate all kernel pgdirs at boot and remove this (also dont remove them anymore from unmap)
#warning M68K:FIXME
	InterruptsSpinLocker locker(sPagingStructuresListLock);

	PagingStructuresList::Iterator it = sPagingStructuresList.GetIterator();
	while (M68KPagingStructures040* info = it.Next())
		info->pgroot_virt[index] = entry;
}
