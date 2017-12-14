/*	$OpenBSD: arc4random.c,v 1.54 2015/09/13 08:31:47 guenther Exp $	*/

/*
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 * Copyright (c) 2013, Markus Friedl <markus@openbsd.org>
 * Copyright (c) 2014, Theo de Raadt <deraadt@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * ChaCha based random number generator for OpenBSD.
 */

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#define KEYSTREAM_ONLY
#include "chacha_private.h"

#define minimum(a, b) ((a) < (b) ? (a) : (b))

#if defined(__GNUC__) || defined(_MSC_VER)
#define inline __inline
#else				/* __GNUC__ || _MSC_VER */
#define inline
#endif				/* !__GNUC__ && !_MSC_VER */

#define KEYSZ	32
#define IVSZ	8
#define BLOCKSZ	64
#define RSBUFSZ	(16*BLOCKSZ)

/* Marked MAP_INHERIT_ZERO, so zero'd out in fork children. */
static struct _rs {
	size_t		rs_have;	/* valid bytes at end of rs_buf */
	size_t		rs_count;	/* bytes till reseed */
} *rs;

/* Maybe be preserved in fork children, if _rs_allocate() decides. */
static struct _rsx {
	chacha_ctx	rs_chacha;	/* chacha context for random keystream */
	u_char		rs_buf[RSBUFSZ];	/* keystream blocks */
} *rsx;


/*
 * Stub functions for portability.
 */

#include <sys/mman.h>

#include <pthread.h>
#include <signal.h>

static pthread_mutex_t arc4random_mtx = PTHREAD_MUTEX_INITIALIZER;
#define _ARC4_LOCK()   pthread_mutex_lock(&arc4random_mtx)
#define _ARC4_UNLOCK() pthread_mutex_unlock(&arc4random_mtx)

#ifdef __GLIBC__
extern void *__dso_handle;
extern int __register_atfork(void (*)(void), void(*)(void), void (*)(void), void *);
#define _ARC4_ATFORK(f) __register_atfork(NULL, NULL, (f), __dso_handle)
#else
/*
 * Unfortunately, pthread_atfork() is broken on FreeBSD (at least 9 and 10) if
 * a program does not link to -lthr. Callbacks registered with pthread_atfork()
 * appear to fail silently. So, it is not always possible to detect a PID
 * wraparound.
 */
#define _ARC4_ATFORK(f) pthread_atfork(NULL, NULL, (f))
#endif

static inline void
_getentropy_fail(void)
{
	raise(SIGKILL);
}

static volatile sig_atomic_t _rs_forked;

static inline void
_rs_forkhandler(void)
{
	_rs_forked = 1;
}

static inline void
_rs_forkdetect(void)
{
	static pid_t _rs_pid = 0;
	pid_t pid = getpid();

	if (_rs_pid == 0 || _rs_pid != pid || _rs_forked) {
		_rs_pid = pid;
		_rs_forked = 0;
		if (rs)
			memset(rs, 0, sizeof(*rs));
	}
}

static inline int
_rs_allocate(struct _rs **rsp, struct _rsx **rsxp)
{
	if ((*rsp = mmap(NULL, sizeof(**rsp), PROT_READ|PROT_WRITE,
	    MAP_ANON|MAP_PRIVATE, -1, 0)) == MAP_FAILED)
		return (-1);

	if ((*rsxp = mmap(NULL, sizeof(**rsxp), PROT_READ|PROT_WRITE,
	    MAP_ANON|MAP_PRIVATE, -1, 0)) == MAP_FAILED) {
		munmap(*rsp, sizeof(**rsp));
		return (-1);
	}

	_ARC4_ATFORK(_rs_forkhandler);
	return (0);
}

static inline void _rs_rekey(u_char *dat, size_t datlen);

static inline void
_rs_init(u_char *buf, size_t n)
{
	if (n < KEYSZ + IVSZ)
		return;

	if (rs == NULL) {
		if (_rs_allocate(&rs, &rsx) == -1)
			abort();
	}

	chacha_keysetup(&rsx->rs_chacha, buf, KEYSZ * 8, 0);
	chacha_ivsetup(&rsx->rs_chacha, buf + KEYSZ);
}

static void
_rs_stir(void)
{
	u_char rnd[KEYSZ + IVSZ];

	if (getentropy(rnd, sizeof rnd) == -1)
		_getentropy_fail();

	if (!rs)
		_rs_init(rnd, sizeof(rnd));
	else
		_rs_rekey(rnd, sizeof(rnd));
	explicit_bzero(rnd, sizeof(rnd));	/* discard source seed */

	/* invalidate rs_buf */
	rs->rs_have = 0;
	memset(rsx->rs_buf, 0, sizeof(rsx->rs_buf));

	rs->rs_count = 1600000;
}

static inline void
_rs_stir_if_needed(size_t len)
{
	_rs_forkdetect();
	if (!rs || rs->rs_count <= len)
		_rs_stir();
	if (rs->rs_count <= len)
		rs->rs_count = 0;
	else
		rs->rs_count -= len;
}

static inline void
_rs_rekey(u_char *dat, size_t datlen)
{
#ifndef KEYSTREAM_ONLY
	memset(rsx->rs_buf, 0, sizeof(rsx->rs_buf));
#endif
	/* fill rs_buf with the keystream */
	chacha_encrypt_bytes(&rsx->rs_chacha, rsx->rs_buf,
	    rsx->rs_buf, sizeof(rsx->rs_buf));
	/* mix in optional user provided data */
	if (dat) {
		size_t i, m;

		m = minimum(datlen, KEYSZ + IVSZ);
		for (i = 0; i < m; i++)
			rsx->rs_buf[i] ^= dat[i];
	}
	/* immediately reinit for backtracking resistance */
	_rs_init(rsx->rs_buf, KEYSZ + IVSZ);
	memset(rsx->rs_buf, 0, KEYSZ + IVSZ);
	rs->rs_have = sizeof(rsx->rs_buf) - KEYSZ - IVSZ;
}

static inline void
_rs_random_buf(void *_buf, size_t n)
{
	u_char *buf = (u_char *)_buf;
	u_char *keystream;
	size_t m;

	_rs_stir_if_needed(n);
	while (n > 0) {
		if (rs->rs_have > 0) {
			m = minimum(n, rs->rs_have);
			keystream = rsx->rs_buf + sizeof(rsx->rs_buf)
			    - rs->rs_have;
			memcpy(buf, keystream, m);
			memset(keystream, 0, m);
			buf += m;
			n -= m;
			rs->rs_have -= m;
		}
		if (rs->rs_have == 0)
			_rs_rekey(NULL, 0);
	}
}

static inline void
_rs_random_u32(uint32_t *val)
{
	u_char *keystream;

	_rs_stir_if_needed(sizeof(*val));
	if (rs->rs_have < sizeof(*val))
		_rs_rekey(NULL, 0);
	keystream = rsx->rs_buf + sizeof(rsx->rs_buf) - rs->rs_have;
	memcpy(val, keystream, sizeof(*val));
	memset(keystream, 0, sizeof(*val));
	rs->rs_have -= sizeof(*val);
}

uint32_t
arc4random(void)
{
	uint32_t val;

	_ARC4_LOCK();
	_rs_random_u32(&val);
	_ARC4_UNLOCK();
	return val;
}

void
arc4random_buf(void *buf, size_t n)
{
	_ARC4_LOCK();
	_rs_random_buf(buf, n);
	_ARC4_UNLOCK();
}
