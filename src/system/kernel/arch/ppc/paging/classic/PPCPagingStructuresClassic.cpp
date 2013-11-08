/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/classic/PPCPagingStructuresClassic.h"

#include <stdlib.h>

#include <heap.h>
#include <util/AutoLock.h>


// Accessor class to reuse the SinglyLinkedListLink of DeferredDeletable for
// PPCPagingStructuresClassic.
struct PagingStructuresGetLink {
private:
	typedef SinglyLinkedListLink<PPCPagingStructuresClassic> Link;

public:
	inline Link* operator()(PPCPagingStructuresClassic* element) const
	{
		return (Link*)element->GetSinglyLinkedListLink();
	}

	inline const Link* operator()(
		const PPCPagingStructuresClassic* element) const
	{
		return (const Link*)element->GetSinglyLinkedListLink();
	}
};


typedef SinglyLinkedList<PPCPagingStructuresClassic, PagingStructuresGetLink>
	PagingStructuresList;


static PagingStructuresList sPagingStructuresList;
static spinlock sPagingStructuresListLock;


PPCPagingStructuresClassic::PPCPagingStructuresClassic()
/*	:
	pgdir_virt(NULL)*/
{
}


PPCPagingStructuresClassic::~PPCPagingStructuresClassic()
{
#if 0//X86
	// free the page dir
	free(pgdir_virt);
#endif
}


void
PPCPagingStructuresClassic::Init(/*page_directory_entry* virtualPageDir,
	phys_addr_t physicalPageDir, page_directory_entry* kernelPageDir*/
	page_table_entry_group *pageTable)
{
//	pgdir_virt = virtualPageDir;
//	pgdir_phys = physicalPageDir;

#if 0//X86
	// zero out the bottom portion of the new pgdir
	memset(pgdir_virt + FIRST_USER_PGDIR_ENT, 0,
		NUM_USER_PGDIR_ENTS * sizeof(page_directory_entry));
#endif

	// insert this new map into the map list
	{
		int state = disable_interrupts();
		acquire_spinlock(&sPagingStructuresListLock);

#if 0//X86
		// copy the top portion of the page dir from the kernel page dir
		if (kernelPageDir != NULL) {
			memcpy(pgdir_virt + FIRST_KERNEL_PGDIR_ENT,
				kernelPageDir + FIRST_KERNEL_PGDIR_ENT,
				NUM_KERNEL_PGDIR_ENTS * sizeof(page_directory_entry));
		}
#endif

		sPagingStructuresList.Add(this);

		release_spinlock(&sPagingStructuresListLock);
		restore_interrupts(state);
	}
}


void
PPCPagingStructuresClassic::Delete()
{
	// remove from global list
	InterruptsSpinLocker locker(sPagingStructuresListLock);
	sPagingStructuresList.Remove(this);
	locker.Unlock();

#if 0
	// this sanity check can be enabled when corruption due to
	// overwriting an active page directory is suspected
	uint32 activePageDirectory = x86_read_cr3();
	if (activePageDirectory == pgdir_phys)
		panic("deleting a still active page directory\n");
#endif

	if (are_interrupts_enabled())
		delete this;
	else
		deferred_delete(this);
}


/*static*/ void
PPCPagingStructuresClassic::StaticInit()
{
	B_INITIALIZE_SPINLOCK(&sPagingStructuresListLock);
	new (&sPagingStructuresList) PagingStructuresList;
}


/*static*/ void
PPCPagingStructuresClassic::UpdateAllPageDirs(int index,
	page_table_entry_group entry)
//XXX:page_table_entry?
{
	InterruptsSpinLocker locker(sPagingStructuresListLock);
#if 0//X86
	PagingStructuresList::Iterator it = sPagingStructuresList.GetIterator();
	while (PPCPagingStructuresClassic* info = it.Next())
		info->pgdir_virt[index] = entry;
#endif
}
