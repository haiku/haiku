/*
 * Copyright 2023, Jérôme Duval, jerome.duval@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sched.h>

#include <syscalls.h>

#ifdef __x86_64__

#include <pthread.h>

#include <x86intrin.h>

#define IA32_FEATURE_RDPID			(1 << 22) // RDPID Instruction
#define IA32_FEATURE_AMD_EXT_RDTSCP		(1 << 27) // rdtscp instruction


typedef int (*sched_cpu_func)();
static pthread_once_t sSchedCpuInitOnce = PTHREAD_ONCE_INIT;
static sched_cpu_func sSchedCpuFunc = NULL;


static int
__sched_cpu_syscall()
{
	return _kern_get_cpu();
}


static int
__sched_cpu_rdtscp()
{
	uint32_t aux;
	__rdtscp(&aux);
	return aux;
}


static int
__sched_cpu_rdpid()
{
	return _rdpid_u32();
}


static void
initSchedCpuFunc()
{
	cpuid_info cpuInfo;
	get_cpuid(&cpuInfo, 0, 0);
	if (cpuInfo.eax_0.max_eax >= 0x7) {
		get_cpuid(&cpuInfo, 0x7, 0);
		if ((cpuInfo.regs.ecx & IA32_FEATURE_RDPID) != 0) {
			sSchedCpuFunc = __sched_cpu_rdpid;
			return;
		}
	}
	get_cpuid(&cpuInfo, 0x80000000, 0);
	if (cpuInfo.eax_0.max_eax >= 0x80000001) {
		get_cpuid(&cpuInfo, 0x80000001, 0);
		if ((cpuInfo.regs.edx & IA32_FEATURE_AMD_EXT_RDTSCP)!= 0) {
			sSchedCpuFunc = __sched_cpu_rdtscp;
			return;
		}
	}
	sSchedCpuFunc = __sched_cpu_syscall;
}


int
sched_getcpu()
{
	if (sSchedCpuFunc == NULL) {
		pthread_once(&sSchedCpuInitOnce, &initSchedCpuFunc);
	}
	return sSchedCpuFunc();
}


#else
int
sched_getcpu()
{
	return _kern_get_cpu();
}
#endif

