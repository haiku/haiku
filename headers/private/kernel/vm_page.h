/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_VM_PAGE_H
#define _KERNEL_VM_PAGE_H

#include <kernel.h>
#include <vm.h>

struct kernel_args;


int vm_page_init(struct kernel_args *ka);
int vm_page_init2(struct kernel_args *ka);
int vm_page_init_postthread(struct kernel_args *ka);

int vm_mark_page_inuse(addr page);
int vm_mark_page_range_inuse(addr start_page, addr len);
int vm_page_set_state(vm_page *page, int state);

vm_page *vm_page_allocate_page(int state);
vm_page *vm_page_allocate_page_run(int state, addr len);
vm_page *vm_page_allocate_specific_page(addr page_num, int state);
vm_page *vm_lookup_page(addr page_num);

#endif	/* _KERNEL_VM_PAGE_H */
