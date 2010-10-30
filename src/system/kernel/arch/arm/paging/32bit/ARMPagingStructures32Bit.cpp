/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/32bit/ARMPagingStructures32Bit.h"

#include <stdlib.h>

#include <heap.h>
#include <util/AutoLock.h>


// Accessor class to reuse the SinglyLinkedListLink of DeferredDeletable for
// ARMPagingStructures32Bit.
struct PagingStructuresGetLink {
private:
	typedef SinglyLinkedListLink<ARMPagingStructures32Bit> Link;

public:
	inline Link* operator()(ARMPagingStructures32Bit* element) const
	{
		return (Link*)element->GetSinglyLinkedListLink();
	}

	inline const Link* operator()(
		const ARMPagingStructures32Bit* element) const
	{
		return (const Link*)element->GetSinglyLinkedListLink();
	}
};


typedef SinglyLinkedList<ARMPagingStructures32Bit, PagingStructuresGetLink>
	PagingStructuresList;


static PagingStructuresList sPagingStructuresList;
static spinlock sPagingStructuresListLock;


ARMPagingStructures32Bit::ARMPagingStructures32Bit()
	:
	pgdir_virt(NULL)
{
}


ARMPagingStructures32Bit::~ARMPagingStructures32Bit()
{
	// free the page dir
	free(pgdir_virt);
}


void
ARMPagingStructures32Bit::Init(page_directory_entry* virtualPageDir,
	phys_addr_t physicalPageDir, page_directory_entry* kernelPageDir)
{
	pgdir_virt = virtualPageDir;
	pgdir_phys = physicalPageDir;

#if 0 // IRA: handle UART better; identity map of DEVICE_BASE from loader gets wiped here
	// zero out the bottom portion of the new pgdir
	memset(pgdir_virt + FIRST_USER_PGDIR_ENT, 0,
		NUM_USER_PGDIR_ENTS * sizeof(page_directory_entry));
#endif
	// insert this new map into the map list
	{
		int state = disable_interrupts();
		acquire_spinlock(&sPagingStructuresListLock);

		// copy the top portion of the page dir from the kernel page dir
		if (kernelPageDir != NULL) {
			memcpy(pgdir_virt + FIRST_KERNEL_PGDIR_ENT,
				kernelPageDir + FIRST_KERNEL_PGDIR_ENT,
				NUM_KERNEL_PGDIR_ENTS * sizeof(page_directory_entry));
		}

		sPagingStructuresList.Add(this);

		release_spinlock(&sPagingStructuresListLock);
		restore_interrupts(state);
	}
}


void
ARMPagingStructures32Bit::Delete()
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
ARMPagingStructures32Bit::StaticInit()
{
	B_INITIALIZE_SPINLOCK(&sPagingStructuresListLock);
	new (&sPagingStructuresList) PagingStructuresList;
}


/*static*/ void
ARMPagingStructures32Bit::UpdateAllPageDirs(int index,
	page_directory_entry entry)
{
	InterruptsSpinLocker locker(sPagingStructuresListLock);

	PagingStructuresList::Iterator it = sPagingStructuresList.GetIterator();
	while (ARMPagingStructures32Bit* info = it.Next())
		info->pgdir_virt[index] = entry;
}
