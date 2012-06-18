/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/kernel_args.h>

#include <stdio.h>


extern "C" int boot_main(struct stage2_args *args);
extern struct kernel_args gKernelArgs;


void
platform_exit(void)
{
	puts("*** exit ***\n");
	exit(-1);
}


void
platform_start_kernel(void)
{
	printf("*** jump to kernel at %p ***\n*** program exits.\n", (void *)gKernelArgs.kernel_image.elf_header.e_entry);
	exit(0);
}


int
main(int argc, char **argv)
{
	// The command arguments are evaluated in platform_devices.cpp!

	stage2_args args;
	boot_main(&args);

	return 0;
}

