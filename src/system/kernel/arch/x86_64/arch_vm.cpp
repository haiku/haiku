/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <vm/vm.h>

#include <arch/vm.h>


status_t
arch_vm_init(kernel_args *args)
{
	return B_OK;
}


status_t
arch_vm_init_post_area(kernel_args *args)
{
	return B_OK;
}


status_t
arch_vm_init_end(kernel_args *args)
{
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

}


bool
arch_vm_supports_protection(uint32 protection)
{
	return true;
}


void
arch_vm_unset_memory_type(struct VMArea *area)
{

}


status_t
arch_vm_set_memory_type(struct VMArea *area, phys_addr_t physicalBase,
	uint32 type)
{
	return B_OK;
}
