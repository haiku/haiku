/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1989, 1991, 1993
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
#ifndef FAT_MOUNT_H
#define FAT_MOUNT_H


// Modified to support the Haiku FAT driver.

#include <sys/queue.h>

#ifdef FS_SHELL
#include "fssh_api_wrapper.h"
#else
#include <fs_interface.h>
#endif // !FS_SHELL

#include "sys/_mutex.h"
#include "sys/ucred.h"


/*
 * filesystem statistics
 */
struct statfs {
	uint64_t f_iosize; /* optimal transfer block size */
	char f_mntonname[B_PATH_NAME_LENGTH]; /* directory on which mounted */
};

struct mount {
	int mnt_kern_flag; /* kernel only flags */
	uint64_t mnt_flag; /* flags shared with user */
	struct vfsconf* mnt_vfc; /* configuration info */
	struct mtx mnt_mtx;
	struct statfs mnt_stat; /* cache of filesystem stats */
		// In the Haiku port, mnt_stat.f_mntonname is only initialized in the kernel addon
		// (not in fat_shell or fat_userland)
	void* mnt_data; /* private data */
	int mnt_iosize_max; /* max size for clusters, etc */

	// Members added for Haiku port
	struct fs_volume* mnt_fsvolume;
	void* mnt_cache;
		// Haiku block cache
	int32 mnt_volentry;
		// index of volume label entry in root directory, or -1 if no such entry was found
	struct {
		rw_lock lock;
		ino_t cur_vnid;
		uint32 cache_size;
		struct vcache_entry** by_vnid;
		struct vcache_entry** by_loc;
	} vcache;
		// hash tables storing the default (location-based) and final inode numbers for each node
};

/*
 * User specifiable flags, stored in mnt_flag.
 */
#define MNT_RDONLY 0x0000000000000001ULL /* read only filesystem */
#define MNT_SYNCHRONOUS 0x0000000000000002ULL /* fs written synchronously */
#define MNT_ASYNC 0x0000000000000040ULL /* fs written asynchronously */
#define MNT_NOCLUSTERW 0x0000000080000000ULL /* disable cluster write */

/*
 * Flags for various system call interfaces.
 */
#define MNT_WAIT 1 /* synchronously wait for I/O to complete */

// For the Haiku port, vfc_typenum will be arbitrarily set to 1.
struct vfsconf {
	int vfc_typenum; /* historic filesystem type number */
};


#endif // FAT_MOUNT_H
