/*
 * Copyright 2012-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "cpu.h"
#include "efi_platform.h"

#include <OS.h>
#include <boot/platform.h>
#include <boot/arch/arm/arch_cpu.h>


bigtime_t
system_time()
{
	#warning Implement system_time in ARM bootloader!
	static bigtime_t sSystemTimeCounter = 0;
	return sSystemTimeCounter++;
}


void
spin(bigtime_t microseconds)
{
	kBootServices->Stall(microseconds);
}
