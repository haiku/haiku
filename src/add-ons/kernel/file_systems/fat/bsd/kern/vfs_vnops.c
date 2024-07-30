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
 * Copyright (c) 2012 Konstantin Belousov <kib@FreeBSD.org>
 * Copyright (c) 2013, 2014 The FreeBSD Foundation
 *
 * Portions of this software were developed by Konstantin Belousov
 * under sponsorship from the FreeBSD Foundation.
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


// Modified to support the Haiku FAT driver.

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/mount.h"
#include "sys/namei.h"
#include "sys/vnode.h"


struct deget_dotdot {
	u_long cluster;
	int blkoff;
};


/*! Return in rvp the private node that is specified by alloc_arg, using the function alloc.
	The FAT driver uses this to call deget when looking up ".." directory entries.
*/
int
vn_vget_ino_gen(struct vnode* vp, vn_get_ino_t alloc, void* alloc_arg, int lkflags,
	struct vnode** rvp)
{
	struct mount* bsdVolume = vp->v_mount;
	struct msdosfsmount* fatVolume = (struct msdosfsmount*)bsdVolume->mnt_data;
	struct deget_dotdot* dotdotLocation = alloc_arg;
	uint64 inode
		= (uint64)fatVolume->pm_bpcluster * dotdotLocation->cluster + dotdotLocation->blkoff;

	status_t status = assign_inode(bsdVolume, (ino_t*)&inode);
	ASSERT_ALWAYS(status == B_OK && node_exists(bsdVolume, inode));
		// It's important that this node has already been constructed.
		// If it hasn't, then deget will construct a private node without publishing it to the VFS.

	return alloc(bsdVolume, alloc_arg, lkflags, rvp);
}


/*! The driver uses this to free up memory if a low resource condition occurs after extending a
	regular file. In FreeBSD, it does this by flushing dirty struct bufs associated with vp.
	In the Haiku port, we remove all pages from this file's file cache.
*/
int
vn_fsync_buf(struct vnode* vp, int waitfor)
{
	status_t status = B_OK;

	if (waitfor == MNT_WAIT) {
		vp->v_sync = true;
		status = file_cache_disable(vp->v_cache);
		vp->v_sync = false;
	} else {
		status = file_cache_disable(vp->v_cache);
	}

	if (status == B_OK)
		file_cache_enable(vp->v_cache);

	return B_TO_POSIX_ERROR(status);
}
