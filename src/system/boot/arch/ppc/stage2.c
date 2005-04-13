/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <boot/stage2.h>
#include <libc/stdarg.h>
#include <libc/printf.h>
#include <libc/string.h>
#include "stage2_priv.h"

void _start(int arg1, int arg2, void *openfirmware);

static kernel_args ka = {0};

void _start(int arg1, int arg2, void *openfirmware)
{
	int handle;

	memset(&ka, 0, sizeof(ka));

	of_init(openfirmware);
	s2_text_init(&ka);

	printf("Welcome to the stage2 bootloader!\n");

	printf("arg1 0x%x, arg2 0x%x, openfirmware 0x%x\n", arg1, arg2, openfirmware);

	printf("msr = 0x%x\n", getmsr());

 	s2_mmu_init(&ka);

	for(;;);
}

