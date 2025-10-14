/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_BOOT_STAGE2_H
#define KERNEL_BOOT_STAGE2_H


#include <boot/kernel_args.h>

#include <util/KMessage.h>

struct stage2_args;

extern struct kernel_args gKernelArgs;
extern KMessage gBootParams;

#ifdef __cplusplus
extern "C" {
#endif

extern int main(struct stage2_args *args);

#ifdef __cplusplus
}
#endif

extern void *kernel_args_malloc(size_t size, uint8 alignment = 1);
extern char *kernel_args_strdup(const char *string);
extern void kernel_args_free(void *address);

#endif	/* KERNEL_BOOT_STAGE2_H */
