/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <stdint.h>

#include <x86intrin.h>


static uint64_t cv_factor;
static uint64_t cv_factor_nsec;


static int64_t
__system_time_lfence()
{
	__builtin_ia32_lfence();
	__uint128_t time = static_cast<__uint128_t>(__rdtsc()) * cv_factor;
	return time >> 64;
}


static int64_t
__system_time_rdtscp()
{
	uint32_t aux;
	__uint128_t time = static_cast<__uint128_t>(__rdtscp(&aux)) * cv_factor;
	return time >> 64;
}


static int64_t
__system_time_nsecs_lfence()
{
	__builtin_ia32_lfence();
	__uint128_t t = static_cast<__uint128_t>(__rdtsc()) * cv_factor_nsec;
	return t >> 32;
}


static int64_t
__system_time_nsecs_rdtscp()
{
	uint32_t aux;
	__uint128_t t = static_cast<__uint128_t>(__rdtscp(&aux)) * cv_factor_nsec;
	return t >> 32;
}


static int64_t (*sSystemTime)(void) = __system_time_lfence;
static int64_t (*sSystemTimeNsecs)(void) = __system_time_nsecs_lfence;


// from kernel/arch/x86/arch_cpu.h
#define IA32_FEATURE_AMD_EXT_RDTSCP		(1 << 27) // rdtscp instruction


extern "C" void
__x86_setup_system_time(uint64_t cv, uint64_t cv_nsec)
{
	cv_factor = cv;
	cv_factor_nsec = cv_nsec;

	cpuid_info cpuInfo;
	get_cpuid(&cpuInfo, 0x80000000, 0);
	if (cpuInfo.eax_0.max_eax >= 0x80000001) {
		get_cpuid(&cpuInfo, 0x80000001, 0);
		if ((cpuInfo.regs.edx & IA32_FEATURE_AMD_EXT_RDTSCP)!= 0) {
			sSystemTime = __system_time_rdtscp;
			sSystemTimeNsecs = __system_time_nsecs_rdtscp;
		}
	}
}


extern "C" int64_t
system_time()
{
	return sSystemTime();
}



extern "C" int64_t
system_time_nsecs()
{
	return sSystemTimeNsecs();
}

