/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * Copyright 2007 François Revol, revol@free.fr
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


//#define TRACE_ARCH_VM
#ifdef TRACE_ARCH_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

status_t
arch_vm_init(kernel_args* args)
{
#warning IMPLEMENT arch_vm_init
	return B_ERROR;
}


status_t
arch_vm_init2(kernel_args* args)
{
#warning IMPLEMENT arch_vm_init2
	return B_ERROR;
}


status_t
arch_vm_init_post_area(kernel_args* args)
{
#warning IMPLEMENT arch_vm_init_post_area
	return B_ERROR;
}


status_t
arch_vm_init_end(kernel_args* args)
{
#warning IMPLEMENT arch_vm_init_end
	return B_ERROR;
}


status_t
arch_vm_init_post_modules(kernel_args* args)
{
#warning IMPLEMENT arch_vm_init_post_modules
	return B_ERROR;
}


void
arch_vm_aspace_swap(struct VMAddressSpace* from, struct VMAddressSpace* to)
{
#warning IMPLEMENT arch_vm_aspace_swap
}


bool
arch_vm_supports_protection(uint32 protection)
{
#warning IMPLEMENT arch_vm_supports_protection
	return true;
}


void
arch_vm_unset_memory_type(VMArea* area)
{
#warning IMPLEMENT arch_vm_unset_memory_type
}


status_t
arch_vm_set_memory_type(VMArea* area, phys_addr_t physicalBase, uint32 type)
{
#warning IMPLEMENT arch_vm_set_memory_type
	return B_ERROR;
}

