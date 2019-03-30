/*
 * Copyright (C) 2009 David McPaul
 *
 * includes code from sysinfo.c which is
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright (c) 2002, Carlos Hasan, for Haiku.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
 
#include <string.h>
#include <cpu_type.h>

#include "CpuCapabilities.h"

CPUCapabilities::~CPUCapabilities()
{
}

CPUCapabilities::CPUCapabilities()
{
	#ifdef __i386__
		setIntelCapabilities();
	#endif
	
	PrintCapabilities();
}

void
CPUCapabilities::setIntelCapabilities()
{
	cpuid_info baseInfo;
	cpuid_info cpuInfo;
	int32 maxStandardFunction, maxExtendedFunction = 0;

	if (get_cpuid(&baseInfo, 0L, 0L) != B_OK) {
		// this CPU doesn't support cpuid
		return;
	}

	maxStandardFunction = baseInfo.eax_0.max_eax;
	if (maxStandardFunction >= 500) {
		maxStandardFunction = 0; /* old Pentium sample chips has cpu signature here */
	}
	
	/* Extended cpuid */

	get_cpuid(&cpuInfo, 0x80000000, 0L);

	// extended cpuid is only supported if max_eax is greater than the service id
	if (cpuInfo.eax_0.max_eax > 0x80000000) {
		maxExtendedFunction = cpuInfo.eax_0.max_eax & 0xff;
	}
	
	if (maxStandardFunction > 0) {

		get_cpuid(&cpuInfo, 1L, 0L);
		if (cpuInfo.eax_1.features & (1UL << 23)) {
			capabilities = CAPABILITY_MMX;
		}
	
		if (cpuInfo.eax_1.features & (1UL << 25)) {
			capabilities = CAPABILITY_SSE1;
		}

		if (cpuInfo.eax_1.features & (1UL << 26)) {
			capabilities = CAPABILITY_SSE2;
		}

		if (maxStandardFunction >= 1) {
			/* Extended features */
			if (cpuInfo.eax_1.extended_features & (1UL << 0)) {
				capabilities = CAPABILITY_SSE3;
			}
			if (cpuInfo.eax_1.extended_features & (1UL << 9)) {
				capabilities = CAPABILITY_SSSE3;
			}
			if (cpuInfo.eax_1.extended_features & (1UL << 19)) {
				capabilities = CAPABILITY_SSE41;
			}
			if (cpuInfo.eax_1.extended_features & (1UL << 20)) {
				capabilities = CAPABILITY_SSE42;
			}
		}
	}
}

bool
CPUCapabilities::HasMMX()
{
	return capabilities >= CAPABILITY_MMX;
}

bool
CPUCapabilities::HasSSE1()
{
	return capabilities >= CAPABILITY_SSE1;
}

bool
CPUCapabilities::HasSSE2()
{
	return capabilities >= CAPABILITY_SSE2;
}

bool
CPUCapabilities::HasSSE3()
{
	return capabilities >= CAPABILITY_SSE3;
}

bool
CPUCapabilities::HasSSSE3()
{
	return capabilities >= CAPABILITY_SSSE3;
}

bool
CPUCapabilities::HasSSE41()
{
	return capabilities >= CAPABILITY_SSE41;
}

bool
CPUCapabilities::HasSSE42()
{
	return capabilities >= CAPABILITY_SSE42;
}

void
CPUCapabilities::PrintCapabilities()
{
	static const char *CapArray[8] = {
		"", "MMX", "SSE1", "SSE2", "SSE3", "SSSE3", "SSE4.1", "SSE4.2"
	};

	printf("CPU is capable of running ");
	if (capabilities) {
		for (uint32 i=1;i<=capabilities;i++) {
			printf("%s ",CapArray[i]);
		}
	} else {
		printf("no extensions");
	}
	printf("\n");
}
