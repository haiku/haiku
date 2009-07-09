/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYSTEM_ARCH_X86_APM_DEFS_H
#define SYSTEM_ARCH_X86_APM_DEFS_H


#include <SupportDefs.h>


// temporary generic syscall interface
#define APM_SYSCALLS "apm"
#define APM_GET_BATTERY_INFO	1

struct apm_battery_info {
	bool	online;
	int32	percent;
	time_t	time_left;
};


#endif	/* SYSTEM_ARCH_X86_APM_DEFS_H */
