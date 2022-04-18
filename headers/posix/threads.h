/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2011 Ed Schouten <ed@FreeBSD.org>
 * Copyright (c) 2022 Dominic Martinez <dom@dominicm.dev>
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

#if __GNUC__ > 2 /* not available on gcc2 */

#ifndef _THREADS_H_
#define _THREADS_H_

#include <sys/types.h>
#include <time.h>

typedef pthread_t thrd_t;
typedef int (*thrd_start_t)(void *);

enum {
	thrd_busy = 1,
	thrd_error = 2,
	thrd_nomem = 3,
	thrd_success = 4,
	thrd_timedout = 5
};

#ifdef __cplusplus
extern "C" {
#endif

int	thrd_create(thrd_t *thread, thrd_start_t, void *);
thrd_t	thrd_current(void);
int	thrd_detach(thrd_t);
int	thrd_equal(thrd_t, thrd_t);
_Noreturn void
	thrd_exit(int);
int	thrd_join(thrd_t, int *);
int	thrd_sleep(const struct timespec *, struct timespec *);
void	thrd_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* _THREADS_H_ */

#endif /* __GNUC__ > 2 */
