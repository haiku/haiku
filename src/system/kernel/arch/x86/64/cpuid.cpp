/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <arch_system_info.h>


status_t
get_current_cpuid(cpuid_info* info, uint32 eax, uint32 ecx)
{
	__asm__("cpuid"
		: "=a" (info->regs.eax), "=b" (info->regs.ebx), "=c" (info->regs.ecx),
			"=d" (info->regs.edx)
		: "a" (eax), "c" (ecx));
	return B_OK;
}


uint32
get_eflags()
{
	uint64_t flags;
	__asm__("pushf; popq %0;" : "=r" (flags));
	return flags;
}


void
set_eflags(uint32 value)
{
	uint64_t flags = value;
	__asm__("pushq %0; popf;" : : "r" (flags) : "cc");
}

