/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_VM_H
#define _NEWOS_KERNEL_ARCH_VM_H

#include <stage2.h>
#include <vm.h>

int arch_vm_init(kernel_args *ka);
int arch_vm_init2(kernel_args *ka);
int arch_vm_init_endvm(kernel_args *ka);
void arch_vm_aspace_swap(vm_address_space *aspace);

#endif

