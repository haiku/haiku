/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>
#include <stdio.h>


extern "C" int boot_main(struct stage2_args *args);
extern addr_t gKernelEntry;


void
platform_start_kernel(void)
{
	printf("*** jump to kernel at %p ***\n*** program exits.\n", (void *)gKernelEntry);
	exit(0);
}


int
main(int argc, char **argv)
{
	boot_main(NULL);

	return 0;
}

