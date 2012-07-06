/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_X86_PAGING_STRUCTURES_H
#define KERNEL_ARCH_X86_PAGING_X86_PAGING_STRUCTURES_H


#include <SupportDefs.h>

#include <heap.h>


struct X86PagingStructures : DeferredDeletable {
	phys_addr_t					pgdir_phys;
	vint32						ref_count;
	vint32						active_on_cpus;
		// mask indicating on which CPUs the map is currently used

								X86PagingStructures();
	virtual						~X86PagingStructures();

	inline	void				AddReference();
	inline	void				RemoveReference();

	virtual	void				Delete() = 0;
};


inline void
X86PagingStructures::AddReference()
{
	atomic_add(&ref_count, 1);
}


inline void
X86PagingStructures::RemoveReference()
{
	if (atomic_add(&ref_count, -1) == 1)
		Delete();
}


#endif	// KERNEL_ARCH_X86_PAGING_X86_PAGING_STRUCTURES_H
