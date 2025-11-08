/*	$NetBSD: msdosfs_vfsops.c,v 1.51 1997/11/17 15:36:58 ws Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (C) 1994, 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1994, 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */


// Modified to support the Haiku FAT driver.

#ifndef FS_SHELL
#include <fs_volume.h>
#include <syscalls.h>
#endif

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/mount.h"
#include "sys/vnode.h"

#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/msdosfsmount.h"


#ifdef USER
#define dprintf printf
#endif


#ifdef _KERNEL_MODE


static status_t
_remount_ro(void* voidVolume)
{
	struct mount* bsdVolume = (struct mount*)voidVolume;
	struct msdosfsmount* fatVolume = VFSTOMSDOSFS(bsdVolume);

	char mntpoint[B_PATH_NAME_LENGTH];
	strlcpy(mntpoint, bsdVolume->mnt_stat.f_mntonname, B_PATH_NAME_LENGTH);
	char device[B_DEV_NAME_LENGTH];
	strlcpy(device, fatVolume->pm_dev->si_device, B_DEV_NAME_LENGTH);
	char fs_name[B_FILE_NAME_LENGTH];
	strlcpy(fs_name, bsdVolume->mnt_fsvolume->file_system_name, B_FILE_NAME_LENGTH);

	status_t status = _kern_unmount(bsdVolume->mnt_stat.f_mntonname, B_FORCE_UNMOUNT);
	if (status == B_OK)
		status = _kern_mount(mntpoint, device, fs_name, B_MOUNT_READ_ONLY, "", 0);

	return status;
}
#endif // _KERNEL_MODE


void
msdosfs_integrity_error(struct msdosfsmount* fatVolume)
{
	// In fs_shell and userlandfs, there is no way to get the mount point, which would be needed
	// to unmount and remount.
#ifdef _KERNEL_MODE
	if (MOUNTED_READ_ONLY(fatVolume) == 0) {
		status_t status;
		thread_id thread;

		// alert the user
		panic("%s will be switched to read-only mode due to possible corruption.\n"
			"Install dosfstools and use fsck.fat to inspect it.\n",
			fatVolume->pm_dev->si_device);

		status = spawn_kernel_thread(_remount_ro, "FAT fault unmount", B_URGENT_PRIORITY,
			fatVolume->pm_mountp);
		if (status < B_OK) {
			dprintf("msdosfs_integrity_error:  failed to spawn thread (%s)\n", strerror(status));
			return;
		}
		thread = status;

		status = resume_thread(thread);
		if (status != B_OK)
			dprintf("msdosfs_integrity_error:  failed to resume thread (%s)\n", strerror(status));

		return;
	}
#endif // _KERNEL_MODE

	dprintf("Possible corruption on %s. Install dosfstools and use fsck.fat to inspect it.\n",
		fatVolume->pm_dev->si_device);

	return;
}
