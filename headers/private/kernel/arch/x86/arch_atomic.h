/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_ARCH_ATOMIC_H
#define _KERNEL_ARCH_x86_ARCH_ATOMIC_H


#ifdef __x86_64__
#	include <arch/x86/64/atomic.h>
#else
#	include <arch/x86/32/atomic.h>
#endif


#endif	// _KERNEL_ARCH_ATOMIC_H

