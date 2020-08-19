/*
 * Copyright 2012-2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 */

#include <OS.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>
#include <string.h>


#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/*! Detect RISCV core extensions.
*/
static status_t
check_cpu_features()
{
	// only seems possible from bootloader openfirmware or FDT?
	return B_OK;
}


extern "C" status_t
boot_arch_cpu_init(void)
{
	status_t err = check_cpu_features();
	if (err != B_OK) {
		panic("It's RISCY business trying to boot Haiku on the wrong CPU!\n");
		return err;
	}

	return B_OK;
}


extern "C" void
arch_ucode_load(BootVolume& volume)
{
	// NOP on riscv currently
}


extern "C" bigtime_t
system_time()
{
	#warning Implement system_time in RISCV bootloader!
	return 0;
}


extern "C" void
spin(bigtime_t microseconds)
{
	#warning Implment spin in RISCV bootloader!
	//bigtime_t time = system_time();
	//while ((system_time() - time) < microseconds)
	//	asm volatile ("nop;");
}
