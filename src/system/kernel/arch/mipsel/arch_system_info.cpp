/*
 * Copyright 2007-2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *		Jonas Sundström <jonas@kirilla.com>
 *
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <arch_cpu.h>
#include <arch/system_info.h>
#include <boot/kernel_args.h>


status_t
arch_get_system_info(system_info *info, size_t size)
{
#warning IMPLEMENT arch_get_system_info
	return B_ERROR;
}


status_t
arch_system_info_init(struct kernel_args *args)
{
#warning IMPLEMENT arch_system_info_init
	return B_ERROR;
}

