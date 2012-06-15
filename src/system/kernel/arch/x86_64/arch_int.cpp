/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include <int.h>

#include <arch/int.h>


void
arch_int_enable_io_interrupt(int irq)
{

}


void
arch_int_disable_io_interrupt(int irq)
{

}


void
arch_int_configure_io_interrupt(int irq, uint32 config)
{

}


status_t
arch_int_init(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_int_init_post_vm(struct kernel_args *args)
{
	return B_OK;
}


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	return B_OK;
}
