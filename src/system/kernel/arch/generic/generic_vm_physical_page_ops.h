/*
 * Copyright 2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_GENERIC_VM_PHYSICAL_PAGE_OPS_H
#define _KERNEL_GENERIC_VM_PHYSICAL_PAGE_OPS_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

status_t generic_vm_memset_physical(addr_t address, int value, size_t length);
status_t generic_vm_memcpy_from_physical(void* to, addr_t from, size_t length,
			bool user);
status_t generic_vm_memcpy_to_physical(addr_t to, const void* from,
			size_t length, bool user);
void generic_vm_memcpy_physical_page(addr_t to, addr_t from);

#ifdef __cplusplus
}
#endif

#endif	// _KERNEL_GENERIC_VM_PHYSICAL_PAGE_OPS_H
