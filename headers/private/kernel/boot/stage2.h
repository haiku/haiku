/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_STAGE2_H
#define KERNEL_BOOT_STAGE2_H


#include <boot/kernel_args.h>


struct stage2_args;

extern struct kernel_args gKernelArgs;
extern addr_t gKernelEntry;

#ifdef __cplusplus
extern "C" {
#endif

extern int main(struct stage2_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_STAGE2_H */
