/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1989, 1993
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


// Modified to support the Haiku FAT driver.

#ifdef FS_SHELL
#include "sys/types.h"
#include "fssh_api_wrapper.h"
#else
#include <disk_device_manager.h>
#include <real_time_clock.h>
#endif

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/malloc.h"
#include "sys/namei.h"
#include "sys/vnode.h"

#include "fs/msdosfs/denode.h"
#include "fs/msdosfs/direntry.h"


#ifdef USER
#define dprintf printf
#endif


/*! Output is in GMT, provided the user has set the timezone.
	In fat_shell, output is in local time.
*/
void
vfs_timestamp(struct timespec* tsp)
{
	// get local time
	bigtime_t usecs = real_time_clock_usecs();

	// convert to GMT
	int32 offset = 0;
#ifdef _KERNEL_MODE
	offset = (int32)get_timezone_offset();
#elif defined USER
	time_t the_time;
	struct tm the_tm;

	time(&the_time);
	localtime_r(&the_time, &the_tm);
	offset = (int32)the_tm.tm_gmtoff;
#endif

	tsp->tv_sec = usecs / 1000000;
	tsp->tv_sec -= offset;
	tsp->tv_nsec = (usecs - tsp->tv_sec * 1000000LL) * 1000LL;

	return;
}


/*! Allocate a struct vnode and initialize it to a generic state.

*/
int
getnewvnode(const char* tag, struct mount* mp, struct vop_vector* vops, struct vnode** vpp)
{
	struct vnode* newBsdNode = calloc(1, sizeof(struct vnode));
	if (newBsdNode == NULL)
		return ENOMEM;

	newBsdNode->v_type = VNON;
	newBsdNode->v_state = VSTATE_UNINITIALIZED;
	newBsdNode->v_data = NULL;
	newBsdNode->v_mount = mp;
	newBsdNode->v_rdev = NULL;
	newBsdNode->v_vflag = 0;
	newBsdNode->v_vnlock = &newBsdNode->v_lock;
	rw_lock_init(&newBsdNode->v_vnlock->haikuRW, "fat vnode");
	newBsdNode->v_bufobj.bo_flag = 0;
	newBsdNode->v_parent = 0;
	newBsdNode->v_mime = NULL;
	newBsdNode->v_cache = NULL;
	newBsdNode->v_file_map = NULL;
	newBsdNode->v_resizing = false;
	newBsdNode->v_sync = false;

	*vpp = newBsdNode;

	return 0;
}


/*! Used by detrunc to update the associated cache. If vp is a regular file, the hook must
	update file cache size separately.
*/
int
vtruncbuf(struct vnode* vp, off_t length, int blksize)
{
	status_t status = B_OK;

	if (vp->v_type == VREG) {
		// calling file_cache_set_size here might cause a deadlock because the node is locked
		// at this point
		file_map_set_size(vp->v_file_map, length);
		return 0;
	}
	if (vp->v_type == VDIR) {
		status = discard_clusters(vp, length);
		return B_TO_POSIX_ERROR(status);
	}

	return B_ERROR;
}


/*! Get a ref to vp and optionally write-lock it.

*/
int
vget(struct vnode* vp, int flags)
{
	status_t status = B_OK;

	if ((flags & LK_EXCLUSIVE) != 0) {
		status = rw_lock_write_lock(&vp->v_vnlock->haikuRW);
		if (status != B_OK)
			return B_TO_POSIX_ERROR(status);
	} else if (flags != 0) {
		return EINVAL;
	}

	status = acquire_vnode(vp->v_mount->mnt_fsvolume, VTODE(vp)->de_inode);

	return B_TO_POSIX_ERROR(status);
}
