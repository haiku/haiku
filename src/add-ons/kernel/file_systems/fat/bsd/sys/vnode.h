/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
#ifndef FAT_VNODE_H
#define FAT_VNODE_H


// Modified to support the Haiku FAT driver.

#ifdef FS_SHELL
#include "fssh_api_wrapper.h"
#else
#include <fs_interface.h>
#include <lock.h>
#endif

#include "sys/buf.h"
#include "sys/bufobj.h"
#include "sys/lockmgr.h"
#include "sys/namei.h"
#include "sys/ucred.h"

#include "fs/msdosfs/direntry.h"
#include "fs/msdosfs/denode.h"

#include "dosfs.h"


// In the Haiku port, one struct vnode per volume is set up to represent its device file.
// It will have type VBLK. All other struct vnodes (those that actually represent
// files on the volume) will have type VREG or VDIR.
enum vtype { VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD, VMARKER };
#define VLASTTYPE VMARKER

enum vstate { VSTATE_UNINITIALIZED, VSTATE_CONSTRUCTED, VSTATE_DESTROYING, VSTATE_DEAD };
#define VLASTSTATE VSTATE_DEAD

/*
 * Reading or writing any of these items requires holding the appropriate lock.
 *
 * Lock reference:
 *	u - Only a reference to the vnode is needed to read.
 *	v - vnode lock
 *
 */
struct vnode {
	/*
	 * Fields which define the identity of the vnode.
	 */
	enum vtype v_type : 8; /* u vnode type */
	enum vstate v_state : 8; /* u vnode state */
	void* v_data; /* u private data for fs */

	/*
	 * Filesystem instance stuff
	 */
	struct mount* v_mount; /* u ptr to vfs we are in */

	/*
	 * Type specific fields, only one applies to any given vnode.
	 */
	union {
		struct cdev* v_rdev; /* v device (VCHR, VBLK) */
	};

	/*
	 * Locking
	 */
	struct lock v_lock; /* u (if fs don't have one) */
	struct lock* v_vnlock; /* u pointer to vnode lock */

	/*
	 * The machinery of being a vnode
	 */
	struct bufobj v_bufobj; /* * Buffer cache object */

	/*
	 * Hooks for various subsystems and features.
	 */
	u_short v_vflag; /* v vnode flags */

	// Members added for Haiku port
	ino_t v_parent; /* v inode of parent directory */
	const char* v_mime; /* v mime type for VREG nodes, otherwise NULL */
	void* v_cache; /* v file cache for VREG nodes, otherwise NULL */
	void* v_file_map; /* v file map for VREG nodes, otherwise NULL */
	bool v_resizing; /* v disable IO (see comment in dosfs_wstat) */
	bool v_sync; /* v use synchronous IO */
};


/*
 * Vnode flags.
 *	VV flags are protected by the vnode lock and live in v_vflag
 */
#define VV_ROOT 0x0001 /* root of its filesystem */

/*
 * Flags for ioflag.
 */
#define IO_SYNC 0x0080 /* do I/O synchronously */

/*
 * Flags for accmode_t.
 */
#define VEXEC 000000000100 /* execute/search permission */
#define VWRITE 000000000200 /* write permission */
#define VREAD 000000000400 /* read permission */

#define VREF(vp) vref(vp)

#define VN_LOCK_AREC(vp) lockallowrecurse((vp)->v_vnlock)


/*! Verify vp is exclusively (i.e. write) locked.

*/
static inline void
assert_vop_elocked(struct vnode* vp, const char* str)
{
	if (vp == NULL)
		return;
	ASSERT_WRITE_LOCKED_RW_LOCK(&vp->v_vnlock->haikuRW);
}


#define ASSERT_VOP_ELOCKED(vp, str) assert_vop_elocked((vp), (str))
// In FreeBSD, this verifies that vp is either write-locked or read-locked. In Haiku, there is no
// equivalent assert.
#define ASSERT_VOP_LOCKED(vp, str) ((void)0)

#define DOINGASYNC(vp) (((vp)->v_mount->mnt_flag & MNT_ASYNC) != 0)

// FreeBSD would generate vnode_if.h, vnode_if_typedef.h, and vnode_if_newproto.h at build time
// and #include them here (the latter two indirectly via the first).
// For the Haiku port, vnode_if.h was generated manually by running a script:
// sys/tools/vnode_if.awk sys/kern/vnode_if.src -h
// and vnode_if_typedef.h by running:
// sys/tools/vnode_if.awk sys/kern/vnode_if.src -q
// and vnode_if_newproto.h by running:
// sys/tools/vnode_if.awk sys/kern/vnode_if.src -p
// Modified excerpts from these headers are pasted below.

// This struct is referenced by the ported code but has no effect in the Haiku port.
struct vop_vector {
};


static inline int
VOP_ACCESS(struct vnode* vp, accmode_t accmode, struct ucred* cred, thread_id td)
{
	struct mount* bsdVolume = (struct mount*)vp->v_mount;

	int mode = 0;
	if ((accmode & VREAD) != 0)
		mode |= R_OK;
	if ((accmode & VWRITE) != 0)
		mode |= W_OK;
	if ((accmode & VEXEC) != 0)
		mode |= X_OK;

	return B_TO_POSIX_ERROR(_dosfs_access(bsdVolume, vp, mode));
}


static inline int
VOP_UNLOCK(struct vnode* vp)
{
	lockmgr(vp->v_vnlock, LK_RELEASE, NULL);

	return 0;
}


// End of vnode_if.h content

typedef int (*vn_get_ino_t)(struct mount*, void*, int, struct vnode**);

typedef int vfs_hash_cmp_t(struct vnode* vp, void* arg);

int vfs_hash_get(struct mount* mp, uint64 hash, int flags, thread_id td, struct vnode** vpp,
	vfs_hash_cmp_t* fn, void* arg);

int vfs_hash_insert(struct vnode* vp, uint64 hash, int flags, thread_id td, struct vnode** vpp,
	vfs_hash_cmp_t* fn, void* arg);
void vfs_hash_rehash(struct vnode* vp, uint64 hash);
void vfs_hash_remove(struct vnode* vp);

struct componentname;

void cache_enter(struct vnode* dvp, struct vnode* vp, struct componentname* cnp);

int getnewvnode(const char* tag, struct mount* mp, struct vop_vector* vops, struct vnode** vpp);


/*! In FreeBSD, this tells the VFS to add vp to its list of mp's nodes.
	In Haiku, the equivalent actions are done automatically as part of get_vnode / publish_vnode.
*/
static inline int
insmntque(struct vnode* vp, struct mount* mp)
{
	return 0;
}


/*! In FreeBSD, this generates a serial number based on system time. The driver uses it to
	initialize denode::de_modrev. However, in the Haiku port, de_modrev is ignored.
*/
static inline u_quad_t
init_va_filerev(void)
{
	return 0;
}


/*! Put a ref to vp and write-unlock it.

*/
static inline void
vput(struct vnode* vp)
{
	put_vnode(vp->v_mount->mnt_fsvolume, VTODE(vp)->de_inode);

	rw_lock_write_unlock(&vp->v_vnlock->haikuRW);
}


/*! Put a ref to vp.

*/
static inline void
vrele(struct vnode* vp)
{
	put_vnode(vp->v_mount->mnt_fsvolume, VTODE(vp)->de_inode);
}


/*! Get a ref to vp.

*/
static inline void
vref(struct vnode* vp)
{
	acquire_vnode(vp->v_mount->mnt_fsvolume, VTODE(vp)->de_inode);
}


/*! Get the node ref count from the VFS. Not implemented in Haiku port (function is used in
	driver but only for debug output).
*/
static __inline int
vrefcnt(struct vnode* vp)
{
	return -1;
}


int vget(struct vnode* vp, int flags);


/*! Prepare the VFS to discard vp.

*/
static inline void
vgone(struct vnode* vp)
{
	remove_vnode(vp->v_mount->mnt_fsvolume, VTODE(vp)->de_inode);
}


int vtruncbuf(struct vnode* vp, off_t length, int blksize);
int vn_fsync_buf(struct vnode* vp, int waitfor);

int vn_vget_ino_gen(struct vnode* vp, vn_get_ino_t alloc, void* alloc_arg, int lkflags,
	struct vnode** rvp);
void vfs_timestamp(struct timespec* tsp);


static inline void
vn_set_state(struct vnode* vp, enum vstate state)
{
	vp->v_state = state;
}


#endif // FAT_VNODE_H
