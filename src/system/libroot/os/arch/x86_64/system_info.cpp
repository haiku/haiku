/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <cpuid.h>


status_t
get_cpuid(cpuid_info* info, uint32 eax, uint32 cpuNum)
{
	__cpuid_count(eax, 0, info->regs.eax, info->regs.ebx, info->regs.ecx,
		info->regs.edx);

	return B_OK;
}

