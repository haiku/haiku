/* 
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <syscalls.h>


status_t
get_cpuid(cpuid_info* info, uint32 eaxRegister, uint32 cpuNum)
{
	return _kern_get_cpuid(info, eaxRegister, cpuNum);
}

