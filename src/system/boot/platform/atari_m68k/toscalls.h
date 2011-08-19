/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TOSCALLS_H
#define _TOSCALLS_H


#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
#include <OS.h>

/* TOS calls use 16 bit param alignment, so we must generate the calls ourselves.
 * We then use asm macros, one for each possible arg type and count.
 * cf. how mint does it in sys/mint/arch/asm_misc.h
 */
//#if __GNUC__ >= 3
#define TOS_CLOBBER_LIST "d1", "d2", "a0", "a1", "a2"
//#else
//#error fixme
//#endif

/* void (no) arg */
#define toscallV(trapnr, callnr)				\
({												\
	register int32 retvalue __asm__("d0");		\
												\
	__asm__ volatile							\
	("/* toscall(" #trapnr ", " #callnr ") */\n"	\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	trap	%[trapn]\n"						\
	"	add.l	#2,%%sp\n"						\
	: "=r"(retvalue)	/* output */			\
	: [trapn]"i"(trapnr),[calln]"i"(callnr) 	\
									/* input */	\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);											\
	retvalue;									\
})

#define toscallW(trapnr, callnr, p1)			\
({												\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);					\
												\
	__asm__ volatile							\
	("/* toscall(" #trapnr ", " #callnr ") */\n"	\
	"	move.w	%1,-(%%sp) \n"					\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	trap	%[trapn]\n"						\
	"	add.l	#4,%%sp \n"						\
	: "=r"(retvalue)			/* output */	\
	: "r"(_p1),						/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);											\
	retvalue;									\
})

#define toscallL(trapnr, callnr, p1)			\
({							\
	register int32 retvalue __asm__("d0");		\
	int32 _p1 = (int32)(p1);			\
							\
	__asm__ volatile				\
	(/*"; toscall(" #trapnr ", " #callnr ")"*/"\n	\
		move.l	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#6,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: "r"(_p1),			/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

#define toscallWW(trapnr, callnr, p1, p2)		\
({							\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);			\
	int16 _p2 = (int16)(p2);			\
							\
	__asm__ volatile				\
	(/*"; toscall(" #trapnr ", " #callnr ")"*/"\n	\
		move.w	%2,-(%%sp) \n			\
		move.w	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#6,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: "r"(_p1), "r"(_p2),		/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

#define toscallWWL(trapnr, callnr, p1, p2, p3)		\
({							\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);			\
	int16 _p2 = (int16)(p2);			\
	int32 _p3 = (int32)(p3);			\
							\
	__asm__ volatile				\
	(/*"; toscall(" #trapnr ", " #callnr ")"*/"\n	\
		move.l	%3,-(%%sp) \n			\
		move.w	%2,-(%%sp) \n			\
		move.w	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#10,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3),			/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

#define toscallWLWWWL(trapnr, callnr, p1, p2, p3, p4, p5, p6)	\
({							\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);			\
	int32 _p2 = (int32)(p2);			\
	int16 _p3 = (int16)(p3);			\
	int16 _p4 = (int16)(p4);			\
	int16 _p5 = (int16)(p5);			\
	int32 _p6 = (int32)(p6);			\
							\
	__asm__ volatile				\
	(/*"; toscall(" #trapnr ", " #callnr ")"*/"\n	\
		move.l	%6,-(%%sp) \n			\
		move.w	%5,-(%%sp) \n			\
		move.w	%4,-(%%sp) \n			\
		move.w	%3,-(%%sp) \n			\
		move.l	%2,-(%%sp) \n			\
		move.w	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#18,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3), "r"(_p4),				\
	  "r"(_p5), "r"(_p6),		/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

#define toscallLLWW(trapnr, callnr, p1, p2, p3, p4)	\
({							\
	register int32 retvalue __asm__("d0");		\
	int32 _p1 = (int32)(p1);			\
	int32 _p2 = (int32)(p2);			\
	int16 _p3 = (int16)(p3);			\
	int16 _p4 = (int16)(p4);			\
							\
	__asm__ volatile				\
	(/*"; toscall(" #trapnr ", " #callnr ")"*/"\n	\
		move.w	%4,-(%%sp) \n			\
		move.w	%3,-(%%sp) \n			\
		move.l	%2,-(%%sp) \n			\
		move.l	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#14,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3), "r"(_p4),		/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

#define toscallLLWWWWW(trapnr, callnr, p1, p2, p3, p4, p5, p6, p7)	\
({							\
	register int32 retvalue __asm__("d0");		\
	int32 _p1 = (int32)(p1);			\
	int32 _p2 = (int32)(p2);			\
	int16 _p3 = (int16)(p3);			\
	int16 _p4 = (int16)(p4);			\
	int16 _p5 = (int16)(p5);			\
	int16 _p6 = (int16)(p6);			\
	int16 _p7 = (int16)(p7);			\
							\
	__asm__ volatile				\
	(/*"; toscall(" #trapnr ", " #callnr ")"*/"\n	\
		move.w	%7,-(%%sp) \n			\
		move.w	%6,-(%%sp) \n			\
		move.w	%5,-(%%sp) \n			\
		move.w	%4,-(%%sp) \n			\
		move.w	%3,-(%%sp) \n			\
		move.l	%2,-(%%sp) \n			\
		move.l	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#18,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3), "r"(_p4),				\
	  "r"(_p5), "r"(_p6),				\
	  "r"(_p7),					/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

/* pointer versions */
#define toscallP(trapnr, callnr, a) toscallL(trapnr, callnr, (int32)a)
#define toscallWWP(trapnr, callnr, p1, p2, p3)		\
	toscallWWL(trapnr, callnr, p1, p2, (int32)p3)
#define toscallWPWWWL(trapnr, callnr, p1, p2, p3, p4, p5, p6) \
	toscallWLWWWL(trapnr, callnr, p1, (int32)p2, p3, p4, p5, p6)
#define toscallPLWWWWW(trapnr, callnr, p1, p2, p3, p4, p5, p6, p7)		\
	toscallLLWWWWW(trapnr, callnr, (int32)p1, (int32)p2, p3, p4, p5, p6, p7)
#define toscallPPWW(trapnr, callnr, p1, p2, p3, p4)		\
	toscallLLWW(trapnr, callnr, (int32)p1, (int32)p2, p3, p4)


#endif /* __ASSEMBLER__ */

#ifdef __ASSEMBLER__
#define _TOSV_P(a) a
#define _TOSV_L(a) a
#define _TOSV_W(a) a
#define _TOSV_B(a) a
#else
#define _TOSV_P(a) ((void **)a)
#define _TOSV_L(a) ((uint32 *)a)
#define _TOSV_W(a) ((uint16 *)a)
#define _TOSV_B(a) ((uint8 *)a)
#endif

/* 
 * TOS Variables
 * only relevant ones,
 * see http://toshyp.atari.org/en/003004.html
 */
#define TOSVAR_autopath	_TOSV_P(0x4ca)
#define TOSVAR_bootdev	_TOSV_W(0x446)
#define TOSVAR_dskbufp	_TOSV_P(0x4c6)
#define TOSVAR_drvbits	_TOSV_L(0x4c2)
#define TOSVAR_frclock	_TOSV_L(0x466)
#define TOSVAR_hz_200	_TOSV_L(0x4ba)
#define TOSVAR_membot	_TOSV_L(0x432)
#define TOSVAR_memtop	_TOSV_L(0x436)
#define TOSVAR_nflops	_TOSV_W(0x4a6)
#define TOSVAR_p_cookies	_TOSV_P(0x5A0)
#define TOSVAR_sysbase	_TOSV_P(0x4f2)
#define TOSVAR_timr_ms	_TOSV_W(0x442)
#define TOSVAR_v_bas_ad	_TOSV_P(0x44e)
#define TOSVAR_vbclock	_TOSV_L(0x462)
#define TOSVARphystop	_TOSV_L(0x42e)
#define TOSVARramtop	_TOSV_L(0x5a4)
#define TOSVARramvalid	_TOSV_L(0x5a8)
#define TOSVARramvalid_MAGIC 0x1357bd13


#define BIOS_TRAP	13
#define XBIOS_TRAP	14
#define GEMDOS_TRAP	1

/* 
 * Atari BIOS calls
 */

/* those are used by asm code too */

#define DEV_PRINTER	0
#define DEV_AUX	1
#define DEV_CON	2
#define DEV_CONSOLE	2
#define DEV_MIDI	3
#define DEV_IKBD	4
#define DEV_RAW	5

#define K_RSHIFT	0x01
#define K_LSHIFT	0x02
#define K_CTRL	0x04
#define K_ALT	0x08
#define K_CAPSLOCK	0x10
#define K_CLRHOME	0x20
#define K_INSERT	0x40

#define RW_READ			0x00
#define RW_WRITE		0x01
#define RW_NOMEDIACH	0x02
#define RW_NORETRY		0x04
#define RW_NOTRANSLATE	0x08

#ifndef __ASSEMBLER__

//extern int32 bios(uint16 nr, ...);

// cf. http://www.fortunecity.com/skyscraper/apple/308/html/bios.htm

struct tos_bpb {
	int16 recsiz;
	int16 clsiz;
	int16 clsizb;
	int16 rdlen;
	int16 fsiz;
	int16 fatrec;
	int16 datrec;
	int16 numcl;
	int16 bflags;
};

struct tos_pun_info {
	int16 puns;
	uint8 pun[16];
	int32 part_start[16]; // unsigned ??
	uint32 p_cookie;
	struct tos_pun_info *p_cooptr; // points to itself
	uint16 p_version;
	uint16 p_max_sector;
	int32 reserved[16];
};
#define PUN_INFO ((struct tos_pun_info *)0x516L)


//#define Getmpb() toscallV(BIOS_TRAP, 0)
#define Bconstat(dev) toscallW(BIOS_TRAP, 1, (uint16)dev)
#define Bconin(dev) toscallW(BIOS_TRAP, 2, (uint16)dev)
#define Bconout(dev, chr) toscallWW(BIOS_TRAP, 3, (uint16)dev, (uint16)chr)
#define Rwabs(mode, buf, count, recno, dev, lrecno) toscallWPWWWL(BIOS_TRAP, 4, (int16)mode, (void *)buf, (int16)count, (int16)recno, (uint16)dev, (int32)lrecno)
//#define Setexc() toscallV(BIOS_TRAP, 5, )
#define Tickcal() toscallV(BIOS_TRAP, 6)
#define Getbpb(dev) (struct tos_bpb *)toscallW(BIOS_TRAP, 7, (uint16)dev)
#define Bcostat(dev) toscallW(BIOS_TRAP, 8, (uint16)dev)
#define Mediach(dev) toscallW(BIOS_TRAP, 9, (int16)dev)
#define Drvmap() (uint32)toscallV(BIOS_TRAP, 10)
#define Kbshift(mode) toscallW(BIOS_TRAP, 11, (uint16)mode)

/* handy shortcut */
static inline int Bconput(int16 handle, const char *string)
{
	int i, col, err;
	for (i = 0, col = 0; string[i]; i++, col++) {
		if (string[i] == '\n') {
			Bconout(handle, '\r');
			col = 0;
		}
		/* hard wrap at 80 col as the ST console emulation doesn't do this. */
		if (col == 80) {
			Bconout(handle, '\r');
			Bconout(handle, '\n');
			col = 0;
		}
		err = Bconout(handle, string[i]);
		if (err < 0)
			break;
	}
	return i;
}

static inline int Bconputs(int16 handle, const char *string)
{
	int err = Bconput(handle, string);
	Bconout(handle, '\r');
	Bconout(handle, '\n');
	return err;
}

#endif /* __ASSEMBLER__ */

/* 
 * Atari XBIOS calls
 */

#define IM_DISABLE	0
#define IM_RELATIVE	1
#define IM_ABSOLUTE	2
#define IM_KEYCODE	4

#define NVM_READ	0
#define NVM_WRITE	1
#define NVM_RESET	2
// unofficial
#define NVM_R_SEC	0
#define NVM_R_MIN	2
#define NVM_R_HOUR	4
#define NVM_R_MDAY	7
#define NVM_R_MON	8	/*+- 1*/
#define NVM_R_YEAR	9
#define NVM_R_VIDEO	29

#define VM_INQUIRE	-1

/* Milan specific video constants */
#define MI_MAGIC	0x4d49
#define CMD_GETMODE		0
#define CMD_SETMODE		1
#define CMD_GETINFO		2
#define CMD_ALLOCPAGE	3
#define CMD_FREEPAGE	4
#define CMD_FLIPPAGE	5
#define CMD_ALLOCMEM	6
#define CMD_FREEMEM		7
#define CMD_SETADR		8
#define CMD_ENUMMODES	9
#define ENUMMODE_EXIT	0
#define ENUMMODE_CONT	1
/* scrFlags */
#define SCRINFO_OK	1
/* scrFormat */
#define INTERLEAVE_PLANES	0
#define STANDARD_PLANES		1
#define PACKEDPIX_PLANES	2
/* bitFlags */
#define STANDARD_BITS	1
#define FALCON_BITS		2
#define INTEL_BITS		8


#ifndef __ASSEMBLER__

/* Milan specific video stuff */
typedef struct screeninfo {
	int32	size;
	int32	devID;
	char	name[64];
	int32	scrFlags;
	int32	frameadr;
	int32	scrHeight;
	int32	scrWidth;
	int32	virtHeight;
	int32	virtWidth;
	int32	scrPlanes;
	int32	scrColors;
	int32	lineWrap;
	int32	planeWrap;
	int32	scrFormat;
	int32	scrClut;
	int32	redBits;
	int32	greenBits;
	int32	blueBits;
	int32	alphaBits;
	int32	genlockBits;
	int32	unusedBits;
	int32	bitFlags;
	int32	maxmem;
	int32	pagemem;
	int32	max_x;
	int32	max_y;
} SCREENINFO;


//extern int32 xbios(uint16 nr, ...);


#define Initmous(mode, param, vec) toscallWPP(XBIOS_TRAP, 0, (int16)mode, (void *)param, (void *)vec)
#define Physbase() (void *)toscallV(XBIOS_TRAP, 2)
#define Logbase() (void *)toscallV(XBIOS_TRAP, 3)
#define Getrez() toscallV(XBIOS_TRAP, 4)
#define Setscreen(log, phys, mode, command) toscallPPWW(XBIOS_TRAP, 5, (void *)log, (void *)phys, (int16)mode, (int16)command)
#define VsetScreen(log, phys, mode, modecode) toscallPPWW(XBIOS_TRAP, 5, (void *)log, (void *)phys, (int16)mode, (int16)modecode)
#define Floprd(buf, dummy, dev, sect, track, side, count) toscallPLWWWWW(XBIOS_TRAP, 8, (void *)buf, (int32)dummy, (int16)dev, (int16)sect, (int16)track, (int16)side, (int16)count)
//#define Mfpint() toscallV(XBIOS_TRAP, 13, )
#define Rsconf(speed, flow, ucr, rsr, tsr, scr) toscallWWWWWW(XBIOS_TRAP, 15, (int16)speed, (int16)flow, (int16)ucr, (int16)rsr, (int16)tsr, (int16)scr)
//#define Keytbl(unshift, shift, caps) (KEYTAB *)toscallPPP(XBIOS_TRAP, 16, (char *)unshift, (char *)shift, (char *)caps)
#define Random() toscallV(XBIOS_TRAP, 17)
#define Gettime() (uint32)toscallV(XBIOS_TRAP, 23)
#define Jdisint(intno) toscallW(XBIOS_TRAP, 26, (int16)intno)
#define Jenabint(intno) toscallW(XBIOS_TRAP, 27, (int16)intno)
#define Supexec(func) toscallP(XBIOS_TRAP, 38, (void *)func)
#define Puntaes() toscallV(XBIOS_TRAP, 39)
#define DMAread(sect, count, buf, dev) toscallLWPW(XBIOS_TRAP, 42, (int32)sect, (int16)count, (void *)buf, (int16)dev)
#define DMAwrite(sect, count, buf, dev) toscallWPLW(XBIOS_TRAP, 43, (int32)sect, (int16)count, (void *)buf, (int16)dev)
#define NVMaccess(op, start, count, buffer) toscallWWWP(XBIOS_TRAP, 46, (int16)op, (int16)start, (int16)count, (char *)buffer)
#define VsetMode(mode) toscallW(XBIOS_TRAP, 88, (int16)mode)
#define VgetMonitor() toscallV(XBIOS_TRAP, 89)
#define mon_type() toscallV(XBIOS_TRAP, 89)
#define VgetSize(mode) toscallW(XBIOS_TRAP, 91, (int16)mode)
#define VsetRGB(index, count, array) toscallWWP(XBIOS_TRAP, 93, (int16)index, (int16)count, (void *)array)
#define Locksnd() toscallV(XBIOS_TRAP, 128)
#define Unlocksnd() toscallV(XBIOS_TRAP, 129)

#endif /* __ASSEMBLER__ */

/* 
 * Atari GEMDOS calls
 */

#define SUP_USER		0
#define SUP_SUPER		1


#ifdef __ASSEMBLER__
#define SUP_SET			0
#define SUP_INQUIRE		1
#else

//extern int32 gemdos(uint16 nr, ...);

#define SUP_SET			(void *)0
#define SUP_INQUIRE		(void *)1

// official names
#define Pterm0() toscallV(GEMDOS_TRAP, 0)
#define Cconin() toscallV(GEMDOS_TRAP, 1)
#define Super(s) toscallP(GEMDOS_TRAP, 0x20, s)
#define Pterm(retcode) toscallW(GEMDOS_TRAP, 76, (int16)retcode)

#endif /* __ASSEMBLER__ */

#ifndef __ASSEMBLER__

/* 
 * MetaDOS XBIOS calls
 */

//#define _DISABLE	0

#ifndef __ASSEMBLER__

#define Metainit(buf) toscallP(XBIOS_TRAP, 48, (void *)buf)
#define Metaopen(drive, p) toscall>P(XBIOS_TRAP, 48, (void *)buf)
#define Metaclose(drive) toscallW(XBIOS_TRAP, 50, (int16)drive)
#define Metagettoc(drive, flag, p) toscallW(XBIOS_TRAP, 62, (int16)drive, (int16)flag, (void *)p)
#define Metadiscinfo(drive, p) toscallWP(XBIOS_TRAP, 63, (int16)drive, (void *)p)
#define Metaioctl(drive, magic, op, buf) toscallWLWP(XBIOS_TRAP, 55, (int16)drive, (int32)magic, (int16)op, )
//#define Meta(drive) toscallW(XBIOS_TRAP, 50, (int16)drive)
//#define Meta(drive) toscallW(XBIOS_TRAP, 50, (int16)drive)
//#define Meta(drive) toscallW(XBIOS_TRAP, 50, (int16)drive)

#endif /* __ASSEMBLER__ */

/*
 * XHDI support
 * see http://toshyp.atari.org/010008.htm
 */

#define XHDI_COOKIE 'XHDI'
#define XHDI_MAGIC 0x27011992
//#define XHDI_CLOBBER_LIST "" /* only d0 */
#define XHDI_CLOBBER_LIST "d1", "d2", "a0", "a1", "a2"

#define XH_TARGET_STOPPABLE		0x00000001L
#define XH_TARGET_REMOVABLE		0x00000002L
#define XH_TARGET_LOCKABLE		0x00000004L
#define XH_TARGET_EJECTABLE		0x00000008L
#define XH_TARGET_LOCKED		0x20000000L
#define XH_TARGET_STOPPED		0x40000000L
#define XH_TARGET_RESERVED		0x80000000L

#ifndef __ASSEMBLER__

/* pointer to the XHDI dispatch function */
extern void *gXHDIEntryPoint;

extern status_t init_xhdi(void);

/* movem should not needed, but just to be safe. */

/* void (no) arg */
#define xhdicallV(callnr)						\
({												\
	register int32 retvalue __asm__("d0");		\
												\
	__asm__ volatile							\
	("/* xhdicall(" #callnr ") */\n"			\
	"	movem.l	%%d3-%%d7/%%a3-%%a6,-(%%sp)\n"		\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	jbsr	(%[entry])\n"						\
	"	add.l	#2,%%sp\n"						\
	"	movem.l	(%%sp)+,%%d3-%%d7/%%a3-%%a6\n"		\
	: "=r"(retvalue)	/* output */			\
	:							/* input */		\
	  [entry]"a"(gXHDIEntryPoint),				\
	  [calln]"i"(callnr)						\
	: XHDI_CLOBBER_LIST /* clobbered regs */	\
	);											\
	retvalue;									\
})

#define xhdicallW(callnr, p1)					\
({												\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);					\
												\
	__asm__ volatile							\
	("/* xhdicall(" #callnr ") */\n"			\
	"	movem.l	%%d3-%%d7/%%a3-%%a6,-(%%sp)\n"		\
	"	move.w	%1,-(%%sp) \n"					\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	jbsr	(%[entry])\n"						\
	"	add.l	#4,%%sp \n"						\
	"	movem.l	(%%sp)+,%%d3-%%d7/%%a3-%%a6\n"		\
	: "=r"(retvalue)	/* output */			\
	: "r"(_p1),			/* input */				\
	  [entry]"a"(gXHDIEntryPoint),				\
	  [calln]"i"(callnr)						\
	: XHDI_CLOBBER_LIST /* clobbered regs */	\
	);											\
	retvalue;									\
})

#define xhdicallWWL(callnr, p1, p2, p3)					\
({												\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);					\
	int16 _p2 = (int16)(p2);					\
	int32 _p3 = (int32)(p3);					\
												\
	__asm__ volatile							\
	("/* xhdicall(" #callnr ") */\n"			\
	"	movem.l	%%d3-%%d7/%%a3-%%a6,-(%%sp)\n"		\
	"	move.l	%3,-(%%sp) \n"					\
	"	move.w	%2,-(%%sp) \n"					\
	"	move.w	%1,-(%%sp) \n"					\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	jbsr	(%[entry])\n"						\
	"	add.l	#10,%%sp \n"						\
	"	movem.l	(%%sp)+,%%d3-%%d7/%%a3-%%a6\n"		\
	: "=r"(retvalue)	/* output */			\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3),			/* input */	\
	  [entry]"a"(gXHDIEntryPoint),				\
	  [calln]"i"(callnr)						\
	: XHDI_CLOBBER_LIST /* clobbered regs */	\
	);											\
	retvalue;									\
})

#define xhdicallWWLL(callnr, p1, p2, p3, p4)					\
({												\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);					\
	int16 _p2 = (int16)(p2);					\
	int32 _p3 = (int32)(p3);					\
	int32 _p4 = (int32)(p4);					\
												\
	__asm__ volatile							\
	("/* xhdicall(" #callnr ") */\n"			\
	"	movem.l	%%d3-%%d7/%%a3-%%a6,-(%%sp)\n"		\
	"	move.l	%4,-(%%sp) \n"					\
	"	move.l	%3,-(%%sp) \n"					\
	"	move.w	%2,-(%%sp) \n"					\
	"	move.w	%1,-(%%sp) \n"					\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	jbsr	(%[entry])\n"						\
	"	add.l	#14,%%sp \n"						\
	"	movem.l	(%%sp)+,%%d3-%%d7/%%a3-%%a6\n"		\
	: "=r"(retvalue)	/* output */			\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3), "r"(_p4),	/* input */	\
	  [entry]"a"(gXHDIEntryPoint),				\
	  [calln]"i"(callnr)						\
	: XHDI_CLOBBER_LIST /* clobbered regs */	\
	);											\
	retvalue;									\
})

#define xhdicallWWLLL(callnr, p1, p2, p3, p4, p5)					\
({												\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);					\
	int16 _p2 = (int16)(p2);					\
	int32 _p3 = (int32)(p3);					\
	int32 _p4 = (int32)(p4);					\
	int32 _p5 = (int32)(p5);					\
												\
	__asm__ volatile							\
	("/* xhdicall(" #callnr ") */\n"			\
	"	movem.l	%%d3-%%d7/%%a3-%%a6,-(%%sp)\n"		\
	"	move.l	%5,-(%%sp) \n"					\
	"	move.l	%4,-(%%sp) \n"					\
	"	move.l	%3,-(%%sp) \n"					\
	"	move.w	%2,-(%%sp) \n"					\
	"	move.w	%1,-(%%sp) \n"					\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	jbsr	(%[entry])\n"						\
	"	add.l	#18,%%sp \n"						\
	"	movem.l	(%%sp)+,%%d3-%%d7/%%a3-%%a6\n"		\
	: "=r"(retvalue)	/* output */			\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3), "r"(_p4),				\
	  "r"(_p5),			/* input */	\
	  [entry]"a"(gXHDIEntryPoint),				\
	  [calln]"i"(callnr)						\
	: XHDI_CLOBBER_LIST /* clobbered regs */	\
	);											\
	retvalue;									\
})

#define xhdicallWLLLL(callnr, p1, p2, p3, p4, p5)					\
({												\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);					\
	int32 _p2 = (int32)(p2);					\
	int32 _p3 = (int32)(p3);					\
	int32 _p4 = (int32)(p4);					\
	int32 _p5 = (int32)(p5);					\
												\
	__asm__ volatile							\
	("/* xhdicall(" #callnr ") */\n"			\
	"	movem.l	%%d3-%%d7/%%a3-%%a6,-(%%sp)\n"		\
	"	move.l	%5,-(%%sp) \n"					\
	"	move.l	%4,-(%%sp) \n"					\
	"	move.l	%3,-(%%sp) \n"					\
	"	move.l	%2,-(%%sp) \n"					\
	"	move.w	%1,-(%%sp) \n"					\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	jbsr	(%[entry])\n"						\
	"	add.l	#20,%%sp \n"						\
	"	movem.l	(%%sp)+,%%d3-%%d7/%%a3-%%a6\n"		\
	: "=r"(retvalue)	/* output */			\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3), "r"(_p4),				\
	  "r"(_p5),			/* input */	\
	  [entry]"a"(gXHDIEntryPoint),				\
	  [calln]"i"(callnr)						\
	: XHDI_CLOBBER_LIST /* clobbered regs */	\
	);											\
	retvalue;									\
})

#define xhdicallWWWLWL(callnr, p1, p2, p3, p4, p5, p6)					\
({												\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);					\
	int16 _p2 = (int16)(p2);					\
	int16 _p3 = (int16)(p3);					\
	int32 _p4 = (int32)(p4);					\
	int16 _p5 = (int16)(p5);					\
	int32 _p6 = (int32)(p6);					\
												\
	__asm__ volatile							\
	("/* xhdicall(" #callnr ") */\n"			\
	"	movem.l	%%d3-%%d7/%%a3-%%a6,-(%%sp)\n"		\
	"	move.l	%6,-(%%sp) \n"					\
	"	move.w	%5,-(%%sp) \n"					\
	"	move.l	%4,-(%%sp) \n"					\
	"	move.w	%3,-(%%sp) \n"					\
	"	move.w	%2,-(%%sp) \n"					\
	"	move.w	%1,-(%%sp) \n"					\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	jbsr	(%[entry])\n"						\
	"	add.l	#18,%%sp \n"						\
	"	movem.l	(%%sp)+,%%d3-%%d7/%%a3-%%a6\n"		\
	: "=r"(retvalue)	/* output */			\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3), "r"(_p4),				\
	  "r"(_p5), "r"(_p6),		/* input */	\
	  [entry]"a"(gXHDIEntryPoint),				\
	  [calln]"i"(callnr)						\
	: XHDI_CLOBBER_LIST /* clobbered regs */	\
	);											\
	retvalue;									\
})

#define xhdicallWWP(callnr, p1, p2, p3) \
	xhdicallWWL(callnr, p1, p2, (int32)(p3))
#define xhdicallWWPP(callnr, p1, p2, p3, p4) \
	xhdicallWWLL(callnr, p1, p2, (int32)(p3), (int32)(p4))
#define xhdicallWWPPP(callnr, p1, p2, p3, p4, p5) \
	xhdicallWWLLL(callnr, p1, p2, (int32)(p3), (int32)(p4), (int32)(p5))
#define xhdicallWPPPP(callnr, p1, p2, p3, p4, p5) \
	xhdicallWLLLL(callnr, p1, (uint32)(p2), (int32)(p3), (int32)(p4), (int32)(p5))
#define xhdicallWWWLWP(callnr, p1, p2, p3, p4, p5, p6) \
	xhdicallWWWLWL(callnr, p1, p2, p3, (uint32)(p4), p5, (uint32)(p6))
#define xhdicallWWWPWP(callnr, p1, p2, p3, p4, p5, p6) \
	xhdicallWWWLWL(callnr, p1, p2, p3, (int32)(p4), p5, (uint32)(p6))

#define XHGetVersion() (uint16)xhdicallV(0)
#define XHInqTarget(major, minor, blocksize, flags, pname) xhdicallWWPPP(1, (uint16)major, (uint16)minor, (uint32 *)(blocksize), (uint32 *)flags, (char *)pname)
//XHReserve 2
//#define XHLock() 3
//#define XHStop() 4
#define XHEject(major, minor, doeject, key) xhdicallWWWW(5, (uint16)major, (uint16)minor, (uint16)doeject, (uint16)key)
#define XHDrvMap() xhdicallV(6)
#define XHInqDev(dev,major,minor,startsect,bpb) xhdicallWPPPP(7,dev,(uint16 *)major,(uint16 *)minor,(uint32 *)startsect,(struct tos_bpb *)bpb)
//XHInqDriver 8
//XHNewCookie 9
#define XHReadWrite(major, minor, rwflags, recno, count, buf) xhdicallWWWLWP(10, (uint16)major, (uint16)minor, (uint16)rwflags, (uint32)recno, (uint16)count, (void *)buf)
#define XHInqTarget2(major, minor, bsize, flags, pname, pnlen) xhdicallWWPPPW(11, (uint16)major, (uint16)minor, (uint32 *)bsize, (uint32 *)flags, (char *)pname, (uint16)pnlen)
//XHInqDev2 12
//XHDriverSpecial 13
#define XHGetCapacity(major, minor, blocks, blocksize) xhdicallWWPP(14, (uint16)major, (uint16)minor, (uint32 *)blocks, (uint32 *)blocksize)
//#define XHMediumChanged() 15
//XHMiNTInfo 16
//XHDosLimits 17
#define XHLastAccess(major, minor, ms) xhdicallWWP(18, major, minor, (uint32 *)ms)
//SHReaccess 19

#endif /* __ASSEMBLER__ */


/*
 * error mapping
 * in debug.c
 */

extern status_t toserror(int32 err);
extern status_t xhdierror(int32 err);
extern void dump_tos_cookies(void);

/*
 * Cookie Jar access
 */

typedef struct tos_cookie {
	uint32 cookie;
	union {
		int32 ivalue;
		void *pvalue;
	};
} tos_cookie;

#define COOKIE_JAR (*((const tos_cookie **)TOSVAR_p_cookies))

static inline const tos_cookie *tos_find_cookie(uint32 what)
{
	const tos_cookie *c = COOKIE_JAR;
	while (c && (c->cookie)) {
		if (c->cookie == what)
			return c;
		c++;
	}
	return NULL;
}

/*
 * OSHEADER access
 */

typedef struct tos_osheader {
	uint16 os_entry;
	uint16 os_version;
	void *reseth;
	struct tos_osheader *os_beg;
	void *os_end;
	void *os_rsv1;
	void *os_magic;
	uint32 os_date;
	uint32 os_conf;
	//uint32/16? os_dosdate;
	// ... more stuff we don't care about
} tos_osheader;

#define tos_sysbase ((const struct tos_osheader **)0x4F2)

static inline const struct tos_osheader *tos_get_osheader()
{
	if (!(*tos_sysbase))
		return NULL;
	return (*tos_sysbase)->os_beg;
}

#endif /* __ASSEMBLER__ */

/*
 * ARAnyM Native Features
 */

#define NF_COOKIE	0x5f5f4e46L	//'__NF'
#define NF_MAGIC	0x20021021L

#ifndef __ASSEMBLER__

typedef struct {
	uint32 magic;
	uint32 (*nfGetID) (const char *);
	int32 (*nfCall) (uint32 ID, ...);
} NatFeatCookie;

extern NatFeatCookie *gNatFeatCookie;
extern uint32 gDebugPrintfNatFeatID;

static inline NatFeatCookie *nat_features(void)
{
	const struct tos_cookie *c;
	if (gNatFeatCookie == (void *)-1)
		return NULL;
	if (gNatFeatCookie)
		return gNatFeatCookie;
	c = tos_find_cookie(NF_COOKIE);
	if (c) {
		gNatFeatCookie = (NatFeatCookie *)c->pvalue;
		if (gNatFeatCookie && gNatFeatCookie->magic == NF_MAGIC) {
			return gNatFeatCookie;
		}
	}
	gNatFeatCookie = (NatFeatCookie *)-1;
	return NULL;
}

extern status_t init_nat_features(void);

static inline int32 nat_feat_getid(const char *name)
{
	NatFeatCookie *c = nat_features();
	if (!c)
		return 0;
	return c->nfGetID(name);
}

#define nat_feat_call(id, code, a...) \
({						\
	int32 ret = -1;				\
	NatFeatCookie *c = nat_features();	\
	if (c)					\
		ret = c->nfCall(id | code, ##a);	\
	ret;					\
})

extern void nat_feat_debugprintf(const char *str);

extern int nat_feat_get_bootdrive(void);
extern status_t nat_feat_get_bootargs(char *str, long size);

#endif /* __ASSEMBLER__ */

/*
 * Drive API used by the bootsector
 * gBootDriveAPI is set to one of those
 * not all are implemented
 */
#define ATARI_BOOT_DRVAPI_UNKNOWN 0
#define ATARI_BOOT_DRVAPI_FLOPPY  1   // Floprd()
#define ATARI_BOOT_DRVAPI_BIOS    2   // Rwabs()
#define ATARI_BOOT_DRVAPI_XBIOS   3   // DMAread()
#define ATARI_BOOT_DRVAPI_XHDI    4   // XHReadWrite()
#define ATARI_BOOT_DRVAPI_METADOS 5   // Metaread()

#ifdef __cplusplus
}
#endif

#endif /* _TOSCALLS_H */
