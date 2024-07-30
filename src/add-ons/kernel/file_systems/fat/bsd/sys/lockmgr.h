/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2008 Attilio Rao <attilio@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
#ifndef FAT_LOCKMGR_H
#define FAT_LOCKMGR_H


// Modified to support the Haiku FAT driver.

#ifndef FS_SHELL
#include <lock.h>
#endif

#include "sys/_mutex.h"
#include "sys/_rwlock.h"


// Flag for lockinit() that is used in the driver, but has no effect in the Haiku port.
#define LK_NOWITNESS 0x000010

// lockmgr() attribute flag. Has no effect in the Haiku port.
#define LK_NOWAIT 0x000200

// lockmgr() operation flags
#define LK_TYPE_MASK 0xFF0000
#define LK_EXCLUSIVE 0x080000
#define LK_RELEASE 0x100000
#define LK_SHARED 0x200000

// lockmgr_assert flag
#define KA_XLOCKED 0x00000004


struct lock {
	u_int flags;
	rw_lock haikuRW;
};

void lockmgr(struct lock* lk, u_int flags, struct mtx* ilk);


/*! KA_XLOCKED is the only flag used by the FAT driver.

*/
static inline void
lockmgr_assert(const struct lock* lk, int what)
{
#ifndef FS_SHELL
	if (what == KA_XLOCKED) {
		ASSERT_WRITE_LOCKED_RW_LOCK(&lk->haikuRW);
	} else {
		panic("lockmgr_assert: unrecognized flag\n");
	}
#endif // !FS_SHELL

	return;
}


/*! Used to enable recursive locking. In the Haiku port, struct lock is implemented such that
	recursive locking is always enabled.
*/
static inline void
lockallowrecurse(struct lock* lk)
{
	return;
}


static inline void
lockdestroy(struct lock* lk)
{
	rw_lock_destroy(&lk->haikuRW);
}


#endif // FAT_LOCKMGR_H
