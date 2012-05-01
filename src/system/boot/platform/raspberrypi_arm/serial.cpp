/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "serial.h"

#include <boot/platform.h>
#include <arch/cpu.h>
#include <boot/stage2.h>

#include <string.h>


static void
serial_putc(char c)
{
#warning IMPLEMENT serial_putc
}


extern "C" void
serial_puts(const char* string, size_t size)
{
#warning IMPLEMENT serial_puts
}


extern "C" void 
serial_disable(void)
{
#warning IMPLEMENT serial_disable
}


extern "C" void 
serial_enable(void)
{
#warning IMPLEMENT serial_enable
}


extern "C" void
serial_cleanup(void)
{
#warning IMPLEMENT serial_cleanup
}


extern "C" void
serial_init(void)
{
#warning IMPLEMENT serial_init
}

