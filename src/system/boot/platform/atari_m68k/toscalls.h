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

/* 
 * Atari BIOS calls
 */

extern int32 bios(uint16 nr, ...);

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


//#define Getmpb() bios(0)
#define Bconstat(dev) bios(1, (uint16)dev)
#define Bconin(dev) bios(2, (uint16)dev)
#define Bconout(dev, chr) bios(3, (uint16)dev, (uint16)chr)
#define Rwabs(mode, buf, count, recno, dev, lrecno) bios(4, (int16)mode, (void *)buf, (int16)count, (int16)recno, (uint16)dev, (int32)lrecno)
//#define Setexc() bios(5, )
#define Tickcal() bios(6)
#define Getbpb(dev) (struct tosbpb *)bios(7, (uint16)dev)
#define Bcostat(dev) bios(8, (uint16)dev)
//#define Mediach() bios(9, )
#define Drvmap() (uint32)bios(10)
#define Kbshift(mode) bios(11, (uint16)mode)

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

/* 
 * Atari XBIOS calls
 */

extern int32 xbios(uint16 nr, ...);

#define IM_DISABLE	0
#define IM_RELATIVE	1
#define IM_ABSOLUTE	2
#define IM_KEYCODE	4

#define NVM_READ	0
#define NVM_WRITE	1
#define NVM_RESET	2

#define Initmous(mode, param, vec) xbios(0, (int16)mode, (void *)param, (void *)vec)
#define Physbase() (void *)xbios(2)
#define Logbase() (void *)xbios(3)
//#define Getrez() xbios(4)
#define Setscreen(log, phys, mode) xbios(5, (void *)log, (void *)phys, (int16)mode)
#define VsetScreen(log, phys, mode, modecode) xbios(5, (void *)log, (void *)phys, (int16)mode)
//#define Mfpint() xbios(13, )
#define Rsconf(speed, flow, ucr, rsr, tsr, scr) xbios(15, (int16)speed, (int16)flow, (int16)ucr, (int16)rsr, (int16)tsr, (int16)scr)
//#define Keytbl(unshift, shift, caps) (KEYTAB *)xbios(16, (char *)unshift, (char *)shift, (char *)caps)
#define Random() xbios(17)
#define Gettime() (uint32)xbios(23)
#define Jdisint(intno) xbios(26, (int16)intno)
#define Jenabint(intno) xbios(27, (int16)intno)
#define Supexec(func) xbios(38, (void *)func)
//#define Puntaes() xbios(39)
#define DMAread(sect, count, buf, dev) xbios(42, (int32)sect, (int16)count, (void *)buf, (int16)dev)
#define DMAwrite(sect, count, buf, dev) xbios(43, (int32)sect, (int16)count, (void *)buf, (int16)dev)
#define NVMaccess(op, start, count, buffer) xbios(46, (int16)op, (int16)start, (int16)count, (char *)buffer)
#define VsetMode(mode) xbios(88, (int16)mode)
#define VgetMonitor() xbios(89)
#define Locksnd() xbios(128)
#define Unlocksnd() xbios(129)

/* 
 * Atari GEMDOS calls
 */

extern int32 gemdos(uint16 nr, ...);

// official names
#define Pterm0() gemdos(0)
#define Cconin() gemdos(1)
#define Super(a) gemdos(0x20, (uint32)a)
#define Pterm(retcode) gemdos(76, (int16)retcode)

/*
 * error mapping
 * in debug.c
 */

extern status_t toserror(int32 err);

#endif /* __ASSEMBLER__ */

#ifdef __cplusplus
}
#endif

#endif /* _TOSCALLS_H */
