/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_VM_STORE_DEVICE_H
#define _KERNEL_VM_STORE_DEVICE_H

#include <kernel.h>
#include <vm.h>

vm_store *vm_store_create_device(addr base_addr);

#endif

