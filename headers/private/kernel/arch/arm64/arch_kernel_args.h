/*
 * Copyright 2018, Jaroslaw Pelczar <jarek@jpelczar.com>
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ARCH_KERNEL_ARGS_H_
#define _KERNEL_ARCH_ARM64_ARCH_KERNEL_ARGS_H_


#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif


typedef struct {
	int			nothing_yet;
} arch_kernel_args;


#endif /* _KERNEL_ARCH_ARM64_ARCH_KERNEL_ARGS_H_ */
