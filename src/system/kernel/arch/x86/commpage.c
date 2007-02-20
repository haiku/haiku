/*
 * Copyright 2007, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <vm.h>
#include <arch/x86/commpage.h>

#include <KernelExport.h>
#include <string.h>

static area_id comm_area;
static area_id user_comm_area;
static unsigned long *comm_ptr;
static unsigned long *user_comm_ptr;
static void *next_comm_addr;

// user syscall assembly stub
extern void _user_syscall_int(void);
extern unsigned int _user_syscall_int_end;

static inline
addr_t commpage_ptr_to_user_ptr(const void *ptr)
{
	return ((addr_t)ptr) + ((addr_t)user_comm_ptr - (addr_t)comm_ptr);
}

static
status_t initialize_commpage_syscall(void)
{
	size_t len;
	
	// for now, we're hard coded to use the legacy method (int 99)
	len = (size_t)((void *)&_user_syscall_int_end - (void *)&_user_syscall_int);
	memcpy(next_comm_addr, &_user_syscall_int, len);

	// fill in the table entry
	comm_ptr[COMMPAGE_ENTRY_SYSCALL] = commpage_ptr_to_user_ptr(next_comm_addr);
	next_comm_addr = (void *)((addr_t)next_comm_addr + ROUNDUP(len, 4));

	return B_OK;
}

status_t
commpage_init(void)
{
	int i;

	// create a read/write kernel area
	comm_area = create_area("commpage", (void **)&comm_ptr, B_ANY_ADDRESS, COMMPAGE_SIZE, B_FULL_LOCK, 
		B_KERNEL_WRITE_AREA | B_KERNEL_READ_AREA);

	// clone it at a fixed address with user read/only permissions
	user_comm_ptr = (void *)USER_COMMPAGE_ADDR;
	user_comm_area = clone_area("user_commpage", (void **)&user_comm_ptr, B_EXACT_ADDRESS, 
		B_READ_AREA | B_EXECUTE_AREA, comm_area);

	// zero it out
	memset(comm_ptr, 0, COMMPAGE_SIZE);

	// fill in some of the table
	comm_ptr[0] = COMMPAGE_SIGNATURE;
	comm_ptr[1] = COMMPAGE_VERSION;

	// the next slot to allocate space is after the table
	next_comm_addr = (void *)&comm_ptr[TABLE_ENTRIES];

	// select the optimum syscall mechanism and patch the commpage
	initialize_commpage_syscall();

	return B_OK;
}

