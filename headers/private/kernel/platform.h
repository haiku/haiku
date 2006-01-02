/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_PLATFORM_H
#define _KERNEL_PLATFORM_H

#include <SupportDefs.h>

struct kernel_args;

#ifdef __cplusplus
extern "C" {
#endif

status_t platform_init(struct kernel_args *kernelArgs);
status_t platform_init_post_vm(struct kernel_args *kernelArgs);

#ifdef __cplusplus
}	// extern "C"
#endif


#endif	// _KERNEL_PLATFORM_H
