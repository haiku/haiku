/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
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
#define toscallV(trapnr, callnr)			\
({							\
	register int32 retvalue __asm__("d0");		\
							\
	__asm__ volatile				\
	("/* toscall(" #trapnr ", " #callnr ") */\n	\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#2,%%sp\n"			\
	: "=r"(retvalue)	/* output */		\
	: [trapn]"i"(trapnr),[calln]"i"(callnr) 	\
					/* input */	\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

#define toscallW(trapnr, callnr, p1)			\
({							\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);			\
							\
	__asm__ volatile				\
	("/* toscall(" #trapnr ", " #callnr ") */\n	\
		move.w	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#4,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: [trapn]"i"(trapnr),[calln]"i"(callnr), 	\
	  "r"(_p1)			/* input */	\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
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
	: [trapn]"i"(trapnr),[calln]"i"(callnr),	\
	  "r"(_p1)			/* input */	\
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
	: [trapn]"i"(trapnr),[calln]"i"(callnr),	\
	  "r"(_p1), "r"(_p2)		/* input */	\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})


/* pointer versions */
#define toscallP(trapnr, callnr, a) toscallL(trapnr, callnr, (int32)a)

#endif /* __ASSEMBLER__ */

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

struct tosbpb {
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


//#define Getmpb() toscallV(BIOS_TRAP, 0)
#define Bconstat(dev) toscallW(BIOS_TRAP, 1, (uint16)dev)
#define Bconin(dev) toscallW(BIOS_TRAP, 2, (uint16)dev)
#define Bconout(dev, chr) toscallWW(BIOS_TRAP, 3, (uint16)dev, (uint16)chr)
#define Rwabs(mode, buf, count, recno, dev, lrecno) toscallWPWWWL(BIOS_TRAP, 4, (int16)mode, (void *)buf, (int16)count, (int16)recno, (uint16)dev, (int32)lrecno)
//#define Setexc() toscallV(BIOS_TRAP, 5, )
#define Tickcal() toscallV(BIOS_TRAP, 6)
#define Getbpb(dev) (struct tosbpb *)toscallW(BIOS_TRAP, 7, (uint16)dev)
#define Bcostat(dev) toscallW(BIOS_TRAP, 8, (uint16)dev)
#define Mediach(dev) toscallW(BIOS_TRAP, 9, (int16)dev)
#define Drvmap() (uint32)toscallV(BIOS_TRAP, 10)
#define Kbshift(mode) toscallW(BIOS_TRAP, 11, (uint16)mode)

/* handy shortcut */
static inline int Bconputs(int16 handle, const char *string)
{
	int i, err;
	for (i = 0; string[i]; i++) {
		err = Bconout(handle, string[i]);
		if (err)
			return i;
	}
	return i;
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


#ifndef __ASSEMBLER__

//extern int32 xbios(uint16 nr, ...);


#define Initmous(mode, param, vec) toscallWPP(XBIOS_TRAP, 0, (int16)mode, (void *)param, (void *)vec)
#define Physbase() (void *)toscallV(XBIOS_TRAP, 2)
#define Logbase() (void *)toscallV(XBIOS_TRAP, 3)
//#define Getrez() toscallV(XBIOS_TRAP, 4)
#define Setscreen(log, phys, mode) toscallPPW(XBIOS_TRAP, 5, (void *)log, (void *)phys, (int16)mode)
#define VsetScreen(log, phys, mode, modecode) toscallPPW(XBIOS_TRAP, 5, (void *)log, (void *)phys, (int16)mode)
//#define Mfpint() toscallV(XBIOS_TRAP, 13, )
#define Rsconf(speed, flow, ucr, rsr, tsr, scr) toscallWWWWWW(XBIOS_TRAP, 15, (int16)speed, (int16)flow, (int16)ucr, (int16)rsr, (int16)tsr, (int16)scr)
//#define Keytbl(unshift, shift, caps) (KEYTAB *)toscallPPP(XBIOS_TRAP, 16, (char *)unshift, (char *)shift, (char *)caps)
#define Random() toscallV(XBIOS_TRAP, 17)
#define Gettime() (uint32)toscallV(XBIOS_TRAP, 23)
#define Jdisint(intno) toscallW(XBIOS_TRAP, 26, (int16)intno)
#define Jenabint(intno) toscallW(XBIOS_TRAP, 27, (int16)intno)
#define Supexec(func) toscallP(XBIOS_TRAP, 38, (void *)func)
//#define Puntaes() toscallV(XBIOS_TRAP, 39)
#define DMAread(sect, count, buf, dev) toscallLWPW(XBIOS_TRAP, 42, (int32)sect, (int16)count, (void *)buf, (int16)dev)
#define DMAwrite(sect, count, buf, dev) toscallWPLW(XBIOS_TRAP, 43, (int32)sect, (int16)count, (void *)buf, (int16)dev)
#define NVMaccess(op, start, count, buffer) toscallWWWP(XBIOS_TRAP, 46, (int16)op, (int16)start, (int16)count, (char *)buffer)
#define VsetMode(mode) toscallW(XBIOS_TRAP, 88, (int16)mode)
#define VgetMonitor() toscallV(XBIOS_TRAP, 89)
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
 * error mapping
 * in debug.c
 */

extern status_t toserror(int32 err);

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

#define COOKIE_JAR (*((const tos_cookie **)0x5A0))

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
 * XHDI
 */

/*
 * ARAnyM Native Features
 */

#define NF_COOKIE	0x5f5f4e46L	//'__NF'
#define NF_MAGIC	0x20021021L

#ifndef __ASSEMBLER__

typedef struct {
	long magic;
	long (*nfGetID) (const char *);
	long (*nfCall) (long ID, ...);
} NatFeatCookie;

extern NatFeatCookie *gNatFeatCookie;

static inline NatFeatCookie *nat_features(void)
{
	const struct tos_cookie *c;
	if (gNatFeatCookie == (void *)-1 || !gNatFeatCookie)
		return NULL;
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


/* XHDI NatFeat */

#define NF_XHDI "XHDI"

#define nfxhdi(code, a...) \
({ \
	gNatFeatCookie->nfCall((uint32)code, a##...); \
}) 


#define NFXHversion() nfxhdi(0)

#endif /* __ASSEMBLER__ */

#ifdef __cplusplus
}
#endif

#endif /* _TOSCALLS_H */
