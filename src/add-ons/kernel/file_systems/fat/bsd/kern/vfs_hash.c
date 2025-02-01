/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2005 Poul-Henning Kamp
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


// Modified to support the Haiku FAT driver.

#ifdef FS_SHELL
#include "sys/types.h"
#include "fssh_api_wrapper.h"
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/mount.h"
#include "sys/namei.h"
#include "sys/vnode.h"

#include "fs/msdosfs/denode.h"
#include "fs/msdosfs/direntry.h"

#include "dosfs.h"


#ifdef USER
#define dprintf printf
#endif


/*! Determine the inode number for a direntry location and return the corresponding node if it is
	constructed.
	@param hash A tentative location-based inode.
	@param arg Pointer to a pre-allocated uint64.
	@param td Ignored in the port.
	@param fn Ignored in the port. In FreeBSD, this is a function pointer that would be passed to
	the VFS and applied to the VFS's list of vnodes to find a match.
	@param arg Pointer to a pre-allocated uint64 that will be set to the final inode number.
	@post *arg is set to the inode number corresponding to hash (which may or may not equal hash).
	*vpp points to the corresponding BSD-style vnode, if that vnode already exists in memory. If
	the node doesn't exist in memory, *vpp is set to NULL.
	The function signature is modified to make hash a uint64 instead of uint32. This is consistent
	with the type passed by deget, and protects against overflow.
	This function never calls get_vnode when the node in question is not already registered with
	the VFS, so it is suitable for use within dosfs_read_vnode.
*/
int
vfs_hash_get(struct mount* mp, uint64 hash, int flags, thread_id td, struct vnode** vpp,
	vfs_hash_cmp_t* fn, void* arg)
{
	uint64* finalInode = arg;
	status_t status = B_OK;
	bool exists = false;
		// Has a node with the same inode number already been constructed?

	status = assign_inode(mp, (ino_t*)&hash);
	if (status != B_OK)
		return B_TO_POSIX_ERROR(status);

	exists = node_exists(mp, hash);
	if (exists == true) {
		// get_vnode will just return a ref (without calling dosfs_read_vnode)
		status = get_vnode(mp->mnt_fsvolume, hash, (void**)vpp);
		if (status != B_OK) {
			dprintf("FAT vfs_hash_get:  get_vnode failure\n");
			return B_TO_POSIX_ERROR(status);
		}
		rw_lock_write_lock(&(*vpp)->v_vnlock->haikuRW);
			// the driver expects the node to be returned locked
	} else {
		// The node needs to be initialized. Returning with a NULL vpp will
		// signal deget() to go through with setting it up.
		*vpp = NULL;
	}

	*finalInode = hash;

	return 0;
}


/*! In FreeBSD, this adds vp to a VFS-level list of nodes. In Haiku, the same thing is
	taken care of automatically by get_vnode.
*/
int
vfs_hash_insert(struct vnode* vp, uint64 hash, int flags, thread_id td, struct vnode** vpp,
	vfs_hash_cmp_t* fn, void* arg)
{
	ASSERT(node_exists(vp->v_mount, hash) == false);

	// This tells the client that there is no existing vnode with the same
	// inode number. This should always be the case under the conditions in which
	// the FAT driver calls this function:  if there was an existing node,
	// deget would have returned early.
	*vpp = NULL;

	return 0;
}


/*! In FreeBSD, this provides the VFS with an updated node hash value based on the new directory
	entry offset when a file is renamed. Not applicable in Haiku.
*/
void
vfs_hash_rehash(struct vnode* vp, uint64 hash)
{
	return;
}
