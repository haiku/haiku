/*
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>

#include <kernel.h>
#include <boot/kernel_args.h>

#include <vm/vm.h>
#include <vm/vm_types.h>
#include <arch/vm.h>
//#include <arch_mmu.h>


//#define TRACE_ARCH_VM
#ifdef TRACE_ARCH_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


status_t
arch_vm_init(kernel_args *args)
{
	TRACE(("arch_vm_init: entry\n"));
	return B_OK;
}


status_t
arch_vm_init2(kernel_args *args)
{
	return B_OK;
}


status_t
arch_vm_init_post_area(kernel_args *args)
{
	TRACE(("arch_vm_init_post_area: entry\n"));
	return B_OK;
}


status_t
arch_vm_init_end(kernel_args *args)
{
	// Throw away all mappings that are unused by the kernel
	vm_free_unused_boot_loader_range(KERNEL_LOAD_BASE, KERNEL_SIZE);

	return B_OK;
}


status_t
arch_vm_init_post_modules(kernel_args *args)
{
	return B_OK;
}


void
arch_vm_aspace_swap(struct VMAddressSpace *from, struct VMAddressSpace *to)
{
	// This functions is only invoked when a userland thread is in the process
	// of dying. It switches to the kernel team and does whatever cleanup is
	// necessary (in case it is the team's main thread, it will delete the
	// team).
	// It is however not necessary to change the page directory. Userland team's
	// page directories include all kernel mappings as well. Furthermore our
	// arch specific translation map data objects are ref-counted, so they won't
	// go away as long as they are still used on any CPU.
}


bool
arch_vm_supports_protection(uint32 protection)
{
	// TODO check ARM protection possibilities
	return true;
}


void
arch_vm_unset_memory_type(VMArea *area)
{
	// TODO
}


status_t
arch_vm_set_memory_type(VMArea *area, phys_addr_t physicalBase, uint32 type)
{
	if (type != 0)
		dprintf("%s: undefined type %lx!\n", __PRETTY_FUNCTION__, type);

	return B_OK;
}
