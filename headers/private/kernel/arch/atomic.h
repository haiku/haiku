/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ATOMIC_H
#define _KERNEL_ARCH_ATOMIC_H


#include <SupportDefs.h>

#include <KernelExport.h>


#ifdef __x86_64__
#	include <arch/x86/64/atomic.h>
#elif __INTEL__
#	include <arch/x86/32/atomic.h>
#elif __POWERPC__
#	include <arch/ppc/arch_atomic.h>
#endif


#endif	// _KERNEL_ARCH_ATOMIC_H

