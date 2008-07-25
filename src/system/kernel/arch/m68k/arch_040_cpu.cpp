/* 
 * Copyright 2003-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 */

#include <KernelExport.h>

#include <arch_platform.h>
#include <arch_thread.h>
#include <arch/cpu.h>
#include <boot/kernel_args.h>

#ifdef __cplusplus
extern "C" {
#endif

/* from arch_040_asm.S */
extern void flush_insn_pipeline_040(void);
extern void flush_atc_all_040(void);
extern void flush_atc_user_040(void);
extern void flush_atc_addr_040(addr_t addr);

#ifdef __cplusplus
}
#endif



#define CACHELINE 16

static void 
sync_icache_040(addr_t address, size_t len)
{
	int l, off;
	char *p;

	off = (unsigned int)address & (CACHELINE - 1);
	len += off;
	
	l = len;
	p = (char *)address - off;
	asm volatile ("nop");

#warning M68K: 040: use CPUSHP on pages when possible for speed.
	do {
		asm volatile (		\
					  "cpushl %%ic,(%0)\n"		\
					  :: "a"(p));
		p += CACHELINE;
	} while ((l -= CACHELINE) > 0);
	asm volatile ("nop");
}


static void 
sync_dcache_040(addr_t address, size_t len)
{
	int l, off;
	char *p;

	off = (unsigned int)address & (CACHELINE - 1);
	len += off;
	
	l = len;
	p = (char *)address - off;
	asm volatile ("nop");

#warning M68K: 040: use CPUSHP on pages when possible for speed.
	do {
		asm volatile (		\
					  "cpushl %%dc,(%0)\n"		\
					  :: "a"(p));
		p += CACHELINE;
	} while ((l -= CACHELINE) > 0);
	asm volatile ("nop");
}


struct m68k_cpu_ops cpu_ops_040 = {
	&flush_insn_pipeline_040,
	&flush_atc_all_040,
	&flush_atc_user_040,
	&flush_atc_addr_040,
	&sync_dcache_040,
	&sync_icache_040,
	NULL // idle
};
