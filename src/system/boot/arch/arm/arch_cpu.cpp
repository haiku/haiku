/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@upgrade-android.com>
 */

#include "cpu.h"

#include <OS.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>
#include <arch_cpu.h>
#include <string.h>

#define TRACE_CPU
#ifdef TRACE_CPU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

/*! check_cpu_features
 *
 * Please note the fact that ARM7 and ARMv7 are two different things ;)
 * ARMx is a specific ARM CPU core instance, while ARMvX refers to the
 * ARM architecture specification version....
 *
 * Most of the architecture versions we're detecting here we will probably
 * never run on, just included for completeness sake... ARMv5 and up are
 * the likely ones for us to support (as they all have some kind of MMU).
 */

static status_t
check_cpu_features()
{
	uint32 result = 0;
	int arch, variant = 0, part = 0, revision = 0, implementor = 0;

	asm volatile("MRC p15, 0, %[c1out], c0, c0, 0":[c1out] "=r" (result));

	implementor = (result >> 24) & 0xff;

	if (!(result & (1 << 19))) {
		switch((result >> 12) & 0xf) {
			case 0:	/* early ARMv3 or even older */
				arch = ARCH_ARM_PRE_ARM7;
				break;

			case 7:	/* ARM7 processor */
				arch = (result & (1 << 23)) ? ARCH_ARM_v4T : ARCH_ARM_v3;
				variant = (result >> 16) & 0x7f;
				part = (result >> 4) & 0xfff;
				revision = result & 0xf;
				break;

			default:
				revision = result & 0xf;
				part = (result >> 4) & 0xfff;
				switch((result >> 16) & 0xf) {
					case 1: arch = ARCH_ARM_v4; break;
					case 2: arch = ARCH_ARM_v4T; break;
					case 3: arch = ARCH_ARM_v5; break;
					case 4: arch = ARCH_ARM_v5T; break;
					case 5: arch = ARCH_ARM_v5TE; break;
					case 6: arch = ARCH_ARM_v5TEJ; break;
					case 7: arch = ARCH_ARM_v6; break;
					case 0xf: /* XXX TODO ARMv7 */; break;
				}
				variant = (result >> 20) & 0xf;
				break;
		}
	}

	TRACE(("%s: implementor=0x%x('%c'), arch=%d, variant=0x%x, part=0x%x, revision=0x%x\n",
		__func__, implementor, implementor, arch, variant, part, revision));

	return B_OK;
}

//	#pragma mark -

extern "C" void
arch_spin(bigtime_t microseconds)
{
	panic("No timing support in bootloader yet!");
}


extern "C" status_t
boot_arch_cpu_init(void)
{
	status_t err = check_cpu_features();
	if (err != B_OK) {
		panic("Retire your old Acorn and get something modern to boot!\n");
		return err;
	}

	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on

	return B_OK;
}
