/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <kernel.h>
#include <arch/cpu.h>
#include <boot/kernel_args.h>


int 
arch_cpu_preboot_init(kernel_args *ka)
{
	return 0;
}


int 
arch_cpu_init(kernel_args *ka)
{
	return 0;
}


int 
arch_cpu_init2(kernel_args *ka)
{
	return 0;
}

#define CACHELINE 32

void 
arch_cpu_sync_icache(void *address, size_t len)
{
	int l, off;
	char *p;

	off = (unsigned int)address & (CACHELINE - 1);
	len += off;

	l = len;
	p = (char *)address - off;
	do {
		asm volatile ("dcbst 0,%0" :: "r"(p));
		p += CACHELINE;
	} while((l -= CACHELINE) > 0);
	asm volatile ("sync");
	p = (char *)address - off;
	do {
		asm volatile ("icbi 0,%0" :: "r"(p));
		p += CACHELINE;
	} while((len -= CACHELINE) > 0);
	asm volatile ("sync");
	asm volatile ("isync");
}


void 
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
	asm volatile("sync");
	while(start < end) {
		asm volatile("tlbie %0" :: "r" (start));
		asm volatile("eieio");
		asm volatile("sync");
		start += PAGE_SIZE;
	}
	asm volatile("tlbsync");
	asm volatile("sync");
}


void 
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
	int i;

	asm volatile("sync");
	for(i = 0; i < num_pages; i++) {
		asm volatile("tlbie %0" :: "r" (pages[i]));
		asm volatile("eieio");
		asm volatile("sync");
	}
	asm volatile("tlbsync");
	asm volatile("sync");
}


void 
arch_cpu_global_TLB_invalidate(void)
{
	unsigned long i;

	asm volatile("sync");
	for(i=0; i< 0x40000; i += 0x1000) {
		asm volatile("tlbie %0" :: "r" (i));
		asm volatile("eieio");
		asm volatile("sync");
	}
	asm volatile("tlbsync");
	asm volatile("sync");
}


// ToDo: doesn't belong here!
#include <OS.h>

long long 
system_time(void)
{
	bigtime_t time_base = get_time_base();

	return (time_base * 1000000) / ((66*1000*1000) / 4); 
		// ToDo: remove hard coded
}


int
arch_cpu_user_memcpy(void *to, const void *from, size_t size, addr *fault_handler)
{
	char *tmp = (char *)to;
	char *s = (char *)from;

	*fault_handler = (addr)&&error;

	while (size--)
		*tmp++ = *s++;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


int
arch_cpu_user_strcpy(char *to, const char *from, addr *fault_handler)
{
	*fault_handler = (addr)&&error;

	while ((*to++ = *from++) != '\0')
		;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


int
arch_cpu_user_strncpy(char *to, const char *from, size_t size, addr *fault_handler)
{
	*fault_handler = (addr)&&error;

	while(size-- && (*to++ = *from++) != '\0')
		;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


/**	\brief Copies at most (\a size - 1) characters from the string in \a from to
 *	the string in \a to, NULL-terminating the result.
 *
 *	\param to Pointer to the destination C-string.
 *	\param from Pointer to the source C-string.
 *	\param size Size in bytes of the string buffer pointed to by \a to.
 *	
 *	\return strlen(\a from).
 */

int
arch_cpu_user_strlcpy(char *to, const char *from, size_t size, addr *faultHandler)
{
	int from_length = 0;

	*faultHandler = (addr)&&error;

	if (size > 0) {
		to[--size] = '\0';
		// copy 
		for ( ; size; size--, from_length++, to++, from++) {
			if ((*to = *from) == '\0')
				break;
		}
	}
	// count any leftover from chars
	while (*from++ != '\0')
		from_length++;

	*faultHandler = 0;
	return from_length;

error:
	*faultHandler = 0;
	return B_BAD_ADDRESS;
}


int
arch_cpu_user_memset(void *s, char c, size_t count, addr *fault_handler)
{
	char *xs = (char *)s;

	*fault_handler = (addr)&&error;

	while (count--)
		*xs++ = c;

	*fault_handler = 0;
	return 0;

error:
	*fault_handler = 0;
	return B_BAD_ADDRESS;
}


void
arch_cpu_idle(void)
{
}


