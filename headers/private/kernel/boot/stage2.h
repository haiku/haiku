/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#ifndef KERNEL_BOOT_STAGE2_H
#define KERNEL_BOOT_STAGE2_H


#include <boot/kernel_args.h>


struct stage2_args;
extern struct kernel_args gKernelArgs;


#ifdef __cplusplus
extern "C" {
#endif

extern int main(struct stage2_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_BOOT_STAGE2_H */
