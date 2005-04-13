/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <kernel/debug.h>

#include <kernel/arch/pmap.h>

#include <nulibc/string.h>

#define CHATTY_PMAP 0

int arch_pmap_init(kernel_args *ka)
{
	dprintf("arch_pmap_init: entry\n");

	return 0;
}

int arch_pmap_init2(kernel_args *ka)
{
	return 0;
}

int pmap_map_page(addr paddr, addr vaddr, int lock)
{
#if CHATTY_PMAP
	dprintf("pmap_map_page: entry paddr 0x%x vaddr 0x%x lock 0x%x\n", paddr, vaddr, lock);
#endif

	arch_pmap_invl_page(vaddr);
	
	return 0;
}

int pmap_unmap_page(addr vaddr)
{
	panic("pmap_unmap_page unimplemented!\n");
	return 0;
}

void arch_pmap_invl_page(addr vaddr)
{
#if CHATTY_PMAP
	dprintf("arch_pmap_invl_page: vaddr 0x%x\n", vaddr);
#endif
	return;
}

int pmap_get_page_mapping(addr vaddr, addr *paddr)
{

	return 0;
}
