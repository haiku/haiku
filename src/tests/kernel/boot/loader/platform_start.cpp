/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>
#include <boot/kernel_args.h>

#include <stdio.h>


extern "C" int boot_main(struct stage2_args *args);
extern struct kernel_args gKernelArgs;


void
platform_start_kernel(void)
{
	printf("*** jump to kernel at %p ***\n*** program exits.\n", (void *)gKernelArgs.kernel_image.elf_header.e_entry);
	exit(0);
}


int
main(int argc, char **argv)
{
	boot_main(NULL);

	return 0;
}

