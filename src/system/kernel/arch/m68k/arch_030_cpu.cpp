/* 
 * Copyright 2003-2007, Haiku Inc. All rights reserved.
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

/* from arch_030_asm.S */
extern void flush_insn_pipeline_030(void);
extern void flush_atc_all_030(void);
extern void flush_atc_addr_030(addr_t addr);

#ifdef __cplusplus
}
#endif



#define CACHELINE 16

static void 
sync_icache_030(addr_t address, size_t len)
{
	int l, off;
	char *p;
	uint32 cacr;

	off = (unsigned int)address & (CACHELINE - 1);
	len += off;
	
	l = len;
	p = (char *)address - off;
	asm volatile ("nop");
	asm volatile ("movec %%cacr,%0" : "=r"(cacr):);
	cacr |= 0x00000004; /* ClearInstructionCacheEntry */
	do {
		/* the 030 invalidates only 1 long of the cache line */
		//XXX: what about 040 and 060 ?
		asm volatile ("movec %0,%%caar\n"		\
					  "movec %1,%%cacr\n"		\
					  "addq.l #4,%0\n"			\
					  "movec %0,%%caar\n"		\
					  "movec %1,%%cacr\n"		\
					  "addq.l #4,%0\n"			\
					  "movec %0,%%caar\n"		\
					  "movec %1,%%cacr\n"		\
					  "addq.l #4,%0\n"			\
					  "movec %0,%%caar\n"		\
					  "movec %1,%%cacr\n"		\
					  :: "r"(p), "r"(cacr));
		p += CACHELINE;
	} while ((l -= CACHELINE) > 0);
	asm volatile ("nop");
}


struct m68k_cpu_ops cpu_ops_030 = {
	&flush_insn_pipeline_030,
	&flush_atc_all_030,
	&flush_atc_all_030, // no global flag, so no useronly flushing
	&flush_atc_addr_030,
	&sync_icache_030, // dcache is the same
	&sync_icache_030,
	NULL // idle
};
