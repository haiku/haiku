/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <kernel/arch/cpu.h>
#include <boot/stage2.h>

int arch_cpu_init(kernel_args *ka)
{
	return 0;
}

int arch_cpu_init2(kernel_args *ka)
{
	return 0;
}

void arch_cpu_invalidate_TLB_range(addr start, addr end)
{
}

void arch_cpu_invalidate_TLB_list(addr pages[], int num_pages)
{
}

void arch_cpu_global_TLB_invalidate(void)
{
}

long long system_time(void)
{
	return 0;
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


