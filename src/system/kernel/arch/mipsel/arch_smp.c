/* 
 * Copyright 2007-2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *		Jonas Sundström <jonas@kirilla.com>
 *
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de
 * Distributed under the terms of the OpenBeOS License.
 */


#include <KernelExport.h>

#include <boot/stage2.h>
#include <arch/smp.h>
#include <debug.h>


status_t
arch_smp_init(kernel_args* args)
{
#warning IMPLEMENT arch_smp_init
	return B_ERROR;
}


status_t
arch_smp_per_cpu_init(kernel_args* args, int32 cpu)
{
#warning IMPLEMENT arch_smp_per_cpu_init
	return B_ERROR;
}


void
arch_smp_send_ici(int32 target_cpu)
{
#warning IMPLEMENT arch_smp_send_ici
	panic("called arch_smp_send_ici!\n");
}


void
arch_smp_send_broadcast_ici()
{
#warning IMPLEMENT arch_smp_send_broadcast_ici
	panic("called arch_smp_send_broadcast_ici\n");
}

