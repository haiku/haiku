/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 *
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "arch_timer.h"

#include <boot/arch/x86/arch_cpu.h>
#include <boot/arch/x86/arch_hpet.h>


void
arch_timer_init(void)
{
	// use PIT channel 2 for the calibration loop
	// as channel 0 is reserved for the UEFI firmware
	determine_cpu_conversion_factor(2);

	hpet_init();
}
