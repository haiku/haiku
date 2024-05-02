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


// Modified to support the Haiku FAT driver.

#include "sys/param.h"
#include "sys/lockmgr.h"
#include "sys/mutex.h"
#include "sys/systm.h"


/*! @param flags LK_EXCLUSIVE or LK_SHARED by itself write- or read-locks the node, respectively.
	LK_RELEASE in combination with LK_EXCLUSIVE or LK_SHARED write- or read-unlocks the node,
	respectively.
	@param ilk Ignored in the Haiku port.
*/
void
lockmgr(struct lock* lk, u_int flags, struct mtx* ilk)
{
	if ((flags & LK_RELEASE) != 0) {
		if ((flags & LK_SHARED) != 0)
			rw_lock_read_unlock(&lk->haikuRW);
		else
			rw_lock_write_unlock(&lk->haikuRW);
	} else if ((flags & LK_EXCLUSIVE) != 0) {
		rw_lock_write_lock(&lk->haikuRW);
	} else if ((flags & LK_SHARED) != 0) {
		rw_lock_read_lock(&lk->haikuRW);
	} else {
		panic("unrecognized lockmgr flag\n");
	}

	return;
}
