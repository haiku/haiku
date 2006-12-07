/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef SPAWN_H
#define SPAWN_H

#define MAXPTTYS 16 * 4

#ifndef CEOF
#define CEOF ('D'&037)
#endif
#ifndef CSUSP
#define CSUSP ('@'&037)
#endif
#ifndef CQUIT
#define CQUIT ('\\'&037)
#endif
#ifndef CEOL
#define CEOL 0
#endif
#ifndef CSTOP
#define CSTOP ('Q'&037)
#endif
#ifndef CSTART
#define CSTART ('S'&037)
#endif
#ifndef CSWTCH
#define CSWTCH 0
#endif

/*
 * ANSI emulation.
 */
#define INQ	0x05
#define	FF	0x0C			/* C0, C1 control names		*/
#define	LS1	0x0E
#define	LS0	0x0F
#define	CAN	0x18
#define	SUB	0x1A
#define	ESC	0x1B
#define US	0x1F
#define	DEL	0x7F
#define HTS     ('H'+0x40)
#define	SS2	0x8E
#define	SS3	0x8F
#define	DCS	0x90
#define	OLDID	0x9A			/* ESC Z			*/
#define	CSI	0x9B
#define	ST	0x9C
#define	OSC	0x9D
#define	PM	0x9E
#define	APC	0x9F
#define	RDEL	0xFF

/*
 *  Prototype:
 */

int spawn_shell (int, int, const char *, const char *);
void Setenv (const char *, const char *);

extern pid_t sh_pid;	/* shell process ID */
extern int pfd_num;	/* number of pfd */


#endif /* SPAWN_H */
