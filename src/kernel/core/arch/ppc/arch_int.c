/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <boot/stage2.h>

#include <kernel/int.h>

bool arch_int_is_interrupts_enabled(void)
{
	return true;
}

void arch_int_enable_io_interrupt(int irq)
{
	return;
}

void arch_int_disable_io_interrupt(int irq)
{
	return;
}

int arch_int_init(kernel_args *ka)
{
	return 0;
}

int arch_int_init2(kernel_args *ka)
{
	return 0;
}

