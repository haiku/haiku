/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/arch/cpu.h>
#include <kernel/debug.h>
#include <kernel/int.h>
#include <boot/stage2.h>

static vcpu_struct *vcpu;

int arch_cpu_preboot_init(kernel_args *ka)
{
	return 0;
}

int arch_cpu_init(kernel_args *ka)
{
	vcpu = ka->arch_args.vcpu;

	vcpu->kernel_asid = 0;
	vcpu->user_asid = 0;

	return 0;
}

int arch_cpu_init2(kernel_args *ka)
{
	return 0;
}

void sh4_set_kstack(addr kstack)
{
//	dprintf("sh4_set_kstack: setting kstack to 0x%x\n", kstack);
	vcpu->kstack = (unsigned int *)kstack;
}

void sh4_set_user_pgdir(addr pgdir)
{
//	dprintf("sh4_set_user_pgdir: setting pgdir to 0x%x\n", pgdir);
	if((addr)vcpu->user_pgdir != pgdir)
		arch_cpu_global_TLB_invalidate();
	vcpu->user_pgdir = (unsigned int *)pgdir;
}

void sh4_invl_page(addr va)
{
	int state;
	int i;

	va = ROUNDOWN(va, PAGE_SIZE);

	state = int_disable_interrupts();

	// wipe it out of the data tlbs
	for(i=0; i<UTLB_COUNT; i++) {
		struct utlb_addr_array *ua = (struct utlb_addr_array *)(UTLB + (i << UTLB_ADDR_SHIFT));
		if(ua->vpn == (va >> 10))
			ua->valid = 0;
	}

	// wipe it out of the instruction tlbs
	for(i=0; i<ITLB_COUNT; i++) {
		struct itlb_addr_array *ia = (struct itlb_addr_array *)(ITLB + (i << ITLB_ADDR_SHIFT));
		if(ia->vpn == (va >> 10))
			ia->valid = 0;
	}

	int_restore_interrupts(state);
}

void arch_cpu_invalidate_TLB_range(addr start, addr end)
{
	for(; start < end; start += PAGE_SIZE) {
		sh4_invl_page(start);
	}
}

void arch_cpu_invalidate_TLB_list(addr pages[], int num_pages)
{
	int i;
	for(i=0; i<num_pages; i++) {
		sh4_invl_page(pages[i]);
	}
}

void arch_cpu_global_TLB_invalidate()
{
	int state;
	int i;

	state = int_disable_interrupts();

	// wipe out the data tlbs
	for(i=0; i<UTLB_COUNT; i++) {
		struct utlb_addr_array *ua = (struct utlb_addr_array *)(UTLB + (i << UTLB_ADDR_SHIFT));
		ua->valid = 0;
	}

	// wipe out the instruction tlbs
	for(i=0; i<ITLB_COUNT; i++) {
		struct itlb_addr_array *ia = (struct itlb_addr_array *)(ITLB + (i << ITLB_ADDR_SHIFT));
		ia->valid = 0;
	}

	int_restore_interrupts(state);
}

int arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr *fault_handler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	*fault_handler = (addr)&&error;

	while(size--)
		*tmp++ = *s++;

	*fault_handler = 0;

	return 0;
error:
	*fault_handler = 0;
	return ERR_VM_BAD_USER_MEMORY;
}

int arch_cpu_user_strcpy(char *to, const char *from, addr *fault_handler)
{
	*fault_handler = (addr)&&error;

	while((*to++ = *from++) != '\0')
		;

	*fault_handler = 0;

	return 0;
error:
	*fault_handler = 0;
	return ERR_VM_BAD_USER_MEMORY;
}

int arch_cpu_user_strncpy(char *to, const char *from, size_t size, addr *fault_handler)
{
	*fault_handler = (addr)&&error;

	while(size-- && (*to++ = *from++) != '\0')
		;

	*fault_handler = 0;

	return 0;
error:
	*fault_handler = 0;
	return ERR_VM_BAD_USER_MEMORY;
}

int arch_cpu_user_memset(void *s, char c, size_t count, addr *fault_handler)
{
	char *xs = (char *) s;

	*fault_handler = (addr)&&error;

	while (count--)
		*xs++ = c;

	*fault_handler = 0;

	return 0;
error:
	*fault_handler = 0;
	return ERR_VM_BAD_USER_MEMORY;
}

