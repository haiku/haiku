/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_ARCH_VM_H
#define KERNEL_ARCH_VM_H


#include <vm.h>
#include <arch_vm.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_vm_init(struct kernel_args *args);
status_t arch_vm_init_post_area(struct kernel_args *args);
status_t arch_vm_init_end(struct kernel_args *args);
void arch_vm_aspace_swap(vm_address_space *aspace);
bool arch_vm_supports_protection(uint32 protection);
void arch_vm_init_area(vm_area *area);

status_t arch_vm_set_memory_type(vm_area *area, uint32 type);
void arch_vm_unset_memory_type(vm_area *area);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_VM_H */
