/*
 * Copyright 2009-2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_PI_STAGE2_H
#define KERNEL_BOOT_PLATFORM_PI_STAGE2_H

#ifndef KERNEL_BOOT_STAGE2_ARGS_H
#	error This file is included from <boot/stage2_args.h> only
#endif

struct platform_stage2_args {
	void *boot_tgz_data;
	uint32 boot_tgz_size;
};

#endif	/* KERNEL_BOOT_PLATFORM_PI_STAGE2_H */
