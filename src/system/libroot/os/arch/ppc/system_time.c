/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <libroot_private.h>
#include <real_time_data.h>


static vint64 *sConversionFactor;

void
__ppc_setup_system_time(vint64 *cvFactor)
{
	sConversionFactor = cvFactor;
}


// Note: We don't implement system_time() in assembly since the 64 bit
// divisions/multiplications are a bit tough to implement for a 32 bit
// PPC.
bigtime_t
system_time(void)
{
	uint64 timeBase = __ppc_get_time_base();
	// TODO: The multiplication doesn't look that nice. The value can easily
	// overflow when timeBase gets big enough. The limit for timebase is
	// about 2^(64 - 20). This might sound a lot, but the conversion factor
	// might be quite big. Assuming a worst case factor of 2^32,
	// this would leave us with only about 2^12 = 4096 seconds we can
	// represent. The actual factor for my Mac mini is about 40 * 10^6, i.e.
	// the overflow limit is ca. 100 times greater, but that isn't more than
	// five days either.
	return (timeBase * 1000000ULL) / *sConversionFactor;
}
