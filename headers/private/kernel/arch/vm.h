/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_ARCH_VM_H
#define KERNEL_ARCH_VM_H


#include <arch_vm.h>

#include <SupportDefs.h>


struct kernel_args;
struct vm_area;
struct vm_address_space;


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_vm_init(struct kernel_args *args);
status_t arch_vm_init_post_area(struct kernel_args *args);
status_t arch_vm_init_end(struct kernel_args *args);
status_t arch_vm_init_post_modules(struct kernel_args *args);
void arch_vm_aspace_swap(struct vm_address_space *from,
	struct vm_address_space *to);
bool arch_vm_supports_protection(uint32 protection);

status_t arch_vm_set_memory_type(struct vm_area *area, addr_t physicalBase,
	uint32 type);
void arch_vm_unset_memory_type(struct vm_area *area);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_VM_H */
