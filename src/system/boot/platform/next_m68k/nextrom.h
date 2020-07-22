
/* parts from: */

/*	$NetBSD: nextrom.h,v 1.11 2011/12/18 04:29:32 tsutsui Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* parts from NeXT headers */

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	N_SIMM		4		/* number of SIMMs in machine */

/* SIMM types */
#define SIMM_SIZE       0x03
#define	SIMM_SIZE_EMPTY	0x00
#define	SIMM_SIZE_16MB	0x01
#define	SIMM_SIZE_4MB	0x02
#define	SIMM_SIZE_1MB	0x03
#define	SIMM_PAGE_MODE	0x04
#define	SIMM_PARITY	0x08 /* ?? */

/* Space for onboard RAM
 */
#define	MAX_PHYS_SEGS	(N_SIMM + 1)

/* Machine types, used in both assembler and C sources. */
#define	NeXT_CUBE	0
#define	NeXT_WARP9	1
#define	NeXT_X15	2
#define	NeXT_WARP9C	3
#define NeXT_TURBO_MONO	4
#define NeXT_TURBO_COLOR 5			/* probed witnessed */

#define	ROM_STACK_SIZE	(8192 - 2048)

#if 0

/*
 *  The ROM monitor uses the old structure alignment for backward
 *  compatibility with previous ROMs.  The old alignment is enabled
 *  with the following pragma.  The kernel uses the "MG" macro to
 *  construct an old alignment offset into the mon_global structure.
 *  The kernel file <mon/assym.h> should be copied from the "assym.h"
 *  found in the build directory of the current ROM release.
 *  It will contain the proper old alignment offset constants.
 */

#if	MONITOR
#pragma	CC_OLD_STORAGE_LAYOUT_ON
#else	MONITOR
#import <mon/assym.h>
#define	MG(type, off) \
	((type) ((u_int) (mg) + off))
#endif	/* MONITOR */

#endif

#define	LMAX		128
#define	NBOOTFILE	64
#define	NADDR		8

/* NetBSD defines macros and other stuff to deal with old alignment
   of this structure, we rather declare it as packed aligned at 2.
   (the __attribute__ directive does not support an argument so we use pragma)
   (cf. src/sys/arch/next68k/next68k/nextrom.h in NetBSD)
   */

/* fake structs to get correct size, calculated from offset deltas */

struct nvram_info {
	char data[54-22];
};

struct mon_region {
	long	first_phys_addr;
	long	last_phys_addr;
};

enum SIO_ARGS {
	SIO_ARGS_max = 1
};

struct sio {
	enum SIO_ARGS si_args;
	unsigned int si_ctrl, si_unit, si_part;
	struct device *si_dev;
	unsigned int si_blklen;
	unsigned int si_lastlba;
	/*caddr_t*/void *si_sadmem;
	/*caddr_t*/void *si_protomem;
	/*caddr_t*/void *si_devmem;
};

struct km_mon {
	char data[370-324];
};

struct km_console_info
{
	int	pixels_per_word;	/* Pixels per 32 bit word: 16, 4, 2, or 1 */
	int	bytes_per_scanline;
	int	dspy_w;			/* Visible display width in pixels */
	int	dspy_max_w;		/* Display width in pixels */
	int	dspy_h;			/* Visible display height in pixels */
#define KM_CON_ON_NEXTBUS	1	/* flag_bits: Console is NextBus device */
	int	flag_bits;		/* Vendor and NeXT flags */
	int	color[4];		/* Bit pattern for white thru black */
#define KM_HIGH_SLOT	6		/* highest possible console slot. */
	char	slot_num;		/* Slot of console device */
	char	fb_num;			/* Logical frame buffer in slot for console */
	char	byte_lane_id;		/* A value of 1, 4, or 8 */
	int	start_access_pfunc;	/* P-code run before each FB access */
	int	end_access_pfunc;	/* P-code run after each FB access */
	struct	{		/* Frame buffer related addresses to be mapped */
			int	phys_addr;
			int	virt_addr;
			int	size;
#define KM_CON_MAP_ENTRIES	6
#define KM_CON_PCODE		0
#define KM_CON_FRAMEBUFFER	1
#define KM_CON_BACKINGSTORE	2
		} map_addr[KM_CON_MAP_ENTRIES];
	int	access_stack;
};
/*
struct km_console_info {
	char data[(936-4)-(788+16)];
};*/

#pragma pack(push,2)

struct mon_global {
	char mg_simm[N_SIMM];	/* MUST BE FIRST (accesed early by locore) */
	char mg_flags;		/* MUST BE SECOND */
#define	MGF_LOGINWINDOW		0x80
#define	MGF_UART_SETUP		0x40
#define	MGF_UART_STOP		0x20
#define	MGF_UART_TYPE_AHEAD	0x10
#define	MGF_ANIM_RUN		0x08
#define	MGF_SCSI_INTR		0x04
#define	MGF_KM_EVENT		0x02
#define	MGF_KM_TYPE_AHEAD	0x01
	u_int mg_sid, mg_pagesize, mg_mon_stack, mg_vbr;
	struct nvram_info mg_nvram;
	char mg_inetntoa[18];
	char mg_inputline[LMAX];
	struct mon_region mg_region[N_SIMM];
	void *mg_alloc_base, *mg_alloc_brk;
	char *mg_boot_dev, *mg_boot_arg, *mg_boot_info, *mg_boot_file;
	char mg_bootfile[NBOOTFILE];
	enum SIO_ARGS mg_boot_how;
	struct km_mon km;
	int mon_init;
	struct sio *mg_si;
	int mg_time;
	char *mg_sddp;
	char *mg_dgp;
	char *mg_s5cp;
	char *mg_odc, *mg_odd;
	char mg_radix;
	int mg_dmachip;
	int mg_diskchip;
	volatile int *mg_intrstat;
	volatile int *mg_intrmask;
	void (*mg_nofault)();
	char fmt;
	int addr[NADDR], na;
	int	mx, my;			/* mouse location */
	u_int	cursor_save[2][32];
	int (*mg_getc)(), (*mg_try_getc)(), (*mg_putc)(int);
	int (*mg_alert)(), (*mg_alert_confirm)();
	void *(*mg_alloc)();
	int (*mg_boot_slider)();
	volatile u_char *eventc_latch;
	volatile u_int event_high;
	struct animation *mg_animate;
	int mg_anim_time;
	void (*mg_scsi_intr)();
	int mg_scsi_intrarg;
	short mg_minor, mg_seq;
	int (*mg_anim_run)();
	short mg_major;
	char *mg_clientetheraddr;
	int mg_console_i;
	int mg_console_o;
#define	CONS_I_KBD	0
#define	CONS_I_SCC_A	1
#define	CONS_I_SCC_B	2
#define	CONS_I_NET	3
#define	CONS_O_BITMAP	0
#define	CONS_O_SCC_A	1
#define	CONS_O_SCC_B	2
#define	CONS_O_NET	3
	char *test_msg;
	/* Next entry should be km_coni. Mach depends on this! */
	struct km_console_info km_coni;	/* Console configuration info. See kmreg.h */
	char *mg_fdgp;
	char mg_machine_type, mg_board_rev;
	int (*mg_as_tune)();
	int mg_flags2;
#define	MGF2_PARITY	0x80000000
	volatile struct bmap *mg_bmap_chip;
	enum mg_pkg {PKG_CUBE, PKG_NS} mg_pkg;
	enum mg_memory_system {MEMSYS_8, MEMSYS_32} mg_memory_system;
	enum mg_video_system {VIDSYS_313, VIDSYS_W9C, VIDSYS_PC} mg_video_system;
	int mg_cpu_clk;
};

#pragma pack(pop)

extern struct mon_global *mg;

#ifdef __cplusplus
}
#endif
