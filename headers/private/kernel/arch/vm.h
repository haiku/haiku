/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_VM_H
#define KERNEL_ARCH_VM_H


#include <vm.h>


#ifdef __cplusplus
extern "C" {
#endif

int arch_vm_init(struct kernel_args *ka);
int arch_vm_init2(struct kernel_args *ka);
int arch_vm_init_endvm(struct kernel_args *ka);
void arch_vm_aspace_swap(vm_address_space *aspace);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_VM_H */
