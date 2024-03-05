/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <kernel/debug.h>

#include <boot/stage2.h>

struct vector *vector_table;

void arch_int_enable_io_interrupt(int32 irq)
{
	return;
}

void arch_int_disable_io_interrupt(int32 irq)
{
	return;
}

int arch_int_init(kernel_args *ka)
{
	int i;

	dprintf("arch_int_init: entry\n");

	return 0;
}

int arch_int_init2(kernel_args *ka)
{
	return 0;
}

