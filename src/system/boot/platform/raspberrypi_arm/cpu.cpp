/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "arch_cpu.h"

extern "C" void
cpu_init()
{
	boot_arch_cpu_init();
}
