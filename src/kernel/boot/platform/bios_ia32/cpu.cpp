/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** calculate_cpu_conversion_factor() was written by Travis Geiselbrecht and
** licensed under the NewOS license.
*/


#include "cpu.h"

#include <OS.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>

#include <string.h>


#define TRACE_CPU
#ifdef TRACE_SMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


extern "C" void smp_boot(void);
extern "C" uint64 rdtsc();

uint32 gTimeConversionFactor;

#define TIMER_CLKNUM_HZ (14318180/12)


static uint32
calculate_cpu_conversion_factor()
{
	uint32 s_low, s_high;
	uint32 low, high;
	uint32 expired;
	uint64 t1, t2;
	uint64 p1, p2, p3;
	double r1, r2, r3;

	out8(0x34, 0x43);	/* program the timer to count down mode */
	out8(0xff, 0x40);	/* low and then high */
	out8(0xff, 0x40);

	/* quick sample */
quick_sample:
	do {
		out8(0x00, 0x43); /* latch counter value */
		s_low = in8(0x40);
		s_high = in8(0x40);
	} while(s_high != 255);
	t1 = rdtsc();
	do {
		out8(0x00, 0x43); /* latch counter value */
		low = in8(0x40);
		high = in8(0x40);
	} while (high > 224);
	t2 = rdtsc();

	p1 = t2-t1;
	r1 = (double)(p1) / (double)(((s_high << 8) | s_low) - ((high << 8) | low));

	/* not so quick sample */
not_so_quick_sample:
	do {
		out8(0x00, 0x43); /* latch counter value */
		s_low = in8(0x40);
		s_high = in8(0x40);
	} while (s_high!= 255);
	t1 = rdtsc();
	do {
		out8(0x00, 0x43); /* latch counter value */
		low = in8(0x40);
		high = in8(0x40);
	} while (high> 192);
	t2 = rdtsc();
	p2 = t2-t1;
	r2 = (double)(p2) / (double)(((s_high << 8) | s_low) - ((high << 8) | low));
	if ((r1/r2) > 1.01) {
		//dprintf("Tuning loop(1)\n");
		goto quick_sample;
	}
	if ((r1/r2) < 0.99) {
		//dprintf("Tuning loop(1)\n");
		goto quick_sample;
	}

	/* slow sample */
	do {
		out8(0x00, 0x43); /* latch counter value */
		s_low = in8(0x40);
		s_high = in8(0x40);
	} while (s_high!= 255);
	t1 = rdtsc();
	do {
		out8(0x00, 0x43); /* latch counter value */
		low = in8(0x40);
		high = in8(0x40);
	} while (high > 128);
	t2 = rdtsc();

	p3 = t2-t1;
	r3 = (double)(p3) / (double)(((s_high << 8) | s_low) - ((high << 8) | low));
	if ((r2/r3) > 1.01) {
		TRACE(("Tuning loop(2)\n"));
		goto not_so_quick_sample;
	}
	if ((r2/r3) < 0.99) {
		TRACE(("Tuning loop(2)\n"));
		goto not_so_quick_sample;
	}

	expired = ((s_high << 8) | s_low) - ((high << 8) | low);
	p3 *= TIMER_CLKNUM_HZ;

	/*
	 * cv_factor contains time in usecs per CPU cycle * 2^32
	 *
	 * The code below is a bit fancy. Originally Michael Noistering
	 * had it like:
	 *
	 *     cv_factor = ((uint64)1000000<<32) * expired / p3;
	 *
	 * whic is perfect, but unfortunately 1000000ULL<<32*expired
	 * may overflow in fast cpus with the long sampling period
	 * i put there for being as accurate as possible under
	 * vmware.
	 *
	 * The below calculation is based in that we are trying
	 * to calculate:
	 *
	 *     (C*expired)/p3 -> (C*(x0<<k + x1))/p3 ->
	 *     (C*(x0<<k))/p3 + (C*x1)/p3
	 *
	 * Now the term (C*(x0<<k))/p3 is rewritten as:
	 *
	 *     (C*(x0<<k))/p3 -> ((C*x0)/p3)<<k + reminder
	 *
	 * where reminder is:
	 *
	 *     floor((1<<k)*decimalPart((C*x0)/p3))
	 *
	 * which is approximated as:
	 *
	 *     floor((1<<k)*decimalPart(((C*x0)%p3)/p3)) ->
	 *     (((C*x0)%p3)<<k)/p3
	 *
	 * So the final expression is:
	 *
	 *     ((C*x0)/p3)<<k + (((C*x0)%p3)<<k)/p3 + (C*x1)/p3
	 */
	 /*
	 * To get the highest accuracy with this method
	 * x0 should have the 12 most significant bits of expired
	 * to minimize the error upon <<k.
	 */
	 /*
	 * Of course, you are not expected to understand any of this.
	 */
	{
		unsigned i;
		unsigned k;
		uint64 C;
		uint64 x0;
		uint64 x1;
		uint64 a, b, c;

		/* first calculate k*/
		k = 0;
		for (i = 12; i < 16; i++) {
			if (expired & (1<<i))
				k = i - 11;
		}

		C = 1000000ULL << 32;
		x0 = expired >> k;
		x1 = expired & ((1 << k) - 1);

		a = ((C * x0) / p3) << k;
		b = (((C * x0) % p3) << k) / p3;
		c = (C * x1) / p3;
#if 0
		dprintf("a=%Ld\n", a);
		dprintf("b=%Ld\n", b);
		dprintf("c=%Ld\n", c);
		dprintf("%d %Ld\n", expired, p3);
#endif
		gTimeConversionFactor = a + b + c;
#if 0
		dprintf("cvf=%Ld\n", cv_factor);
#endif
	}

#ifdef TRACE_CPU
	if (p3 / expired / 1000000000LL)
		dprintf("CPU at %Ld.%03Ld GHz\n", p3/expired/1000000000LL, ((p3/expired)%1000000000LL)/1000000LL);
	else
		dprintf("CPU at %Ld.%03Ld MHz\n", p3/expired/1000000LL, ((p3/expired)%1000000LL)/1000LL);
#endif

	return gTimeConversionFactor;
}


static status_t
check_cpu_features()
{
	// ToDo: for now
	return B_OK;
}


//	#pragma mark -


extern "C" void
spin(bigtime_t microseconds)
{
	bigtime_t time = system_time();

	while((system_time() - time) < microseconds)
		;
}


extern "C" void
cpu_boot_other_cpus()
{
	smp_boot();
}


extern "C" void
cpu_init()
{
	if (check_cpu_features() != B_OK)
		panic("You need a Pentium or higher in order to boot!\n");

	gKernelArgs.arch_args.system_time_cv_factor = calculate_cpu_conversion_factor();
	gKernelArgs.num_cpus = 1;
		// this will eventually be corrected later on
}

