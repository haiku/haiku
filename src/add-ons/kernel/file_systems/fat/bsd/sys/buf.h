/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef FAT_BUF_H
#define FAT_BUF_H


// Modified to support the Haiku FAT driver.

#include <sys/queue.h>

#ifdef FS_SHELL
#include "fssh_fs_cache.h"
#else
#include <fs_cache.h>
#endif // FS_SHELL

#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/msdosfsmount.h"
#include "sys/mount.h"
#include "sys/pcpu.h"
#include "sys/ucred.h"

#include "dosfs.h"


/*
 * The buffer header describes an I/O operation in the kernel.
 *
 * NOTES:
 *	b_bufsize, b_bcount. b_bufsize is the allocation size of the
 *	buffer, either DEV_BSIZE or PAGE_SIZE aligned. b_bcount is the
 *	originally requested buffer size and can serve as a bounds check
 *	against EOF. For most, but not all uses, b_bcount == b_bufsize.
 *
 *	b_resid. Number of bytes remaining in I/O. After an I/O operation
 *	completes, b_resid is usually 0 indicating 100% success.
 */
struct buf {
	long b_bcount;
	caddr_t b_data;
	long b_resid;
	daddr_t b_blkno; /* Underlying physical block number. */
	uint32_t b_flags; /* B_* flags. */
	long b_bufsize; /* Allocated buffer size. */
		// In the Haiku port, if b_data is a pointer to a block cache block,
		// this is still populated even though b_data is not owned by the driver.
	daddr_t b_lblkno; /* Logical block number. */
	struct vnode* b_vp; /* Device vnode. */

	// Members added for Haiku port
	bool b_owned;
		// false if b_data is a pointer to a block cache block
	struct vnode* b_vreg;
		// the node that was passed to bread, if it was of type VREG; otherwise, NULL
	void* b_bcpointers[128];
		// If b_data is assembled from multiple block cache blocks, this holds pointers to
		// each source block. The FAT specification allows up to 128 sectors per cluster.
	SLIST_ENTRY(buf) link;
		// for the list members of struct bufobj
};

// In the Haiku port, these FreeBSD flags are referenced but do not have any effect
#define B_DELWRI 0x00000080 /* Delay I/O until buffer reused. */
#define B_CLUSTEROK 0x00020000 /* Pagein op, so swap() can count it. */
#define B_INVALONERR 0x00040000 /* Invalidate on write error. */

// In FreeBSD, struct vn_clusterw stores the state of an optimized routine for writing multiple
// bufs. In the Haiku port, it is not used for anything meaningful.
struct vn_clusterw {
	daddr_t v_cstart; /* v start block of cluster */
	daddr_t v_lasta; /* v last allocation  */
	daddr_t v_lastw; /* v last write  */
	int v_clen; /* v length of cur. cluster */
};

// Zero out the data member of a struct buf.
// Port version is tolerant of a buf that holds less than the client-requested size in b_data.
#define clrbuf(bp) \
	{ \
		bzero((bp)->b_data, (u_int)(bp)->b_bufsize); \
		(bp)->b_resid = 0; \
	}


int bwrite(struct buf* bp);
int buf_dirty_count_severe(void);
int bread(struct vnode* vp, daddr_t blkno, int size, struct ucred* cred, struct buf** bpp);
void bdwrite(struct buf* bp);
void bawrite(struct buf* bp);
void brelse(struct buf* bp);
struct buf* getblk(struct vnode* vp, daddr_t blkno, int size, int slpflag, int slptimeo, int flags);


static inline void
cluster_init_vn(struct vn_clusterw* vnc)
{
	return;
}


void vfs_bio_bzero_buf(struct buf* bp, int base, int size);
void vfs_bio_clrbuf(struct buf* bp);


#endif // FAT_BUF_H
