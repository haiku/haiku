/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/platform.h>


extern "C" int boot_main(struct stage2_args *args);


int
main(int argc, char **argv)
{
	boot_main(NULL);

	return 0;
}

