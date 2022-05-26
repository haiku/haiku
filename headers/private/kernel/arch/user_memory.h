/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_USER_MEMORY_H
#define _KERNEL_ARCH_USER_MEMORY_H


#include <OS.h>

#include <thread.h>


#ifdef __x86_64__
#	include <arch/generic/user_memory.h>
#elif defined(__M68K__)
#	include <arch/generic/user_memory.h>
#elif defined(__riscv)
#	include <arch/generic/user_memory.h>
#else

extern "C" {

status_t _arch_cpu_user_memcpy(void* to, const void* from, size_t size,
	void (**faultHandler)(void));
ssize_t _arch_cpu_user_strlcpy(char* to, const char* from, size_t size,
	void (**faultHandler)(void));
status_t _arch_cpu_user_memset(void* s, char c, size_t count,
	void (**faultHandler)(void));

}


static inline status_t
arch_cpu_user_memcpy(void* to, const void* from, size_t size)
{
	return _arch_cpu_user_memcpy(to, from, size,
		&thread_get_current_thread()->fault_handler);
}


static inline ssize_t
arch_cpu_user_strlcpy(char* to, const char* from, size_t size)
{
	return _arch_cpu_user_strlcpy(to, from, size,
		&thread_get_current_thread()->fault_handler);
}


static inline status_t
arch_cpu_user_memset(void* s, char c, size_t count)
{
	return _arch_cpu_user_memset(s, c, count,
		&thread_get_current_thread()->fault_handler);
}


#endif

#endif	// _KERNEL_ARCH_USER_MEMORY_H

