/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_VM_PRIV_H
#define _KERNEL_VM_PRIV_H

#include <vm.h>
#include <khash.h>

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

#define RESERVED_REGION_ID -1

// page attributes (in addition to B_READ_AREA etc.)
#define PAGE_MODIFIED 64
#define PAGE_ACCESSED 128
#define PAGE_PRESENT  256

// Should only be used by vm internals
int vm_page_fault(addr_t address, addr_t fault_address, bool is_write, bool is_user, addr_t *newip);
void vm_increase_max_commit(addr_t delta);
int vm_daemon_init(void);

// used by the page daemon to walk the list of address spaces
int vm_aspace_walk_start(struct hash_iterator *i);
vm_address_space *vm_aspace_walk_next(struct hash_iterator *i);

// get some data about the number of pages in the system
addr_t vm_page_num_pages(void);
addr_t vm_page_num_free_pages(void);

// allocates memory from the ka structure
addr_t vm_alloc_from_ka_struct(kernel_args *ka, unsigned int size, int lock);

#endif	/* _KERNEL_VM_PRIV_H */

