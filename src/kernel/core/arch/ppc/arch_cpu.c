/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <arch/cpu.h>
#include <malloc.h>
#include <vm.h>
#include <debug.h>
#include <smp.h>
#include <Errors.h>
#include <kerrors.h>
#include <tls.h>

#include <boot/stage2.h>

#include <string.h>
#include <stdio.h>


int
arch_cpu_preboot_init(kernel_args *ka)
{
	return 0;
}


int
arch_cpu_init(kernel_args *ka)
{
	//setup_system_time(ka->arch_args.system_time_cv_factor);

	return 0;
}


int
arch_cpu_init2(kernel_args *ka)
{
	return 0;
}


void
arch_cpu_invalidate_TLB_range(addr start, addr end)
{
	int num_pages = end/PAGE_SIZE - start/PAGE_SIZE;
	while (num_pages-- >= 0) {
		//invalidate_TLB(start);
		start += PAGE_SIZE;
	}
}


void
arch_cpu_invalidate_TLB_list(addr pages[], int num_pages)
{
	int i;
	for (i = 0; i < num_pages; i++) {
		//invalidate_TLB(pages[i]);
	}
}


void
arch_cpu_global_TLB_invalidate(void)
{
}


int
arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr *faultHandler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	*faultHandler = (addr)&&error;

	while (size--)
		*tmp++ = *s++;

	*faultHandler = 0;
	return 0;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


int
arch_cpu_user_strcpy(char *to, const char *from, addr *faultHandler)
{
	*faultHandler = (addr)&&error;

	while ((*to++ = *from++) != '\0')
		;

	*faultHandler = 0;
	return 0;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


int
arch_cpu_user_strncpy(char *to, const char *from, size_t size, addr *faultHandler)
{
	*faultHandler = (addr)&&error;

	while(size-- && (*to++ = *from++) != '\0')
		;

	*faultHandler = 0;
	return 0;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


int
arch_cpu_user_strlcpy(char *to, const char *from, size_t size, addr *faultHandler)
{
	*faultHandler = (addr)&&error;

	to[--size] = '\0';
	while (size-- && (*to++ = *from++) != '\0')
		;

	*faultHandler = 0;
	return 0;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


int
arch_cpu_user_memset(void *s, char c, size_t count, addr *faultHandler)
{
	char *xs = (char *)s;

	*faultHandler = (addr)&&error;

	while (count--)
		*xs++ = c;

	*faultHandler = 0;
	return 0;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


void
arch_cpu_idle(void)
{
	switch (smp_get_num_cpus()) {
		case 0:
			panic("You need at least 1 CPU to run OpenBeOS\n");
		case 1:
			//hlt();
			break;
		default:
			break;
	}
}

