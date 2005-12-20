/*
 * Copyright 2002-2005, The Haiku Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_PRIV_H
#define _KERNEL_VM_PRIV_H

#include <vm.h>
#include <util/khash.h>

/* should make these scale with the system */
#define DEFAULT_KERNEL_WORKING_SET  1024
#define DEFAULT_WORKING_SET         256
#define DEFAULT_MAX_WORKING_SET     65536
#define DEFAULT_MIN_WORKING_SET     64

#define WORKING_SET_INCREMENT       32
#define WORKING_SET_DECREMENT       32

#define PAGE_DAEMON_INTERVAL 500000
#define PAGE_SCAN_QUANTUM 500
#define WORKING_SET_ADJUST_INTERVAL 5000000
#define MAX_FAULTS_PER_SECOND 100
#define MIN_FAULTS_PER_SECOND 10

#define WRITE_COUNT 1024
#define READ_COUNT 1

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
status_t vm_page_fault(addr_t address, addr_t faultAddress, bool isWrite, bool isUser, addr_t *newip);
void vm_unreserve_memory(size_t bytes);
status_t vm_try_reserve_memory(size_t bytes);
status_t vm_daemon_init(void);

// used by the page daemon to walk the list of address spaces
void vm_address_space_walk_start(struct hash_iterator *i);
vm_address_space *vm_address_space_walk_next(struct hash_iterator *i);

// allocates memory from the kernel_args structure
addr_t vm_alloc_from_kernel_args(kernel_args *args, size_t size, uint32 lock);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_PRIV_H */

