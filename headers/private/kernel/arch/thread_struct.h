/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_THREAD_STRUCT_H
#define _NEWOS_KERNEL_ARCH_THREAD_STRUCT_H

#ifdef ARCH_x86
#include <arch/x86/thread_struct.h>
#endif
#ifdef ARCH_sh4
#include <arch/sh4/thread_struct.h>
#endif
#ifdef ARCH_alpha
#include <arch/alpha/thread_struct.h>
#endif
#ifdef ARCH_sparc64
#include <arch/sparc64/thread_struct.h>
#endif
#ifdef ARCH_mips
#include <arch/mips/thread_struct.h>
#endif
#ifdef ARCH_ppc
#include <arch/ppc/thread_struct.h>
#endif
#ifdef ARCH_m68k
#include <arch/m68k/thread_struct.h>
#endif

#endif

