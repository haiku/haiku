/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2004 Poul-Henning Kamp
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
 */
#ifndef FAT_BUFOBJ_H
#define FAT_BUFOBJ_H


// Modified to support the Haiku FAT driver.

#include <sys/queue.h>

#include "sys/_rwlock.h"


SLIST_HEAD(buf_list, buf);

// For the device vnode, v_bufobj holds lists of struct bufs that are available for use
// by the functions defined in vfs_bio.c. For all other nodes, v_bufobj is not used.
struct bufobj {
	struct rwlock bo_lock;
	u_int bo_flag;

	// Members added for Haiku port
	struct buf_list bo_clusterbufs;
		// an SLIST of buffers with b_bufsize equal to cluster size
	uint32 bo_clusters;
		// count of bufs in bo_clusterbufs
	struct buf_list bo_fatbufs;
		// SLIST of buffers of the size used to read the FAT. Currently, FAT16 and FAT32 volumes
		// read the FAT in 512-byte blocks, and this is only useful for FAT12, which reads it in
		// 1536-byte blocks.
	uint32 bo_fatblocks;
	struct buf_list bo_emptybufs;
		// SLIST of buffers with no space allocated for b_data (b_data must be set to point to a
		// block in the block cache).
	uint32 bo_empties;
};

#define BO_LOCK(bo) rw_lock_write_lock(BO_LOCKPTR(bo))
#define BO_UNLOCK(bo) rw_lock_write_unlock(BO_LOCKPTR(bo))


#endif // FAT_BUFOBJ_H
