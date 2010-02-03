/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_PRIV_H
#define _KERNEL_VM_VM_PRIV_H


#include <vm/vm_types.h>


// reserved area definitions
#define RESERVED_AREA_ID -1
#define RESERVED_AVOID_BASE 0x01

// page attributes (in addition to B_READ_AREA etc.)
#define PAGE_MODIFIED 0x1000
#define PAGE_ACCESSED 0x2000
#define PAGE_PRESENT  0x4000


#ifdef __cplusplus
extern "C" {
#endif

// Should only be used by vm internals
status_t vm_page_fault(addr_t address, addr_t faultAddress, bool isWrite,
	bool isUser, addr_t *newip);
void vm_unreserve_memory(size_t bytes);
status_t vm_try_reserve_memory(size_t bytes, int priority, bigtime_t timeout);
status_t vm_daemon_init(void);

const char *page_state_to_string(int state);
	// for debugging purposes only

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_VM_PRIV_H */
