/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 1999,2000,2001 Jonathan Lemon <jlemon@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */
#ifndef _BSD_SYS_EVENT_H_
#define _BSD_SYS_EVENT_H_

#include <features.h>

#ifdef _DEFAULT_SOURCE

#include <stdint.h>


#define EVFILT_READ			(-1)
#define EVFILT_WRITE		(-2)
#define EVFILT_PROC			(-5)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define	EV_SET(kevp_, a, b, c, d, e, f) do {	\
	*(kevp_) = (struct kevent){		\
		.ident = (a),			\
		.filter = (b),			\
		.flags = (c),			\
		.fflags = (d),			\
		.data = (e),			\
		.udata = (f),			\
		.ext = {0},				\
	};					\
} while (0)
#else /* Pre-C99 or not STDC (e.g., C++) */
/*
 * The definition of the local variable kevp could possibly conflict
 * with a user-defined value passed in parameters a-f.
 */
#define EV_SET(kevp_, a, b, c, d, e, f) do {	\
	struct kevent *kevp = (kevp_);		\
	(kevp)->ident = (a);			\
	(kevp)->filter = (b);			\
	(kevp)->flags = (c);			\
	(kevp)->fflags = (d);			\
	(kevp)->data = (e);			\
	(kevp)->udata = (f);			\
	(kevp)->ext[0] = 0;			\
	(kevp)->ext[1] = 0;			\
	(kevp)->ext[2] = 0;			\
	(kevp)->ext[3] = 0;			\
} while (0)
#endif

struct kevent {
	uintptr_t	ident;		/* identifier for this event */
	short		filter;		/* filter for event */
	unsigned short	flags;		/* action flags for kqueue */
	unsigned int	fflags;		/* filter flag value */
	int64_t	data;		/* filter data value */
	void		*udata;		/* opaque user data identifier */
	uint64_t	ext[4];		/* extensions */
};

/* actions */
#define EV_ADD		0x0001		/* add event to kq (implies enable) */
#define EV_DELETE	0x0002		/* delete event from kq */

/* flags */
#define EV_ONESHOT	0x0010		/* only report one occurrence */
#define EV_CLEAR	0x0020		/* clear event state after reporting */

/* returned values */
#define EV_EOF		0x8000		/* EOF detected */
#define EV_ERROR	0x4000		/* error, data contains errno */

/* data/hint flags for EVFILT_PROC */
#define	NOTE_EXIT	0x80000000		/* process exited */


#ifdef __cplusplus
extern "C" {
#endif

int     kqueue(void);
int     kevent(int kq, const struct kevent *changelist, int nchanges,
			struct kevent *eventlist, int nevents,
			const struct timespec *timeout);

#ifdef __cplusplus
}
#endif


#endif	/* _DEFAULT_SOURCE */

#endif	/* _BSD_SYS_EVENT_H_ */
