/* ++++++++++
	File:			perfmon_kernel.h
	Description:	kernel mode interface to performance counters and time stamp 
					registers of 586 and 686 CPUs

	DO NOT use these functions in the production code !!! 
	This interface WILL BE CHANGED in the next releases.	

	Copyright (c) 1998 by Be Incorporated.  All Rights Reserved.
+++++ */


#ifndef	_PERFMON_KERNEL_H
#define	_PERFMON_KERNEL_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __INTEL__

extern _IMPEXP_KERNEL 		uint64	read_msr(uint32 msr); 
extern _IMPEXP_KERNEL		void		write_msr(uint32 msr, uint64 val); 
extern _IMPEXP_KERNEL 		uint64	read_pmc(uint32 pmc); 
extern _IMPEXP_KERNEL 		uint64	read_tsc(void); 

#endif


#ifdef __cplusplus
}
#endif

#endif
