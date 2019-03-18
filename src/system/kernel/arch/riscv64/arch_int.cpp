/*
 * Copyright 2003-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <int.h>


status_t
arch_int_init(kernel_args *args)
{
	return B_OK;
}


status_t
arch_int_init_post_vm(kernel_args *args)
{
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


void
arch_int_enable_io_interrupt(int irq)
{
}


void
arch_int_disable_io_interrupt(int irq)
{
}


void
arch_int_assign_to_cpu(int32 irq, int32 cpu)
{
}
