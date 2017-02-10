/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_BOOT_STAGE2_ARGS_H
#define KERNEL_BOOT_STAGE2_ARGS_H


#include <SupportDefs.h>
#include <platform_stage2_args.h>


typedef struct stage2_args {
	size_t heap_size;
	const char **arguments;
	int32 arguments_count;
	struct platform_stage2_args	platform;
} stage2_args ;

#endif	/* KERNEL_BOOT_STAGE2_ARGS_H */
