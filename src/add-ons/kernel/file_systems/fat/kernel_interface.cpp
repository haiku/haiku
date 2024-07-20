/*
 * Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
 * Copyright 2001-2020, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */

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


#ifdef FS_SHELL
#include "fssh_api_wrapper.h"
#else // !FS_SHELL
#include <dirent.h>
#include <malloc.h>
#include <new>
#include <stdlib.h>
#endif // !FS_SHELL

#ifndef FS_SHELL
#include <NodeMonitor.h>
#include <OS.h>
#include <TypeConstants.h>
#include <driver_settings.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <fs_volume.h>
#include <io_requests.h>
#endif // !FS_SHELL

#if defined USER && __GNUC__ == 2
// required for fs_ops_support.h
#define alignof(type) __alignof__(type)
#endif // USER && __GNUC__ == 2
#include <fs_ops_support.h>
#ifdef FS_SHELL
#include "fssh_auto_deleter.h"
#include "syscalls.h"
#else // !FS_SHELL
#include <AutoDeleter.h>
#include <arch_vm.h>
#include <kernel.h>
#include <syscalls.h>
#include <util/AutoLock.h>
#include <vfs.h>
#endif // !FS_SHELL

// FreeBSD flag that turns on full implementation of ported code
#define _KERNEL

extern "C"
{
#include "sys/param.h"
#include "sys/buf.h"
#include "sys/clock.h"
#include "sys/conf.h"
#include "sys/iconv.h"
#include "sys/mount.h"
#include "sys/mutex.h"
#include "sys/namei.h"
#include "sys/vnode.h"

#include "fs/msdosfs/bootsect.h"
#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/denode.h"
#include "fs/msdosfs/direntry.h"
#include "fs/msdosfs/fat.h"
#include "fs/msdosfs/msdosfsmount.h"
}

#include "debug.h"
#include "dosfs.h"
#ifdef FS_SHELL
#include "fssh_defines.h"
#endif // FS_SHELL
#include "mkdos.h"
#include "support.h"
#include "vcache.h"


static status_t iterative_io_get_vecs_hook(void* cookie, io_request* request, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count);
static status_t iterative_io_finished_hook(void* cookie, io_request* request, status_t status,
	bool partialTransfer, size_t bytesTransferred);

static status_t _dosfs_sync(mount* volume, bool data = true);
static status_t _dosfs_fsync(vnode* bsdNode);
static status_t _dosfs_read_vnode(mount* bsdVolume, const ino_t id, vnode** newNode, bool createFileCache = true);

static status_t bsd_volume_init(fs_volume* fsVolume, const uint32 flags, mount** volume);
status_t bsd_volume_uninit(mount* volume);
static status_t bsd_device_init(mount* bsdVolume, const dev_t devID, const char* deviceFile,
	cdev** bsdDevice, bool* _readOnly);
status_t bsd_device_uninit(cdev* device);
static status_t dev_bsd_node_init(cdev* bsdDevice, vnode** devNode);
status_t dev_bsd_node_uninit(vnode* devNode);
static status_t fat_volume_init(vnode* devvp, mount* bsdVolume, const uint64_t fatFlags,
	const char* oemPref);
status_t fat_volume_uninit(msdosfsmount* volume);


typedef struct IdentifyCookie {
	uint32 fBytesPerSector;
	uint32 fTotalSectors;
	char fName[12];
} IdentifyCookie;

typedef struct FileCookie {
	uint32 fMode; // open mode
	u_long fLastSize; // file size at last notify_stat_changed call
	u_short fMtimeAtOpen; // inital modification time
	u_short fMdateAtOpen; // initial modification date
	bigtime_t fLastNotification; // time of last notify_stat_changed call
} FileCookie;

typedef struct DirCookie {
	uint32 fIndex; // read this entry next
} DirCookie;

typedef struct AttrCookie {
	uint32 fMode; // open mode
	int32 fType; // attribute type
#define FAT_ATTR_MIME 0x1234
} AttrCookie;


typedef CObjectDeleter<mount, status_t, &bsd_volume_uninit> StructMountDeleter;
typedef CObjectDeleter<cdev, status_t, &bsd_device_uninit> StructCdevDeleter;
typedef CObjectDeleter<vnode, status_t, &dev_bsd_node_uninit> DevVnodeDeleter;
typedef CObjectDeleter<msdosfsmount, status_t, &fat_volume_uninit> StructMsdosfsmountDeleter;


struct iconv_functions* msdosfs_iconv;


static status_t
dosfs_mount(fs_volume* volume, const char* device, uint32 flags, const char* args,
	ino_t* _rootVnodeID)
{
#ifdef FS_SHELL
	FUNCTION_START("device %" B_PRIdDEV "\n", volume->id);
#else
	FUNCTION_START("device %" B_PRIdDEV ", partition %" B_PRId32 "\n", volume->id,
		volume->partition);
#endif

	status_t status = B_OK;

	int opSyncMode = 0;
	char oemPref[11] = "";
	void* handle = load_driver_settings("dos");
	if (handle != NULL) {
		opSyncMode = strtoul(get_driver_parameter(handle, "op_sync_mode", "0", "0"), NULL, 0);
		if (opSyncMode < 0 || opSyncMode > 2)
			opSyncMode = 0;

		strlcpy(oemPref, get_driver_parameter(handle, "OEM_code_page", "", ""), 11);

		unload_driver_settings(handle);
	}

	uint64 fatFlags = 0;
	// libiconv support is implemented only for the userlandfs module
#ifdef USER
	fatFlags |= MSDOSFSMNT_KICONV;
	if (strcmp(oemPref, "") == 0)
		strlcpy(oemPref, "CP1252", 11);
#endif // USER

	// args is a command line option; dosfs doesn't use any so we can ignore it

	bool readOnly = (flags & B_MOUNT_READ_ONLY) != 0;
	if ((flags & ~B_MOUNT_READ_ONLY) != 0) {
		INFORM("unsupported mount flag(s) %" B_PRIx32 "\n", (flags & ~B_MOUNT_READ_ONLY));
		return B_UNSUPPORTED;
	}

	// Initialize the struct mount, which is an adapted FreeBSD VFS object. It is present in the
	// port because the ported BSD code relies on it.
	mount* bsdVolume;
	status = bsd_volume_init(volume, flags, &bsdVolume);
	if (status != B_OK)
		RETURN_ERROR(status);
	StructMountDeleter bsdVolumeDeleter(bsdVolume);

	// initialize a BSD-style device struct
	cdev* bsdDevice = NULL;
	status = bsd_device_init(bsdVolume, volume->id, device, &bsdDevice, &readOnly);
	if (status != B_OK)
		RETURN_ERROR(status);
	StructCdevDeleter bsdDeviceDeleter(bsdDevice);

	if (readOnly == true) {
		bsdVolume->mnt_flag |= MNT_RDONLY;
		fatFlags |= MSDOSFSMNT_RONLY;
	}

	// A shell/FUSE host system might not call dosfs_sync automatically at shutdown/reboot if the
	// user forgets to unmount a volume, so we always use op sync mode for those targets.
#ifdef FS_SHELL
	opSyncMode = 2;
#endif // FS_SHELL

	// see if we need to go into op sync mode
	switch (opSyncMode) {
		case 1:
			if (bsdDevice->si_geometry->removable == false) {
				// we're not removable, so skip op_sync
				break;
			}
			// supposed to fall through

		case 2:
			PRINT("mounted with op sync enabled\n");
			bsdVolume->mnt_flag |= MNT_SYNCHRONOUS;
			fatFlags |= MSDOSFSMNT_WAITONFAT;
			break;

		case 0:
		default:
			bsdVolume->mnt_flag |= MNT_ASYNC;
			break;
	}

	// The driver needs access to a BSD-format vnode representing the device file, which in BSD
	// would be a vnode on another volume. We manually generate a stand-in.
	vnode* devNode;
	status = dev_bsd_node_init(bsdDevice, &devNode);
	if (status != B_OK)
		RETURN_ERROR(status);
	DevVnodeDeleter devVnodeDeleter(devNode);

	// initialize the FAT private volume data
	status = fat_volume_init(devNode, bsdVolume, fatFlags, oemPref);
	if (status != B_OK)
		RETURN_ERROR(status);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	StructMsdosfsmountDeleter fatVolumeDeleter(fatVolume);

	// create caches of struct bufs for the driver to use in bread()
	rw_lock_write_lock(&devNode->v_bufobj.bo_lock.haikuRW);
	for (uint32 i = 0; i < BUF_CACHE_SIZE; ++i) {
		status = slist_insert_buf(devNode, fatVolume->pm_bpcluster);
		if (status != B_OK)
			RETURN_ERROR(status);
		status = slist_insert_buf(devNode, fatVolume->pm_fatblocksize);
		if (status != B_OK)
			RETURN_ERROR(status);
		status = slist_insert_buf(devNode, 0);
		if (status != B_OK)
			RETURN_ERROR(status);
	}
	rw_lock_write_unlock(&devNode->v_bufobj.bo_lock.haikuRW);

	volume->private_volume = bsdVolume;
	volume->ops = &gFATVolumeOps;

	// publish root vnode

	u_long dirClust = FAT32(fatVolume) == true ? fatVolume->pm_rootdirblk : MSDOSFSROOT;
	u_long dirOffset = MSDOSFSROOT_OFS;
	ino_t rootInode = DETOI(fatVolume, dirClust, dirOffset);

	status = add_to_vcache(bsdVolume, rootInode, rootInode);
	if (status != B_OK)
		RETURN_ERROR(status);

	vnode* bsdRootNode;
	status = _dosfs_read_vnode(bsdVolume, rootInode, &bsdRootNode);
	if (status != B_OK)
		RETURN_ERROR(status);
	denode* fatRootNode = reinterpret_cast<denode*>(bsdRootNode->v_data);
	ASSERT(fatRootNode->de_dirclust == dirClust && fatRootNode->de_diroffset == dirOffset);

	status = publish_vnode(volume, rootInode, bsdRootNode, &gFATVnodeOps, S_IFDIR, 0);
	if (status != B_OK)
		RETURN_ERROR(status);

	PRINT("root vnode id = %" B_PRIdINO ", @ %p\n", fatRootNode->de_inode, bsdRootNode);

	*_rootVnodeID = fatRootNode->de_inode;

#ifdef _KERNEL_MODE
	// initialize mnt_stat.f_mntonname, for use by msdosfs_integrity_error
	dev_t mountpt;
	ino_t mountino;
	vfs_get_mount_point(fatVolume->pm_dev->si_id, &mountpt, &mountino);
	vfs_entry_ref_to_path(mountpt, mountino, NULL, true, bsdVolume->mnt_stat.f_mntonname,
		B_PATH_NAME_LENGTH);
#endif // _KERNEL_MODE

	bsdVolumeDeleter.Detach();
	bsdDeviceDeleter.Detach();
	devVnodeDeleter.Detach();
	fatVolumeDeleter.Detach();

	return B_OK;
}


static float
dosfs_identify_partition(int fd, partition_data* partition, void** _cookie)
{
	FUNCTION_START("dosfs_identify_partition\n");

	// read in the boot sector
	uint8 buf[512];
	if (read_pos(fd, 0, buf, 512) != 512)
		return -1;

	FatType type;
	bool dos33;
	status_t status = check_bootsector(buf, type, dos33);
	if (status != B_OK)
		return status;

	// partially set up a msdosfsmount, enough to read the volume label from the root directory
	msdosfsmount dummyVolume;
	dummyVolume.pm_mountp = NULL;
	switch (type) {
		case fat12:
			dummyVolume.pm_fatmask = FAT12_MASK;
			break;
		case fat16:
			dummyVolume.pm_fatmask = FAT16_MASK;
			break;
		case fat32:
			dummyVolume.pm_fatmask = FAT32_MASK;
			break;
		default:
			return -1;
	}
	status = parse_bpb(&dummyVolume, reinterpret_cast<union bootsector*>(buf), dos33);
	if (status != B_OK)
		return status;
	dummyVolume.pm_BlkPerSec = dummyVolume.pm_BytesPerSec / DEV_BSIZE;
	dummyVolume.pm_rootdirsize = howmany(dummyVolume.pm_RootDirEnts * sizeof(direntry), DEV_BSIZE);
		// Will be 0 for a FAT32 volume.
	dummyVolume.pm_bpcluster
		= dummyVolume.pm_bpb.bpbSecPerClust * dummyVolume.pm_BlkPerSec * DEV_BSIZE;
	dummyVolume.pm_bnshift = ffs(DEV_BSIZE) - 1;
	// for FAT12/16, parse_bpb doesn't initialize pm_rootdirblk
	if (type != fat32) {
		dummyVolume.pm_BlkPerSec = dummyVolume.pm_BytesPerSec / DEV_BSIZE;
		dummyVolume.pm_fatblk = dummyVolume.pm_ResSectors * dummyVolume.pm_BlkPerSec;
		dummyVolume.pm_rootdirblk
			= dummyVolume.pm_fatblk + dummyVolume.pm_FATs * dummyVolume.pm_FATsecs;
	}

	char name[LABEL_CSTRING];
	strcpy(name, "no name");
	read_label(&dummyVolume, fd, buf, name);
	sanitize_label(name);

	IdentifyCookie* cookie = new(std::nothrow) IdentifyCookie;
	if (!cookie)
		return -1;
	cookie->fBytesPerSector = dummyVolume.pm_BytesPerSec;
	cookie->fTotalSectors = dummyVolume.pm_HugeSectors;
	strlcpy(cookie->fName, name, 12);

	*_cookie = cookie;

	return 0.8f;
}


static status_t
dosfs_scan_partition(int fd, partition_data* partition, void* _cookie)
{
	IdentifyCookie* cookie = reinterpret_cast<IdentifyCookie*>(_cookie);

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = static_cast<off_t>(cookie->fTotalSectors) * cookie->fBytesPerSector;
	partition->block_size = cookie->fBytesPerSector;
	partition->content_name = strdup(cookie->fName);
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
dosfs_free_identify_partition_cookie(partition_data* partition, void* _cookie)
{
	delete reinterpret_cast<IdentifyCookie*>(_cookie);

	return;
}


static status_t
dosfs_unmount(fs_volume* volume)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	vnode* deviceNode = fatVolume->pm_devvp;
	cdev* bsdDevice = fatVolume->pm_dev;

#ifdef FS_SHELL
	FUNCTION_START("device %" B_PRIdDEV "\n", volume->id);
#else
	FUNCTION_START("device %" B_PRIdDEV ", partition %" B_PRId32 "\n", volume->id,
		volume->partition);
#endif

	status_t status = B_OK;
	status_t returnStatus = B_OK;

	MutexLocker locker(bsdVolume->mnt_mtx.haikuMutex);

	status = fat_volume_uninit(fatVolume);
	if (status != B_OK)
		returnStatus = status;

	// pseudo-BSD layer cleanup
	status = bsd_device_uninit(bsdDevice);
	if (status != B_OK)
		returnStatus = status;
	status = dev_bsd_node_uninit(deviceNode);
	if (status != B_OK)
		returnStatus = status;
	locker.Unlock();
	status = bsd_volume_uninit(bsdVolume);
	if (status != B_OK)
		returnStatus = status;

	RETURN_ERROR(returnStatus);
}


static status_t
dosfs_read_fs_stat(fs_volume* volume, struct fs_info* info)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	cdev* bsdDevice = fatVolume->pm_dev;

	FUNCTION();

	MutexLocker locker(bsdVolume->mnt_mtx.haikuMutex);

	info->flags = B_FS_IS_PERSISTENT | B_FS_HAS_MIME;
	if ((bsdVolume->mnt_flag & MNT_RDONLY) != 0)
		info->flags |= B_FS_IS_READONLY;

	if (bsdDevice->si_geometry->removable == true)
		info->flags |= B_FS_IS_REMOVABLE;

	info->block_size = fatVolume->pm_bpcluster;

	info->io_size = FAT_IO_SIZE;

	info->total_blocks = fatVolume->pm_maxcluster + 1 - 2;
		// convert from index to count and adjust for 2 reserved cluster numbers

	info->free_blocks = fatVolume->pm_freeclustercount;

	info->total_nodes = LONGLONG_MAX;

	info->free_nodes = LONGLONG_MAX;

	strlcpy(info->volume_name, fatVolume->pm_dev->si_name, sizeof(info->volume_name));
	sanitize_label(info->volume_name);

	strlcpy(info->device_name, fatVolume->pm_dev->si_device, sizeof(info->device_name));

	strlcpy(info->fsh_name, "fat", sizeof(info->fsh_name));

	return B_OK;
}


static status_t
dosfs_write_fs_stat(fs_volume* volume, const struct fs_info* info, uint32 mask)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);

	FUNCTION_START("with mask %" B_PRIx32 "\n", mask);

	MutexLocker locker(bsdVolume->mnt_mtx.haikuMutex);

	if ((mask & FS_WRITE_FSINFO_NAME) == 0)
		return B_OK;

	// if it's a r/o file system, then don't allow volume renaming
	if ((bsdVolume->mnt_flag & MNT_RDONLY) != 0)
		return B_READ_ONLY_DEVICE;

	// convert volume_name into an all-caps space-padded array
	char name[LABEL_LENGTH];
	int i, j;
	memset(name, ' ', LABEL_LENGTH);
	PRINT("wfsstat: setting name to %s\n", info->volume_name);
	for (i = j = 0; (i < LABEL_LENGTH) && (info->volume_name[j]); j++) {
		char c = info->volume_name[j];
		if ((c >= 'a') && (c <= 'z'))
			c += 'A' - 'a';
		// spaces acceptable in volume names
		if (strchr(sAcceptable, c) || (c == ' '))
			name[i++] = c;
	}
	if (i == 0) // bad name, kiddo
		return B_BAD_VALUE;
	PRINT("wfsstat: converted to [%11.11s]\n", name);

	// update the BPB, unless the volume is too old to have a label field in the BPB
	void* blockCache = bsdVolume->mnt_cache;
	u_char* buffer;
	status_t status
		= block_cache_get_writable_etc(blockCache, 0, 0, 1, -1, reinterpret_cast<void**>(&buffer));
	if (status != B_OK)
		return status;
	// check for the extended boot signature
	uint32 ebsOffset = FAT32(fatVolume) != 0 ? 0x42 : 0x26;
	uint32 labelOffset = ebsOffset + 5;
	char* memoryLabel = fatVolume->pm_dev->si_name;
	if (buffer[ebsOffset] == EXBOOTSIG) {
		// double check the position by verifying the name presently stored there
		char bpbLabel[LABEL_CSTRING];
		memcpy(bpbLabel, buffer + labelOffset, LABEL_LENGTH);
		sanitize_label(bpbLabel);
		if (strncmp(bpbLabel, memoryLabel, LABEL_LENGTH) == 0) {
			memcpy(buffer + labelOffset, name, LABEL_LENGTH);
		} else {
			INFORM("wfsstat: BPB position check failed\n");
			block_cache_set_dirty(blockCache, 0, false, -1);
			status = B_ERROR;
		}
	}
	block_cache_put(blockCache, 0);

	// update the label file if there is one
	if (bsdVolume->mnt_volentry >= 0) {
		uint8* rootDirBuffer;
		daddr_t rootDirBlock = fatVolume->pm_rootdirblk;
		if (FAT32(fatVolume) == true)
			rootDirBlock = cntobn(fatVolume, fatVolume->pm_rootdirblk);
		daddr_t dirOffset = bsdVolume->mnt_volentry * sizeof(direntry);
		rootDirBlock += dirOffset / DEV_BSIZE;

		status = block_cache_get_writable_etc(blockCache, rootDirBlock, 0, 1, -1,
			reinterpret_cast<void**>(&rootDirBuffer));
		if (status == B_OK) {
			direntry* label_direntry = reinterpret_cast<direntry*>(rootDirBuffer + dirOffset);

			char rootLabel[LABEL_CSTRING];
			memcpy(rootLabel, label_direntry->deName, LABEL_LENGTH);
			sanitize_label(rootLabel);
			if (strncmp(rootLabel, memoryLabel, LABEL_LENGTH) == 0) {
				memcpy(label_direntry->deName, name, LABEL_LENGTH);
			} else {
				INFORM("wfsstat: root directory position check failed\n");
				block_cache_set_dirty(blockCache, rootDirBlock, false, -1);
				status = B_ERROR;
			}
			block_cache_put(blockCache, rootDirBlock);
		}
	} else {
		// A future enhancement could be to create a label direntry if none exists already.
	}

	if (status == B_OK) {
		memcpy(memoryLabel, name, LABEL_LENGTH);
		sanitize_label(memoryLabel);
	}

	if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0)
		_dosfs_sync(bsdVolume, false);

	RETURN_ERROR(status);
}


static status_t
dosfs_sync(fs_volume* volume)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);

	FUNCTION();

	MutexLocker volumeLocker(bsdVolume->mnt_mtx.haikuMutex);
	WriteLocker fatLocker(fatVolume->pm_fatlock.haikuRW);

	RETURN_ERROR(_dosfs_sync(bsdVolume));
}


/*! If data is true, include regular file data in the sync. Otherwise, only sync directories,
	the FAT, and, if applicable, the fsinfo sector.
*/
status_t
_dosfs_sync(struct mount* bsdVolume, bool data)
{
	status_t status = B_OK;
	status_t returnStatus = B_OK;

	status = write_fsinfo(reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data));
	if (status != B_OK) {
		REPORT_ERROR(status);
		returnStatus = status;
	}

	status = block_cache_sync(bsdVolume->mnt_cache);
	if (status != B_OK) {
		REPORT_ERROR(status);
		returnStatus = status;
	}

	if (data == true) {
		status = sync_all_files(bsdVolume);
		if (status != B_OK) {
			REPORT_ERROR(status);
			returnStatus = status;
		}
	}

	return returnStatus;
}


static status_t
dosfs_read_vnode(fs_volume* volume, ino_t id, fs_vnode* vnode, int* _type, uint32* _flags,
	bool reenter)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	struct vnode* bsdNode;

	FUNCTION_START("id %" B_PRIdINO ", type %d, flags %" B_PRIx32 "\n", id, *_type, *_flags);

	MutexLocker locker(bsdVolume->mnt_mtx.haikuMutex);

	// In case 2 threads are concurrently executing get_vnode() with the same ID, verify
	// after locking the volume that the node has not been constructed already.
	if (node_exists(bsdVolume, id) == true)
		return B_BAD_VALUE;

	status_t status = _dosfs_read_vnode(bsdVolume, id, &bsdNode);
	if (status != B_OK)
		RETURN_ERROR(status);

	ASSERT(static_cast<ino_t>(reinterpret_cast<denode*>(bsdNode->v_data)->de_inode) == id);

	vnode->private_node = bsdNode;
	vnode->ops = &gFATVnodeOps;
	if (bsdNode->v_type == VDIR)
		*_type = S_IFDIR;
	else if (bsdNode->v_type == VREG)
		*_type = S_IFREG;
	else
		panic("dosfs_read_vnode:  unknown type\n");

	*_flags = 0;

	return B_OK;
}


/*! Can be used internally by the FS to generate a private node.

 */
static status_t
_dosfs_read_vnode(mount* bsdVolume, const ino_t id, vnode** newNode, bool createFileCache)
{
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);

	status_t status = B_OK;
	u_long dirClust, dirOffset;
	if (id == root_inode(fatVolume)) {
		dirClust = FAT32(fatVolume) == true ? fatVolume->pm_rootdirblk : MSDOSFSROOT;
		dirOffset = MSDOSFSROOT_OFS;
	} else {
		status = get_location(bsdVolume, id, &dirClust, &dirOffset);
		if (status != B_OK)
			return status;
	}

	denode* fatNode;
	status = B_FROM_POSIX_ERROR(deget(fatVolume, dirClust, dirOffset, LK_EXCLUSIVE, &fatNode));
	if (status != B_OK)
		return status;

	vnode* bsdNode = fatNode->de_vnode;
	if (bsdNode->v_type == VREG) {
		status = set_mime_type(bsdNode, false);
		if (status != B_OK)
			REPORT_ERROR(status);

		if (createFileCache) {
			bsdNode->v_cache
				= file_cache_create(fatVolume->pm_dev->si_id, fatNode->de_inode, fatNode->de_FileSize);
			bsdNode->v_file_map
				= file_map_create(fatVolume->pm_dev->si_id, fatNode->de_inode, fatNode->de_FileSize);
		}
	}

	// identify the parent directory
	if (id == root_inode(fatVolume)) {
		bsdNode->v_parent = id;
	} else if (bsdNode->v_type == VREG) {
		bsdNode->v_parent = fatVolume->pm_bpcluster * dirClust;
		assign_inode(bsdVolume, &bsdNode->v_parent);
	}
	// For a directory other than the root directory, there is no easy way to
	// ID the parent. That Will be done in later (in dosfs_walk / dosfs_mkdir).

	bsdNode->v_state = VSTATE_CONSTRUCTED;

	status = vcache_set_constructed(bsdVolume, fatNode->de_inode);
	if (status != B_OK) {
		free(fatNode);
		free(bsdNode);
		return status;
	}

#ifdef DEBUG
	status = vcache_set_node(bsdVolume, fatNode->de_inode, bsdNode);
	if (status != B_OK)
		REPORT_ERROR(status);
#endif // DEBUG

	*newNode = bsdNode;

	rw_lock_write_unlock(&bsdNode->v_vnlock->haikuRW);

	return B_OK;
}


static status_t
dosfs_walk(fs_volume* volume, fs_vnode* dir, const char* name, ino_t* _id)
{
	vnode* bsdDir = reinterpret_cast<vnode*>(dir->private_node);
	denode* fatDir = reinterpret_cast<denode*>(bsdDir->v_data);
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);

	WriteLocker locker(bsdDir->v_vnlock->haikuRW);
		// msdosfs_lookup_ino will modify de_fndoffset, de_fndcnt

	if (bsdDir->v_type != VDIR)
		RETURN_ERROR(B_NOT_A_DIRECTORY);

	ComponentName bsdName((strcmp(name, "..") == 0 ? MAKEENTRY | ISDOTDOT : MAKEENTRY), NOCRED,
		LOOKUP, 0, name);

	daddr_t dirClust;
	u_long dirOffset;
	status_t status = B_FROM_POSIX_ERROR(
		msdosfs_lookup_ino(bsdDir, NULL, bsdName.Data(), &dirClust, &dirOffset));
	if (status != B_OK) {
		entry_cache_add_missing(volume->id, fatDir->de_inode, bsdName.Data()->cn_nameptr);
		RETURN_ERROR(B_ENTRY_NOT_FOUND);
	}
	// msdosfs_lookup_ino will return 0 for cluster number if looking up .. in a directory
	// whose parent is the root directory, even on FAT32 volumes (which reflects the
	// value that is meant to be stored in the .. direntry, per the FAT spec)
	if (FAT32(fatVolume) == true && dirClust == MSDOSFSROOT)
		dirClust = fatVolume->pm_rootdirblk;
	vnode* bsdResult;
	status = assign_inode_and_get(bsdVolume, dirClust, dirOffset, &bsdResult);
	if (status != B_OK)
		RETURN_ERROR(status);
	denode* fatResult = reinterpret_cast<denode*>(bsdResult->v_data);

	if (bsdResult->v_type == VDIR) {
		// dosfs_read_vnode does not set this for directories because it does not know the
		// parent inode
		bsdResult->v_parent = fatDir->de_inode;
	}

	*_id = fatResult->de_inode;

	entry_cache_add(volume->id, fatDir->de_inode, name, fatResult->de_inode);

	return B_OK;
}


static status_t
dosfs_release_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	FUNCTION_START("inode %" B_PRIdINO " @ %p\n", fatNode->de_inode, bsdNode);

	status_t status = B_OK;

	if ((bsdNode->v_vflag & VV_ROOT) == 0) {
		WriteLocker locker(bsdNode->v_vnlock->haikuRW);
			// needed only in this block

		status = B_FROM_POSIX_ERROR(deupdat(fatNode, 0));
		if (status != B_OK)
			RETURN_ERROR(status);

		if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0)
			_dosfs_fsync(bsdNode);
	}

	if (bsdNode->v_type == VREG) {
		status = file_cache_sync(bsdNode->v_cache);
		file_cache_delete(bsdNode->v_cache);
		file_map_delete(bsdNode->v_file_map);
	} else {
		status = discard_clusters(bsdNode, 0);
	}

	vcache_set_constructed(bsdVolume, fatNode->de_inode, false);

	free(fatNode);

	rw_lock_destroy(&bsdNode->v_vnlock->haikuRW);

	free(bsdNode);

	RETURN_ERROR(status);
}


status_t
dosfs_remove_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	FUNCTION_START("%" B_PRIu64 " @ %p\n", fatNode->de_inode, bsdNode);

	WriteLocker locker(bsdNode->v_vnlock->haikuRW);

	if (MOUNTED_READ_ONLY(fatVolume) != 0)
		RETURN_ERROR(B_READ_ONLY_DEVICE);

	status_t status = B_OK;

	if (bsdNode->v_type == VREG) {
		file_cache_delete(bsdNode->v_cache);
		bsdNode->v_cache = NULL;
		file_map_delete(bsdNode->v_file_map);
		bsdNode->v_file_map = NULL;
	} else {
		status = discard_clusters(bsdNode, 0);
		if (status != B_OK)
			REPORT_ERROR(status);
	}

	// truncate the file
	if (fatNode->de_refcnt <= 0 && fatNode->de_StartCluster != root_start_cluster(fatVolume)) {
		rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
		status = B_FROM_POSIX_ERROR(detrunc(fatNode, static_cast<u_long>(0), 0, NOCRED));
		rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
		if (status != B_OK)
			REPORT_ERROR(status);
	}
	if (status == B_OK) {
		// remove vnode id from the cache
		if (find_vnid_in_vcache(bsdVolume, fatNode->de_inode) == B_OK)
			remove_from_vcache(bsdVolume, fatNode->de_inode);

		if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0)
			_dosfs_sync(bsdVolume, false);
	}

	free(fatNode);

	locker.Detach();
	rw_lock_destroy(&bsdNode->v_vnlock->haikuRW);

	free(bsdNode);

	RETURN_ERROR(status);
}


static bool
dosfs_can_page(fs_volume* vol, fs_vnode* vnode, void* cookie)
{
	// ToDo: we're obviously not even asked...
	return false;
}


static status_t
dosfs_read_pages(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, const iovec* vecs,
	size_t count, size_t* _numBytes)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	FUNCTION_START("%p\n", bsdNode);

	if (bsdNode->v_cache == NULL)
		return B_BAD_VALUE;

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	uint32 vecIndex = 0;
	size_t vecOffset = 0;
	size_t bytesLeft = *_numBytes;
	status_t status;

	while (true) {
		struct file_io_vec fileVecs[8];
		size_t fileVecCount = 8;
		bool bufferOverflow;
		size_t bytes = bytesLeft;

		status
			= file_map_translate(bsdNode->v_file_map, pos, bytesLeft, fileVecs, &fileVecCount, 0);
		if (status != B_OK && status != B_BUFFER_OVERFLOW)
			break;

		bufferOverflow = status == B_BUFFER_OVERFLOW;

		status = read_file_io_vec_pages(fatVolume->pm_dev->si_fd, fileVecs, fileVecCount, vecs,
			count, &vecIndex, &vecOffset, &bytes);
		if (status != B_OK || !bufferOverflow)
			break;

		pos += bytes;
		bytesLeft -= bytes;
	}

	RETURN_ERROR(status);
}


static status_t
dosfs_write_pages(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, const iovec* vecs,
	size_t count, size_t* _numBytes)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	uint32 vecIndex = 0;
	size_t vecOffset = 0;
	size_t bytesLeft = *_numBytes;
	status_t status;

	FUNCTION_START("%p\n", bsdNode);

	if (bsdNode->v_cache == NULL)
		return B_BAD_VALUE;

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	if (MOUNTED_READ_ONLY(fatVolume) != 0)
		return B_READ_ONLY_DEVICE;

	while (true) {
		struct file_io_vec fileVecs[8];
		size_t fileVecCount = 8;
		bool bufferOverflow;
		size_t bytes = bytesLeft;

		status
			= file_map_translate(bsdNode->v_file_map, pos, bytesLeft, fileVecs, &fileVecCount, 0);
		if (status != B_OK && status != B_BUFFER_OVERFLOW)
			break;

		bufferOverflow = status == B_BUFFER_OVERFLOW;

		status = write_file_io_vec_pages(fatVolume->pm_dev->si_fd, fileVecs, fileVecCount, vecs,
			count, &vecIndex, &vecOffset, &bytes);
		if (status != B_OK || !bufferOverflow)
			break;

		pos += bytes;
		bytesLeft -= bytes;
	}

	RETURN_ERROR(status);
}


static status_t
dosfs_io(fs_volume* volume, fs_vnode* vnode, void* cookie, io_request* request)
{
#if KDEBUG_RW_LOCK_DEBUG
	// dosfs_io depends on read-locks being implicitly transferrable across threads.
	return B_UNSUPPORTED;
#endif
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

#ifndef FS_SHELL
	if (io_request_is_write(request) && MOUNTED_READ_ONLY(fatVolume) != 0) {
		notify_io_request(request, B_READ_ONLY_DEVICE);
		return B_READ_ONLY_DEVICE;
	}
#endif

	if (bsdNode->v_cache == NULL) {
#ifndef FS_SHELL
		notify_io_request(request, B_BAD_VALUE);
#endif
		panic("dosfs_io:  no file cache\n");
		RETURN_ERROR(B_BAD_VALUE);
	}

	// divert to synchronous IO?
	if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0 || bsdNode->v_sync == true)
		return B_UNSUPPORTED;

	rw_lock_read_lock(&bsdNode->v_vnlock->haikuRW);

	RETURN_ERROR(do_iterative_fd_io(fatVolume->pm_dev->si_fd, request, iterative_io_get_vecs_hook,
		iterative_io_finished_hook, bsdNode));
}


static status_t
dosfs_get_file_map(fs_volume* volume, fs_vnode* vnode, off_t position, size_t length,
	struct file_io_vec* vecs, size_t* _count)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	FUNCTION_START("%" B_PRIuSIZE " bytes at %" B_PRIdOFF " (vnode id %" B_PRIdINO " at %p)\n",
		length, position, fatNode->de_inode, bsdNode);

	size_t max = *_count;
	*_count = 0;

	if ((bsdNode->v_type & VDIR) != 0)
		return B_IS_A_DIRECTORY;

	if (position < 0)
		position = 0;

	size_t fileSize = fatNode->de_FileSize;

	if (fileSize == 0 || length == 0 || static_cast<u_long>(position) >= fileSize)
		return B_OK;

	// truncate to file size, taking overflow into account
	if (static_cast<uint64>(position + length) >= fileSize
		|| static_cast<off_t>(position + length) < position) {
		length = fileSize - position;
	}

	csi iter;
	status_t status = init_csi(fatVolume, fatNode->de_StartCluster, 0, &iter);
	if (status != B_OK)
		RETURN_ERROR(B_IO_ERROR);

	size_t bytesPerSector = fatVolume->pm_BytesPerSec;

	// file-relative sector in which position lies
	uint32 positionSector = position / bytesPerSector;

	if (positionSector > 0) {
		status = iter_csi(&iter, positionSector);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	status = validate_cs(iter.fatVolume, iter.cluster, iter.sector);
	if (status != B_OK)
		RETURN_ERROR(status);

	int32 sectorOffset = position % bytesPerSector;
	size_t index = 0;

	// Each iteration populates one vec
	while (length > 0) {
		off_t initFsSector = fs_sector(&iter);
		uint32 sectors = 1;

		length -= min_c(length, bytesPerSector - sectorOffset);

		// Each iteration advances iter to the next sector of the file.
		// Break when iter reaches the first sector of a non-contiguous cluster.
		while (length > 0) {
			status = iter_csi(&iter, 1);
			ASSERT(status == B_OK);
			status = validate_cs(iter.fatVolume, iter.cluster, iter.sector);
			if (status != B_OK)
				RETURN_ERROR(status);

			if (initFsSector + sectors != fs_sector(&iter)) {
				// disjoint sectors, need to flush and begin a new vector
				break;
			}

			length -= min_c(length, bytesPerSector);
			sectors++;
		}

		vecs[index].offset = initFsSector * bytesPerSector + sectorOffset;
		vecs[index].length = sectors * bytesPerSector - sectorOffset;
		position += vecs[index].length;

		// for the last vector only, extend to the end of the last cluster
		if (length == 0) {
			if (IS_FIXED_ROOT(fatNode) == 0) {
				uint32 remainder = position % fatVolume->pm_bpcluster;
				if (remainder != 0)
					vecs[index].length += (fatVolume->pm_bpcluster - remainder);
			}
		}

		index++;

		if (index >= max) {
			// we're out of file_io_vecs; let's bail out
			*_count = index;
			return B_BUFFER_OVERFLOW;
		}

		sectorOffset = 0;
	}

	*_count = index;

	return B_OK;
}


static status_t
dosfs_fsync(fs_volume* volume, fs_vnode* vnode)
{
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	FUNCTION_START("%p\n", bsdNode);

	return _dosfs_fsync(bsdNode);
}


static status_t
_dosfs_fsync(struct vnode* bsdNode)
{
	mount* bsdVolume = bsdNode->v_mount;
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	status_t status = B_OK;
	if (bsdNode->v_cache != NULL) {
		PRINT("fsync:  file_cache_sync\n");
		status = file_cache_sync(bsdNode->v_cache);
	} else {
		status = sync_clusters(bsdNode);
	}

	// If user chose op sync mode, flush the whole block cache. This will ensure that
	// the metadata that is external to the direntry (FAT chain for this file and all directory
	// files in the hierarchy above this file) is also synced. If not, just sync the FAT and the
	// node's direntry, if it has one (the root directory doesn't).
	status_t externStatus = B_OK;

	if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0) {
		externStatus = block_cache_sync(bsdVolume->mnt_cache);
		if (externStatus != B_OK)
			REPORT_ERROR(externStatus);
	} else {
		size_t fatBlocks = (fatVolume->pm_fatsize * fatVolume->pm_FATs) / DEV_BSIZE;
		status_t fatStatus
			= block_cache_sync_etc(bsdVolume->mnt_cache, fatVolume->pm_fatblk, fatBlocks);
		if (fatStatus != B_OK) {
			externStatus = fatStatus;
			REPORT_ERROR(fatStatus);
		}
		if ((bsdNode->v_vflag & VV_ROOT) == 0) {
			status_t entryStatus = B_FROM_POSIX_ERROR(deupdat(fatNode, 1));
			if (entryStatus != B_OK) {
				externStatus = entryStatus;
				REPORT_ERROR(entryStatus);
			}
		}
	}

	if (status == B_OK)
		status = externStatus;

	RETURN_ERROR(status);
}


static status_t
dosfs_link(fs_volume* volume, fs_vnode* dir, const char* name, fs_vnode* vnode)
{
	FUNCTION_START("attempt to assign %s to %p in directory %p\n", name, vnode, dir);

	return B_UNSUPPORTED;
}


static status_t
dosfs_unlink(fs_volume* volume, fs_vnode* dir, const char* name)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	vnode* bsdDir = reinterpret_cast<vnode*>(dir->private_node);
	denode* fatDir = reinterpret_cast<denode*>(bsdDir->v_data);
	vnode* bsdNode = NULL;
	denode* fatNode = NULL;

	FUNCTION_START("%s in directory @ %p\n", name, bsdDir);

	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return B_NOT_ALLOWED;

	ComponentName bsdName(ISLASTCN, NOCRED, DELETE, 0, name);

	// multiple unlinks of files in the same dir would interfere when msdosfs_lookup_ino sets
	// de_fndofset and de_fndcnt of the parent node
	WriteLocker dirLocker(bsdDir->v_vnlock->haikuRW);

	// set bsdNode to the file to be removed
	daddr_t cluster;
	u_long offset;
	status_t status
		= B_FROM_POSIX_ERROR(msdosfs_lookup_ino(bsdDir, NULL, bsdName.Data(), &cluster, &offset));
	if (status != B_OK)
		RETURN_ERROR(status);
	status = assign_inode_and_get(bsdVolume, cluster, offset, &bsdNode);
	if (status != B_OK)
		RETURN_ERROR(status);
	WriteLocker nodeLocker(bsdNode->v_vnlock->haikuRW);
	NodePutter nodePutter(bsdNode);
	fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	if (bsdNode->v_type == VDIR)
		return B_IS_A_DIRECTORY;

	status = _dosfs_access(bsdVolume, bsdNode, W_OK);
	if (status != B_OK)
		RETURN_ERROR(B_NOT_ALLOWED);

	status = B_FROM_POSIX_ERROR(removede(fatDir, fatNode));
	if (status != B_OK)
		RETURN_ERROR(status);

	// Set the loc to a unique value. This effectively removes it from the
	// vcache without releasing its vnid for reuse. It also nicely reserves
	// the vnid from use by other nodes. This is okay because the vnode is
	// locked in memory after this point and loc will not be referenced from
	// here on.
	ino_t ino = fatNode->de_inode;
	status = vcache_set_entry(bsdVolume, ino, generate_unique_vnid(bsdVolume));
	if (status != B_OK)
		RETURN_ERROR(status);

	status = remove_vnode(volume, ino);
	if (status != B_OK)
		RETURN_ERROR(status);

	status = entry_cache_remove(volume->id, fatDir->de_inode, name);
	if (status != B_OK)
		REPORT_ERROR(status);

	notify_entry_removed(volume->id, fatDir->de_inode, name, ino);

	nodeLocker.Unlock();

	if (status == B_OK && (bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0) {
		// sync the parent directory changes
		_dosfs_sync(bsdVolume, false);
	}

	RETURN_ERROR(status);
}


/*!
	What follows is the basic algorithm:

	if (file move) {
		if (dest file exists)
			remove dest file
		if (dest and src in same directory) {
			rewrite name in existing directory slot
		} else {
			write new entry in dest directory
			update offset and dirclust in denode
			clear old directory entry
		}
	} else {
		directory move
		if (dest directory exists) {
			if (dest is not empty)
				return ENOTEMPTY
			remove dest directory
		}
		if (dest and src in same directory)
			rewrite name in existing entry
		else {
			be sure dest is not a child of src directory
			write entry in dest directory
			update "." and ".." in moved directory
			clear old directory entry for moved directory
		}
	}
*/
status_t
dosfs_rename(fs_volume* volume, fs_vnode* fromDir, const char* fromName, fs_vnode* toDir,
	const char* toName)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	vnode* fromDirBsdNode = reinterpret_cast<vnode*>(fromDir->private_node);
	vnode* toDirBsdNode = reinterpret_cast<vnode*>(toDir->private_node);

	if (fromDir == toDir && !strcmp(fromName, toName))
		return B_OK;

	if (is_filename_legal(toName) == false) {
		INFORM("file name '%s' is not permitted in the FAT filesystem\n", toName);
		return B_BAD_VALUE;
	}

	ComponentName fromBsdName(ISLASTCN, NOCRED, RENAME, 0, fromName);
	ComponentName toBsdName(ISLASTCN, NOCRED, RENAME, 0, toName);

	// Don't do 2 renames at the same time on the same volume. If moving to a new directory,
	// and the destination directory of one thread is the origin directory of the other,
	// and vice versa, a deadlock can occur.
	MutexLocker volumeLocker(bsdVolume->mnt_mtx.haikuMutex);

	WriteLocker fromDirLocker(fromDirBsdNode->v_vnlock->haikuRW);
	WriteLocker toDirLocker;
	if (fromDirBsdNode != toDirBsdNode)
		toDirLocker.SetTo(toDirBsdNode->v_vnlock->haikuRW, false);

	status_t status = _dosfs_access(bsdVolume, fromDirBsdNode, W_OK);
	if (status == B_OK && fromDirBsdNode != toDirBsdNode)
		status = _dosfs_access(bsdVolume, toDirBsdNode, W_OK);
	if (status != B_OK)
		RETURN_ERROR(status);

	// get the 'from' node
	daddr_t fromCluster;
	u_long fromOffset;
	status = B_FROM_POSIX_ERROR(
		msdosfs_lookup_ino(fromDirBsdNode, NULL, fromBsdName.Data(), &fromCluster, &fromOffset));
	if (status != B_OK)
		RETURN_ERROR(status);
	vnode* fromBsdNode;
	status = assign_inode_and_get(bsdVolume, fromCluster, fromOffset, &fromBsdNode);
	if (status != B_OK)
		RETURN_ERROR(status);
	NodePutter fromPutter(fromBsdNode);
	WriteLocker fromLocker(fromBsdNode->v_vnlock->haikuRW);

	// make sure the from entry wasn't deleted before we locked it
	status = B_FROM_POSIX_ERROR(
		msdosfs_lookup_ino(fromDirBsdNode, NULL, fromBsdName.Data(), &fromCluster, &fromOffset));
	if (status != B_OK) {
		INFORM("dosfs_rename:  file no longer present\n");
		RETURN_ERROR(status);
	}

	// get the "to" node, if the target name already exists
	daddr_t toCluster;
	u_long toOffset;
	status = B_FROM_POSIX_ERROR(
		msdosfs_lookup_ino(toDirBsdNode, NULL, toBsdName.Data(), &toCluster, &toOffset));
	if (status != B_OK && status != B_FROM_POSIX_ERROR(EJUSTRETURN))
		RETURN_ERROR(status);
	vnode* toBsdNode = NULL;
	if (status == B_OK) {
		// the target name does exist
		status = assign_inode_and_get(bsdVolume, toCluster, toOffset, &toBsdNode);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	// Is toName equivalent to fromName in the FAT filesystem?
	bool caseChange = false;
	if (fromBsdNode == toBsdNode) {
		// The names they must differ only in capitalization. Ignore the match that was found for
		// the "to" node.
		put_vnode(volume, reinterpret_cast<denode*>(toBsdNode->v_data)->de_inode);
		toBsdNode = NULL;
		caseChange = true;
	}

	NodePutter toPutter;
	WriteLocker toLocker;

	if (toBsdNode != NULL) {
		status = msdosfs_lookup_ino(toDirBsdNode, NULL, toBsdName.Data(), &toCluster, &toOffset);
		if (status != 0) {
			toBsdNode = NULL;
			status = B_OK;
		} else {
			toLocker.SetTo(toBsdNode->v_vnlock->haikuRW, false);
			toPutter.SetTo(toBsdNode);
		}
	}

	denode* fromDirFatNode = reinterpret_cast<denode*>(fromDirBsdNode->v_data);
	denode* fromFatNode = reinterpret_cast<denode*>(fromBsdNode->v_data);
	denode* toDirFatNode = reinterpret_cast<denode*>(toDirBsdNode->v_data);
	denode* toFatNode = toBsdNode != NULL ? reinterpret_cast<denode*>(toBsdNode->v_data) : NULL;

	PRINT("dosfs_rename: %" B_PRIu64 "/%s->%" B_PRIu64 "/%s\n", fromDirFatNode->de_inode, fromName,
		toDirFatNode->de_inode, toName);

	u_long toDirOffset = toDirFatNode->de_fndoffset;

	// Is fromName a directory?
	bool doingDirectory = false;
	// Be sure we are not renaming ".", "..", or an alias of ".". This leads to a
	// crippled directory tree. It's pretty tough to do a "ls" or "pwd" with the
	// "." directory entry missing, and "cd .."doesn't work if the ".." entry is missing.
	if ((fromFatNode->de_Attributes & ATTR_DIRECTORY) != 0) {
		// Avoid ".", "..", and aliases of "." for obvious reasons.
		if ((fromBsdName.Data()->cn_namelen == 1 && fromBsdName.Data()->cn_nameptr[0] == '.')
			|| fromDirFatNode == fromFatNode || (fromBsdName.Data()->cn_flags & ISDOTDOT) != 0
			|| (fromBsdName.Data()->cn_flags & ISDOTDOT) != 0) {
			RETURN_ERROR(B_BAD_VALUE);
		}
		doingDirectory = true;
	}

	// Is the target being moved to new parent directory?
	bool newParent = fromDirFatNode != toDirFatNode ? true : false;

	// If ".." must be changed (ie the directory gets a new parent) then the source
	// directory must not be in the directory hierarchy above the target, as this would
	// orphan everything below the source directory. Also the user must have write
	// permission in the source so as to be able to change "..".
	status = _dosfs_access(bsdVolume, fromBsdNode, W_OK);
	if (doingDirectory && newParent) {
		if (status != B_OK) // write access check above
			RETURN_ERROR(status);

		rw_lock_write_lock(&fatVolume->pm_checkpath_lock.haikuRW);

		// The BSD function doscheckpath requires a third argument to return the location of
		// any child directory of fromFatNode that is locked by another thread. In the port we
		// don't use make use of this information, we just wait for that node to be unlocked.
		daddr_t dummy;
		// Switch the 'to' directory from the WriteLocker to a simple lock. This is a workaround
		// for problems that occur when doscheckpath() works with the node lock, while that lock
		// is held by a WriteLocker.
		rw_lock_write_lock(&toDirBsdNode->v_vnlock->haikuRW);
		toDirLocker.Unlock();
		status = B_FROM_POSIX_ERROR(doscheckpath(fromFatNode, toDirFatNode, &dummy));
		toDirLocker.Lock();
		rw_lock_write_unlock(&toDirBsdNode->v_vnlock->haikuRW);

		rw_lock_write_unlock(&fatVolume->pm_checkpath_lock.haikuRW);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	if (toFatNode != NULL) {
		// Target must be empty if a directory and have no links to it. Also, ensure source and
		// target are compatible (both directories, or both not directories).
		if ((toFatNode->de_Attributes & ATTR_DIRECTORY) != 0) {
			if (!dosdirempty(toFatNode))
				RETURN_ERROR(B_DIRECTORY_NOT_EMPTY);
			if (!doingDirectory)
				RETURN_ERROR(B_NOT_A_DIRECTORY);
			entry_cache_remove(volume->id, toDirFatNode->de_inode, toBsdName.Data()->cn_nameptr);
		} else if (doingDirectory) {
			RETURN_ERROR(B_IS_A_DIRECTORY);
		}

		// delete the file/directory that we are overwriting
		daddr_t remCluster;
		u_long remOffset;
		status = msdosfs_lookup_ino(toDirBsdNode, NULL, toBsdName.Data(), &remCluster, &remOffset);
			// set de_fndoffset for use by removede
		status = B_FROM_POSIX_ERROR(removede(toDirFatNode, toFatNode));
		if (status != B_OK)
			RETURN_ERROR(status);

		// Set the loc to a unique value. This effectively removes it from the vcache without
		// releasing its vnid for reuse. It also nicely reserves the vnid from use by other
		// nodes. This is okay because the vnode is locked in memory after this point and loc
		// will not be referenced from here on.
		vcache_set_entry(bsdVolume, toFatNode->de_inode, generate_unique_vnid(bsdVolume));

		entry_cache_remove(volume->id, toDirFatNode->de_inode, toName);
		notify_entry_removed(volume->id, toDirFatNode->de_inode, toName, toFatNode->de_inode);

		remove_vnode(volume, toFatNode->de_inode);

		toLocker.Unlock();
		toPutter.Put();

		toBsdNode = NULL;
		toFatNode = NULL;
	}

	// Convert the filename in toBsdName into a dos filename. We copy this into the denode and
	// directory entry for the destination file/directory.
	u_char toShortName[SHORTNAME_CSTRING], oldShortNameArray[SHORTNAME_LENGTH];
	if (caseChange == false) {
		status = B_FROM_POSIX_ERROR(uniqdosname(toDirFatNode, toBsdName.Data(), toShortName));
		if (status != B_OK)
			RETURN_ERROR(status);
		if (is_shortname_legal(toShortName) == false)
			return B_NOT_ALLOWED;
	}
	// if only changing case, the dos filename (always all-caps) will remain the same

	// First write a new entry in the destination directory and mark the entry in the source
	// directory as deleted. If we moved a directory, then update its .. entry to point to
	// the new parent directory.
	if (caseChange == false) {
		memcpy(oldShortNameArray, fromFatNode->de_Name, SHORTNAME_LENGTH);
		memcpy(fromFatNode->de_Name, toShortName, SHORTNAME_LENGTH); // update denode
	} else {
		// We prefer to create the new dir entry before removing the old one, but if only
		// changing case, we remove the old dir entry first, so that msdosfs_lookup_ino call below
		// won't see it as a match for the to-name when it does its case-insensitive search,
		// which would cause it to return before it has found empty slots for the new dir entry.
		status = B_FROM_POSIX_ERROR(removede(fromDirFatNode, fromFatNode));
		if (status != B_OK) {
			INFORM("rename removede error:  %" B_PRIu64 "/%" B_PRIu64 ": %s\n",
				fromDirFatNode->de_inode, fromFatNode->de_inode, strerror(status));
			msdosfs_integrity_error(fatVolume);
			RETURN_ERROR(status);
		}
	}

	daddr_t createCluster;
	u_long createOffset;
	status
		= msdosfs_lookup_ino(toDirBsdNode, NULL, toBsdName.Data(), &createCluster, &createOffset);
	rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
		// the FAT will be updated if the directory needs to be extended to hold another dirent
	if (status == EJUSTRETURN) {
		toDirFatNode->de_fndoffset = toDirOffset;
			// if the to-name already existed, ensure that creatde will write the new
			// direntry to the space previously occupied by the (removed) to-name entry
		status = createde(fromFatNode, toDirFatNode, NULL, toBsdName.Data());
	}
	rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
	if (status != B_OK) {
		if (caseChange == true) {
			// We failed to create the new dir entry, and the old dir entry is already gone.
			// Try to restore the old entry.  Since the old name is a case variant of the new name
			// in the same directory, creating an entry with the old name will probably fail too.
			// Use the dos name instead of the long name, to simplify entry creation and try to
			// avoid the same mode of failure.
			ComponentName restoreName(ISLASTCN, NOCRED, CREATE, 0,
				reinterpret_cast<char*>(fromFatNode->de_Name));
			createde(fromFatNode, fromDirFatNode, NULL, restoreName.Data());
		} else {
			// we haven't removed the old dir entry yet
			memcpy(fromFatNode->de_Name, oldShortNameArray, SHORTNAME_LENGTH);
		}
		RETURN_ERROR(B_FROM_POSIX_ERROR(status));
	}

	// If fromFatNode is for a directory, then its name should always be "." since it is for the
	// directory entry in the directory itself (msdosfs_lookup() always translates to the "."
	// entry so as to get a unique denode, except for the root directory there are different
	// complications). However, we just corrupted its name to pass the correct name to
	// createde(). Undo this.
	if ((fromFatNode->de_Attributes & ATTR_DIRECTORY) != 0)
		memcpy(fromFatNode->de_Name, oldShortNameArray, SHORTNAME_LENGTH);
	fromFatNode->de_refcnt++;
		// offset the decrement that will occur in removede
	daddr_t remFromCluster;
	u_long remFromOffset;
	status = msdosfs_lookup_ino(fromDirBsdNode, NULL, fromBsdName.Data(), &remFromCluster,
		&remFromOffset);
	if (caseChange == false) {
		status = B_FROM_POSIX_ERROR(removede(fromDirFatNode, fromFatNode));
		if (status != B_OK) {
			INFORM("rename removede error:  %" B_PRIu64 "/%" B_PRIu64 ": %s\n",
				fromDirFatNode->de_inode, fromFatNode->de_inode, strerror(status));
			msdosfs_integrity_error(fatVolume);
			RETURN_ERROR(status);
		}
	}
	if (!doingDirectory) {
		status = B_FROM_POSIX_ERROR(pcbmap(toDirFatNode, de_cluster(fatVolume, toDirOffset), 0,
			&fromFatNode->de_dirclust, 0));
		if (status != B_OK) {
			msdosfs_integrity_error(fatVolume);
				// fs is corrupt
			RETURN_ERROR(status);
		}
		if (fromFatNode->de_dirclust == MSDOSFSROOT)
			fromFatNode->de_diroffset = toDirOffset;
		else
			fromFatNode->de_diroffset = toDirOffset & fatVolume->pm_crbomask;
	}

	fromBsdNode->v_parent = toDirFatNode->de_inode;

	ino_t newLocation = DETOI(fatVolume, fromFatNode->de_dirclust, fromFatNode->de_diroffset);
	vcache_set_entry(bsdVolume, fromFatNode->de_inode, newLocation);

	// If we moved a directory to a new parent directory, then we must fixup the ".." entry in
	// the moved directory.
	if (doingDirectory && newParent) {
		buf* dotDotBuf = NULL;
		u_long clustNumber = fromFatNode->de_StartCluster;
		ASSERT(clustNumber != MSDOSFSROOT);
			// this should never happen
		daddr_t blockNumber = cntobn(fatVolume, clustNumber);
		status = B_FROM_POSIX_ERROR(
			bread(fatVolume->pm_devvp, blockNumber, fatVolume->pm_bpcluster, NOCRED, &dotDotBuf));
		if (status != B_OK) {
			INFORM("rename read error:  %" B_PRIu64 "/%" B_PRIu64 ": %s\n",
				fromDirFatNode->de_inode, fromFatNode->de_inode, strerror(status));
			msdosfs_integrity_error(fatVolume);
			RETURN_ERROR(status);
		}
		direntry* dotDotEntry = reinterpret_cast<direntry*>(dotDotBuf->b_data) + 1;
		u_long parentClust = toDirFatNode->de_StartCluster;
		if (FAT32(fatVolume) == true && parentClust == fatVolume->pm_rootdirblk)
			parentClust = MSDOSFSROOT;
		putushort(dotDotEntry->deStartCluster, parentClust);
		if (FAT32(fatVolume) == true)
			putushort(dotDotEntry->deHighClust, parentClust >> 16);
		if (DOINGASYNC(fromBsdNode)) {
			bdwrite(dotDotBuf);
		} else if ((status = B_FROM_POSIX_ERROR(bwrite(dotDotBuf))) != B_OK) {
			INFORM("rename write error:  %" B_PRIu64 "/%" B_PRIu64 ":  %s\n",
				fromDirFatNode->de_inode, fromFatNode->de_inode, strerror(status));
			msdosfs_integrity_error(fatVolume);
			RETURN_ERROR(status);
		}
		entry_cache_add(volume->id, fromFatNode->de_inode, "..", toDirFatNode->de_inode);
	}

	status = entry_cache_remove(volume->id, fromDirFatNode->de_inode, fromName);
	if (status != B_OK)
		REPORT_ERROR(status);
	status = entry_cache_add(volume->id, toDirFatNode->de_inode, toName, fromFatNode->de_inode);
	if (status != B_OK)
		REPORT_ERROR(status);

	status = notify_entry_moved(volume->id, fromDirFatNode->de_inode, fromName,
		toDirFatNode->de_inode, toName, fromFatNode->de_inode);
	if (status != B_OK)
		REPORT_ERROR(status);

	set_mime_type(fromBsdNode, true);

	if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0) {
		// sync the directory entry changes
		status = block_cache_sync(bsdVolume->mnt_cache);
	}

	RETURN_ERROR(status);
}


static status_t
dosfs_access(fs_volume* vol, fs_vnode* node, int mode)
{
	mount* bsdVolume = reinterpret_cast<mount*>(vol->private_volume);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(node->private_node);

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	RETURN_ERROR(_dosfs_access(bsdVolume, bsdNode, mode));
}


status_t
_dosfs_access(const mount* bsdVolume, const struct vnode* bsdNode, const int mode)
{
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);

	if ((mode & W_OK) != 0 && MOUNTED_READ_ONLY(fatVolume))
		RETURN_ERROR(B_READ_ONLY_DEVICE);

	mode_t fileMode = 0;
	mode_bits(bsdNode, &fileMode);

	// userlandfs does not provide check_access_permissions
#ifdef USER
	return check_access_permissions_internal(mode, fileMode, fatVolume->pm_gid, fatVolume->pm_uid);
#else
	return check_access_permissions(mode, fileMode, fatVolume->pm_gid, fatVolume->pm_uid);
#endif
}


static status_t
dosfs_rstat(fs_volume* volume, fs_vnode* vnode, struct stat* stat)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	// file mode bits
	mode_bits(bsdNode, &stat->st_mode);
	// file type bits
	status_t status = B_OK;
	if (bsdNode->v_type == VDIR)
		stat->st_mode |= S_IFDIR;
	else if (bsdNode->v_type == VREG)
		stat->st_mode |= S_IFREG;
	else
		status = B_BAD_VALUE;

	stat->st_nlink = 1;

	// The FAT filesystem does not keep track of ownership at the file level
	stat->st_uid = fatVolume->pm_uid;

	stat->st_gid = fatVolume->pm_gid;

	stat->st_size = fatNode->de_FileSize;

	stat->st_blksize = FAT_IO_SIZE;

	fattime2timespec(fatNode->de_MDate, fatNode->de_MTime, 0, 1, &stat->st_mtim);

	// FAT does not keep a record of last change time
	stat->st_ctim = stat->st_mtim;

	fattime2timespec(fatNode->de_ADate, 0, 0, 1, &stat->st_atim);

	fattime2timespec(fatNode->de_CDate, fatNode->de_CTime, fatNode->de_CHun, 1, &stat->st_crtim);

	stat->st_blocks = howmany(fatNode->de_FileSize, 512);

	RETURN_ERROR(status);
}


static status_t
dosfs_wstat(fs_volume* volume, fs_vnode* vnode, const struct stat* stat, uint32 statMask)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	FUNCTION_START("inode %" B_PRIu64 ", @ %p\n", fatNode->de_inode, bsdNode);

	WriteLocker locker(bsdNode->v_vnlock->haikuRW);

	bool hasWriteAccess = _dosfs_access(bsdVolume, bsdNode, W_OK) == B_OK;
	uid_t uid = geteuid();
	bool isOwnerOrRoot = uid == 0 || uid == fatVolume->pm_uid;
	;

	// We don't allow setting attributes on the root directory. The special case for the root
	// directory is because before FAT32, the root directory didn't have an entry for itself
	// (and was otherwise special). With FAT32, the root directory is not so special, but still
	// doesn't have an entry for itself.
	if (bsdNode->v_vflag & VV_ROOT)
		RETURN_ERROR(B_BAD_VALUE);

	off_t previousSize = fatNode->de_FileSize;
	status_t status = B_OK;

	if ((statMask & B_STAT_SIZE) != 0) {
		if (!hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);

		switch (bsdNode->v_type) {
			case VDIR:
				return B_IS_A_DIRECTORY;
			case VREG:
				break;
			default:
				return B_BAD_VALUE;
				break;
		}

		if (stat->st_size >= MSDOSFS_FILESIZE_MAX)
			RETURN_ERROR(B_FILE_TOO_LARGE);

		bool shrinking = previousSize > stat->st_size;

		// If growing the file, detrunc will call deextend, which tries to zero out the new
		// clusters. We use the v_resizing flag to disable writes during detrunc to prevent that,
		// because using file_cache_write while the node is locked can cause a deadlock.
		// The new clusters will be cleared after return from detrunc instead.
		// We also disable writes in the case of shrinking the file because, unlike the detrunc
		// call in create or open, which always truncate to zero, this call will most likely pass
		// a size that is not a multiple of cluster size, so detrunc will want to zero out the end
		// of the last cluster.
		bsdNode->v_resizing = true;
		rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
		status = B_FROM_POSIX_ERROR(detrunc(fatNode, stat->st_size, 0, NOCRED));
		rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
		bsdNode->v_resizing = false;
		if (status != B_OK)
			RETURN_ERROR(status);

		PRINT("dosfs_wstat: inode %" B_PRIu64 ", @ %p size change from %" B_PRIdOFF " to %" B_PRIu64
			"\n", fatNode->de_inode, bsdNode, previousSize, stat->st_size);

		locker.Unlock();
			// avoid deadlock with dosfs_io
		file_cache_set_size(bsdNode->v_cache, fatNode->de_FileSize);
		if (shrinking == false && (statMask & B_STAT_SIZE_INSECURE) == 0) {
			status = fill_gap_with_zeros(bsdNode, previousSize, fatNode->de_FileSize);
			if (status != B_OK)
				RETURN_ERROR(status);
		}
		locker.Lock();

		if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0)
			_dosfs_fsync(bsdNode);

		fatNode->de_Attributes |= ATTR_ARCHIVE;
		fatNode->de_flag |= DE_MODIFIED;
	}

	// DOS files only have the ability to have their writability attribute set, so we use the
	// owner write bit to set the readonly attribute.
	if ((statMask & B_STAT_MODE) != 0) {
		if (!isOwnerOrRoot)
			RETURN_ERROR(B_NOT_ALLOWED);
		PRINT("setting file mode to %o\n", stat->st_mode);
		if (bsdNode->v_type != VDIR) {
			if ((stat->st_mode & S_IWUSR) == 0)
				fatNode->de_Attributes |= ATTR_READONLY;
			else
				fatNode->de_Attributes &= ~ATTR_READONLY;

			// We don't set the archive bit when modifying the time of
			// a directory to emulate the Windows/DOS behavior.
			fatNode->de_Attributes |= ATTR_ARCHIVE;
			fatNode->de_flag |= DE_MODIFIED;
		}
	}

	if ((statMask & B_STAT_UID) != 0) {
		PRINT("cannot set UID at file level\n");
		if (stat->st_uid != fatVolume->pm_uid)
			status = B_BAD_VALUE;
	}

	if ((statMask & B_STAT_GID) != 0) {
		PRINT("cannot set GID at file level\n");
		if (stat->st_gid != fatVolume->pm_gid)
			status = B_BAD_VALUE;
	}

	if ((statMask & B_STAT_ACCESS_TIME) != 0) {
		PRINT("setting access time\n");
		fatNode->de_flag &= ~DE_ACCESS;
		struct timespec atimGMT;
		local_to_GMT(&stat->st_atim, &atimGMT);
		timespec2fattime(&atimGMT, 0, &fatNode->de_ADate, NULL, NULL);
		if (bsdNode->v_type != VDIR)
			fatNode->de_Attributes |= ATTR_ARCHIVE;
		fatNode->de_flag |= DE_MODIFIED;
	}

	if ((statMask & B_STAT_MODIFICATION_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);
		PRINT("setting modification time\n");
		fatNode->de_flag &= ~DE_UPDATE;
		struct timespec mtimGMT;
		local_to_GMT(&stat->st_mtim, &mtimGMT);
		timespec2fattime(&mtimGMT, 0, &fatNode->de_MDate, &fatNode->de_MTime, NULL);
		if (bsdNode->v_type != VDIR)
			fatNode->de_Attributes |= ATTR_ARCHIVE;
		fatNode->de_flag |= DE_MODIFIED;
	}

	if ((statMask & B_STAT_CREATION_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);
		PRINT("setting creation time\n");
		struct timespec crtimGMT;
		local_to_GMT(&stat->st_crtim, &crtimGMT);
		timespec2fattime(&crtimGMT, 0, &fatNode->de_CDate, &fatNode->de_CTime, NULL);
		fatNode->de_flag |= DE_MODIFIED;
	}

	// node change time is not recorded in the FAT file system

	status = B_FROM_POSIX_ERROR(deupdat(fatNode, (bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0));

	notify_stat_changed(volume->id, bsdNode->v_parent, fatNode->de_inode, statMask);

	RETURN_ERROR(status);
}


static status_t
dosfs_create(fs_volume* volume, fs_vnode* dir, const char* name, int openMode, int perms,
	void** _cookie, ino_t* _newVnodeID)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	vnode* bsdDir = reinterpret_cast<vnode*>(dir->private_node);
	denode* fatDir = reinterpret_cast<denode*>(bsdDir->v_data);

	FUNCTION_START("create %s in %" B_PRIu64 ", perms = %o openMode =%o\n", name, fatDir->de_inode,
		perms, openMode);

	ComponentName bsdName(ISLASTCN | MAKEENTRY, NOCRED, CREATE, 0, name);

	WriteLocker locker(bsdDir->v_vnlock->haikuRW);

	if (_dosfs_access(bsdVolume, bsdDir, open_mode_to_access(openMode)) != B_OK)
		RETURN_ERROR(B_NOT_ALLOWED);

	if ((openMode & O_NOCACHE) != 0)
		RETURN_ERROR(B_UNSUPPORTED);

	if (is_filename_legal(name) != true) {
		INFORM("invalid FAT file name '%s'\n", name);
		RETURN_ERROR(B_UNSUPPORTED);
	}

	bool removed = false;
	status_t status = get_vnode_removed(volume, fatDir->de_inode, &removed);
	if (status == B_OK && removed == true)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	if ((openMode & O_RWMASK) == O_RDONLY)
		RETURN_ERROR(B_NOT_ALLOWED);

	FileCookie* cookie = new(std::nothrow) FileCookie;
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<FileCookie> cookieDeleter(cookie);

	// In addition to checking for an existing file with this name, msdosfs_lookup_ino
	// will set de_fndoffset of the parent node to a vacant direntry slot if there is
	// no existing file, in preparation for createde.
	daddr_t cluster;
	u_long offset;
	status
		= B_FROM_POSIX_ERROR(msdosfs_lookup_ino(bsdDir, NULL, bsdName.Data(), &cluster, &offset));

	if (status == B_OK) {
		// there is already a file with this name
		vnode* existingBsdNode;
		status = assign_inode_and_get(bsdVolume, cluster, offset, &existingBsdNode);
		if (status != B_OK)
			RETURN_ERROR(status);
		WriteLocker existingLocker(existingBsdNode->v_vnlock->haikuRW);
		NodePutter existingPutter(existingBsdNode);
		denode* existingFatNode = reinterpret_cast<denode*>(existingBsdNode->v_data);

		if ((openMode & O_EXCL) != 0)
			RETURN_ERROR(B_FILE_EXISTS);
		if (existingBsdNode->v_type == VDIR)
			RETURN_ERROR(B_NOT_ALLOWED);
		if ((openMode & O_TRUNC) != 0) {
			status = _dosfs_access(bsdVolume, existingBsdNode, open_mode_to_access(openMode));
			if (status != B_OK)
				RETURN_ERROR(status);
			rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
			status = B_FROM_POSIX_ERROR(detrunc(existingFatNode, 0, 0, NOCRED));
			rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
			if (status != B_OK)
				RETURN_ERROR(status);

			existingLocker.Unlock();
				// avoid deadlock that can happen when reducing cache size
			file_cache_set_size(existingBsdNode->v_cache, 0);

			if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0)
				_dosfs_fsync(existingBsdNode);
		} else {
			status = _dosfs_access(bsdVolume, existingBsdNode, open_mode_to_access(openMode));
			if (status != B_OK)
				RETURN_ERROR(status);
		}

		*_newVnodeID = existingFatNode->de_inode;

		cookie->fMode = openMode;
		cookie->fLastSize = existingFatNode->de_FileSize;
		cookie->fMtimeAtOpen = existingFatNode->de_MTime;
		cookie->fMdateAtOpen = existingFatNode->de_MDate;
		cookie->fLastNotification = 0;
		*_cookie = cookie;
		cookieDeleter.Detach();

		return B_OK;
	}

	if (status != B_FROM_POSIX_ERROR(EJUSTRETURN))
		return status;

	// If this is the FAT12/16 root directory and there is no space left we can't do anything.
	// This is because the root directory can not change size.
	if (fatDir->de_StartCluster == MSDOSFSROOT && fatDir->de_fndoffset >= fatDir->de_FileSize) {
		INFORM("root directory is full and cannot be expanded\n");
		return B_UNSUPPORTED;
	}

	// set up a dummy node that will be converted into a direntry
	denode newDirentry;
	memset(&newDirentry, 0, sizeof(newDirentry));
	status = B_FROM_POSIX_ERROR(uniqdosname(fatDir, bsdName.Data(), newDirentry.de_Name));
	if (status != B_OK)
		return status;
	if (is_shortname_legal(newDirentry.de_Name) == false) {
		INFORM("invalid FAT short file name '%s'\n", name);
		RETURN_ERROR(B_UNSUPPORTED);
	}
	newDirentry.de_Attributes = ATTR_ARCHIVE;
	if ((perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		newDirentry.de_Attributes |= ATTR_READONLY;
	newDirentry.de_LowerCase = 0;
	newDirentry.de_StartCluster = 0;
	newDirentry.de_FileSize = 0;
	newDirentry.de_pmp = fatDir->de_pmp;
	newDirentry.de_flag = DE_ACCESS | DE_CREATE | DE_UPDATE;
	timespec timeSpec;
	vfs_timestamp(&timeSpec);
	DETIMES(&newDirentry, &timeSpec, &timeSpec, &timeSpec);

	// write the direntry
	u_long fndoffset = fatDir->de_fndoffset;
		// remember this value, because fatDir->de_fndoffset is liable to change during createde
	rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
	status = B_FROM_POSIX_ERROR(createde(&newDirentry, fatDir, NULL, bsdName.Data()));
	rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
	if (status != B_OK)
		RETURN_ERROR(status);

	// determine the inode number
	u_long newCluster;
	status = B_FROM_POSIX_ERROR(
		pcbmap(fatDir, de_cluster(fatVolume, fndoffset), NULL, &newCluster, NULL));
	if (status != B_OK)
		RETURN_ERROR(status);
	uint32 newOffset = fndoffset;
	if (newCluster != MSDOSFSROOT)
		newOffset = fndoffset % fatVolume->pm_bpcluster;
	ino_t inode = DETOI(fatVolume, newCluster, newOffset);
	status = assign_inode(bsdVolume, &inode);
	if (status != B_OK)
		RETURN_ERROR(status);

	// set up the actual node
	vnode* bsdNode;
	status = _dosfs_read_vnode(bsdVolume, inode, &bsdNode, false);
	if (status != B_OK)
		RETURN_ERROR(status);
	mode_t nodeType = 0;
	if (bsdNode->v_type == VDIR)
		nodeType = S_IFDIR;
	else if (bsdNode->v_type == VREG)
		nodeType = S_IFREG;
	else
		panic("dosfs_create:  unknown node type\n");

	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	cookie->fMode = openMode;
	cookie->fLastSize = fatNode->de_FileSize;
	cookie->fMtimeAtOpen = fatNode->de_MTime;
	cookie->fMdateAtOpen = fatNode->de_MDate;
	cookie->fLastNotification = 0;
	*_cookie = cookie;

	status = publish_vnode(volume, inode, bsdNode, &gFATVnodeOps, nodeType, 0);

	// This is usually done in _dosfs_read_vnode. However, the node wasn't published yet,
	// so it would not have worked there.
	bsdNode->v_cache
		= file_cache_create(fatVolume->pm_dev->si_id, fatNode->de_inode, fatNode->de_FileSize);
	bsdNode->v_file_map
		= file_map_create(fatVolume->pm_dev->si_id, fatNode->de_inode, fatNode->de_FileSize);

	ASSERT(static_cast<ino_t>(fatNode->de_inode) == inode);
	*_newVnodeID = fatNode->de_inode;

	if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0)
		_dosfs_fsync(bsdNode);

	entry_cache_add(volume->id, fatDir->de_inode, name, fatNode->de_inode);
	notify_entry_created(volume->id, fatDir->de_inode, name, fatNode->de_inode);

	cookieDeleter.Detach();

	return B_OK;
}


status_t
dosfs_open(fs_volume* volume, fs_vnode* vnode, int openMode, void** _cookie)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	FUNCTION_START("node %" B_PRIu64 " @ %p, omode %o\n", fatNode->de_inode, bsdNode, openMode);

	*_cookie = NULL;

	if ((openMode & O_NOCACHE) != 0)
		RETURN_ERROR(B_UNSUPPORTED);

	if ((openMode & O_CREAT) != 0) {
		PRINT("dosfs_open called with O_CREAT. call dosfs_create instead!\n");
		return B_BAD_VALUE;
	}

	ReadLocker readLocker;
	WriteLocker writeLocker;
	if ((openMode & O_TRUNC) != 0)
		writeLocker.SetTo(bsdNode->v_vnlock->haikuRW, false);
	else
		readLocker.SetTo(bsdNode->v_vnlock->haikuRW, false);

	// Opening a directory read-only is allowed, although you can't read
	// any data from it.
	if (bsdNode->v_type == VDIR && (openMode & O_RWMASK) != O_RDONLY)
		return B_IS_A_DIRECTORY;
	if ((openMode & O_DIRECTORY) != 0 && bsdNode->v_type != VDIR)
		return B_NOT_A_DIRECTORY;

	if ((bsdVolume->mnt_flag & MNT_RDONLY) != 0 || (fatNode->de_Attributes & ATTR_READONLY) != 0)
		openMode = (openMode & ~O_RWMASK) | O_RDONLY;

	if ((openMode & O_TRUNC) != 0 && (openMode & O_RWMASK) == O_RDONLY)
		return B_NOT_ALLOWED;

	status_t status = _dosfs_access(bsdVolume, bsdNode, open_mode_to_access(openMode));
	if (status != B_OK)
		RETURN_ERROR(status);

	FileCookie* cookie = new(std::nothrow) FileCookie;
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<FileCookie> cookieDeleter(cookie);
	cookie->fMode = openMode;
	cookie->fLastSize = fatNode->de_FileSize;
	cookie->fMtimeAtOpen = fatNode->de_MTime;
	cookie->fMdateAtOpen = fatNode->de_MDate;
	cookie->fLastNotification = 0;
	*_cookie = cookie;

	if ((openMode & O_TRUNC) != 0) {
		rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
		status = B_FROM_POSIX_ERROR(detrunc(fatNode, 0, 0, NOCRED));
		rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
		if (status != B_OK)
			RETURN_ERROR(status);

		writeLocker.Unlock();
		status = file_cache_set_size(bsdNode->v_cache, 0);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	cookieDeleter.Detach();

	return B_OK;
}


static status_t
dosfs_close(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	FUNCTION_START("%p\n", vnode->private_node);

	return B_OK;
}


static status_t
dosfs_free_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	FUNCTION_START("%s (inode %" B_PRIu64 " at %p)\n", fatNode->de_Name, fatNode->de_inode,
		bsdNode);

	ReadLocker readLocker;
	WriteLocker writeLocker;
	bool correctLock = false;
	while (correctLock == false) {
		if ((fatNode->de_flag & (DE_UPDATE | DE_ACCESS | DE_CREATE)) != 0) {
			writeLocker.SetTo(bsdNode->v_vnlock->haikuRW, false);
			if ((fatNode->de_flag & (DE_UPDATE | DE_ACCESS | DE_CREATE)) != 0)
				correctLock = true;
			else
				writeLocker.Unlock();
		} else {
			readLocker.SetTo(bsdNode->v_vnlock->haikuRW, false);
			if ((fatNode->de_flag & (DE_UPDATE | DE_ACCESS | DE_CREATE)) == 0)
				correctLock = true;
			else
				readLocker.Unlock();
		}
	}

	struct timespec timeSpec;
	vfs_timestamp(&timeSpec);
	DETIMES(fatNode, &timeSpec, &timeSpec, &timeSpec);

	FileCookie* fatCookie = reinterpret_cast<FileCookie*>(cookie);
	bool changedSize = fatCookie->fLastSize != fatNode->de_FileSize ? true : false;
	bool changedTime = false;
	if (fatCookie->fMtimeAtOpen != fatNode->de_MTime
		|| fatCookie->fMdateAtOpen != fatNode->de_MDate) {
		changedTime = true;
	}
	if (changedSize || changedTime) {
		notify_stat_changed(volume->id, bsdNode->v_parent, fatNode->de_inode,
			(changedTime ? B_STAT_MODIFICATION_TIME : 0) | (changedSize ? B_STAT_SIZE : 0));
	}

	if ((bsdNode->v_mount->mnt_flag & MNT_SYNCHRONOUS) != 0)
		deupdat(fatNode, 1);

	delete fatCookie;

	return B_OK;
}


status_t
dosfs_read(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, void* buffer,
	size_t* length)
{
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	FileCookie* fatCookie = reinterpret_cast<FileCookie*>(cookie);

	FUNCTION_START("%" B_PRIuSIZE " bytes at %" B_PRIdOFF " (node %" B_PRIu64 " @ %p)\n", *length,
		pos, reinterpret_cast<denode*>(bsdNode->v_data)->de_inode, bsdNode);

	if ((bsdNode->v_type & VDIR) != 0) {
		*length = 0;
		return B_IS_A_DIRECTORY;
	}

	if ((fatCookie->fMode & O_RWMASK) == O_WRONLY) {
		*length = 0;
		RETURN_ERROR(B_NOT_ALLOWED);
	}

	// The userlandfs implementation of file_cache_read seems to rely on the FS to decide
	// when to stop reading - it returns B_BAD_VALUE if called again after EOF has been reached.
#if USER
	if (static_cast<u_long>(pos) >= reinterpret_cast<denode*>(bsdNode->v_data)->de_FileSize) {
		*length = 0;
		return B_OK;
	}
#endif

	RETURN_ERROR(file_cache_read(bsdNode->v_cache, fatCookie, pos, buffer, length));
}


status_t
dosfs_write(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, const void* buffer,
	size_t* length)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	if (pos < 0)
		return B_BAD_VALUE;

	FileCookie* fatCookie = reinterpret_cast<FileCookie*>(cookie);

	if ((fatCookie->fMode & O_RWMASK) == O_RDONLY)
		RETURN_ERROR(B_NOT_ALLOWED);

	WriteLocker locker(bsdNode->v_vnlock->haikuRW);

	FUNCTION_START("%" B_PRIuSIZE " bytes at %" B_PRIdOFF " from buffer at %p (vnode id %" B_PRIu64
		")\n", *length, pos, buffer, fatNode->de_inode);

	size_t origSize = fatNode->de_FileSize;

	switch (bsdNode->v_type) {
		case VREG:
			if ((fatCookie->fMode & O_APPEND) != 0)
				pos = fatNode->de_FileSize;
			break;
		case VDIR:
			return B_IS_A_DIRECTORY;
		default:
			RETURN_ERROR(B_BAD_VALUE);
	}

	// if they've exceeded their filesize limit, tell them about it
	if (pos >= MSDOSFS_FILESIZE_MAX)
		RETURN_ERROR(B_FILE_TOO_LARGE);

	if ((pos + *length) >= MSDOSFS_FILESIZE_MAX)
		*length = static_cast<size_t>(MSDOSFS_FILESIZE_MAX - pos);

	// if we write beyond the end of the file, extend it
	status_t status = B_OK;
	if (pos + (*length) > fatNode->de_FileSize) {
		PRINT("dosfs_write:  extending %" B_PRIu64 " to %" B_PRIdOFF " > file size %lu\n",
			fatNode->de_inode, pos + *length, fatNode->de_FileSize);

		bsdNode->v_resizing = true;
		rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
		status = B_FROM_POSIX_ERROR(deextend(fatNode, static_cast<size_t>(pos) + *length, NOCRED));
		rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
		bsdNode->v_resizing = false;
		// if there is not enough free space to extend as requested, we return here
		if (status != B_OK)
			RETURN_ERROR(status);

		PRINT("setting file size to %lu (%lu clusters)\n", fatNode->de_FileSize,
			de_clcount(fatVolume, fatNode->de_FileSize));
		ASSERT(fatNode->de_FileSize == static_cast<unsigned long>(pos) + *length);
	}

	locker.Unlock();
	status = file_cache_set_size(bsdNode->v_cache, fatNode->de_FileSize);
	if (status == B_OK) {
		status = file_cache_write(bsdNode->v_cache, fatCookie, pos, buffer, length);
		if (status != B_OK) {
			REPORT_ERROR(status);
			status = B_OK;
		}
		if (*length == 0)
			status = B_IO_ERROR;
	}
	if (status != B_OK) {
		// complete write failure
		if (origSize < fatNode->de_FileSize) {
			// return file to its previous size
			int truncFlag = ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0) ? IO_SYNC : 0;
			locker.Lock();
			rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
			status_t undoStatus = B_FROM_POSIX_ERROR(detrunc(fatNode, origSize, truncFlag, NOCRED));
			if (undoStatus != 0)
				REPORT_ERROR(undoStatus);
			rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
			locker.Unlock();
			file_cache_set_size(bsdNode->v_cache, origSize);
		}
		RETURN_ERROR(status);
	}

	// do the zeroing that is disabled in deextend
	if (static_cast<u_long>(pos) > origSize) {
		status = fill_gap_with_zeros(bsdNode, origSize, pos);
		if (status != B_OK)
			REPORT_ERROR(status);
	}

	if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0) {
		status = _dosfs_fsync(bsdNode);
		if (status != B_OK)
			REPORT_ERROR(status);
	}

	if (fatNode->de_FileSize > 0 && fatNode->de_FileSize > fatCookie->fLastSize
		&& system_time() > fatCookie->fLastNotification + INODE_NOTIFICATION_INTERVAL) {
		notify_stat_changed(volume->id, bsdNode->v_parent, fatNode->de_inode,
			B_STAT_MODIFICATION_TIME | B_STAT_SIZE | B_STAT_INTERIM_UPDATE);
		fatCookie->fLastSize = fatNode->de_FileSize;
		fatCookie->fLastNotification = system_time();
	}

	return B_OK;
}


static status_t
dosfs_mkdir(fs_volume* volume, fs_vnode* parent, const char* name, int perms)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	vnode* bsdParent = reinterpret_cast<vnode*>(parent->private_node);
	denode* fatParent = reinterpret_cast<denode*>(bsdParent->v_data);

	FUNCTION_START("%" B_PRIu64 "/%s (perm %o)\n", fatParent->de_inode, name, perms);

	if (is_filename_legal(name) == false)
		RETURN_ERROR(B_BAD_VALUE);

	ComponentName bsdName(ISLASTCN, NOCRED, CREATE, 0, name);

	WriteLocker locker(bsdParent->v_vnlock->haikuRW);

	status_t status = _dosfs_access(bsdVolume, bsdParent, W_OK);
	if (status != B_OK)
		RETURN_ERROR(status);

	if (bsdParent->v_type != VDIR)
		return B_BAD_TYPE;

	bool removed = false;
	status = get_vnode_removed(volume, fatParent->de_inode, &removed);
	if (status == B_OK && removed == true)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// add file type information to perms
	perms &= ~S_IFMT;
	perms |= S_IFDIR;

	// set fatParent::de_fndoffset and de_fndcnt in preparation for createde
	vnode* existingBsdNode;
	status = msdosfs_lookup_ino(bsdParent, &existingBsdNode, bsdName.Data(), NULL, NULL);
	if (status == 0) {
		// a directory with this name already exists
		rw_lock_write_unlock(&existingBsdNode->v_vnlock->haikuRW);
		put_vnode(volume, (reinterpret_cast<denode*>(existingBsdNode->v_data))->de_inode);
		return B_FILE_EXISTS;
	}
	if (status != EJUSTRETURN)
		RETURN_ERROR(B_FROM_POSIX_ERROR(status));

	// If this is the FAT12/16 root directory and there is no space left we can't do anything.
	// This is because the root directory can not change size.
	if (fatParent->de_StartCluster == MSDOSFSROOT
		&& fatParent->de_fndoffset >= fatParent->de_FileSize) {
		INFORM("root directory is full and cannot be expanded\n");
		return B_UNSUPPORTED;
	}

	// allocate a cluster to hold the about to be created directory
	u_long newCluster;
	rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
	status = B_FROM_POSIX_ERROR(clusteralloc(fatVolume, 0, 1, CLUST_EOFE, &newCluster, NULL));
	rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
	if (status != B_OK)
		RETURN_ERROR(status);

	// start setting up a dummy node to convert to the new direntry
	denode newEntry;
	memset(&newEntry, 0, sizeof(newEntry));
	newEntry.de_pmp = fatVolume;
	newEntry.de_flag = DE_ACCESS | DE_CREATE | DE_UPDATE;
	timespec timeSpec;
	vfs_timestamp(&timeSpec);
	DETIMES(&newEntry, &timeSpec, &timeSpec, &timeSpec);

	// Now fill the cluster with the "." and ".." entries. And write the cluster to disk. This
	// way it is there for the parent directory to be pointing at if there were a crash.
	int startBlock = cntobn(fatVolume, newCluster);
	buf* newData = getblk(fatVolume->pm_devvp, startBlock, fatVolume->pm_bpcluster, 0, 0, 0);
	if (newData == NULL) {
		clusterfree(fatVolume, newCluster);
		RETURN_ERROR(B_ERROR);
	}
	// clear out rest of cluster to keep scandisk happy
	memset(newData->b_data, 0, fatVolume->pm_bpcluster);
	memcpy(newData->b_data, &gDirTemplate, sizeof gDirTemplate);
	direntry* childEntries = reinterpret_cast<direntry*>(newData->b_data);
	putushort(childEntries[0].deStartCluster, newCluster);
	putushort(childEntries[0].deCDate, newEntry.de_CDate);
	putushort(childEntries[0].deCTime, newEntry.de_CTime);
	childEntries[0].deCHundredth = newEntry.de_CHun;
	putushort(childEntries[0].deADate, newEntry.de_ADate);
	putushort(childEntries[0].deMDate, newEntry.de_MDate);
	putushort(childEntries[0].deMTime, newEntry.de_MTime);
	u_long parentCluster = fatParent->de_StartCluster;
	// Although the root directory has a non-magic starting cluster number for FAT32, chkdsk and
	// fsck_msdosfs still require references to it in dotdot entries to be magic.
	if (FAT32(fatVolume) == true && parentCluster == fatVolume->pm_rootdirblk)
		parentCluster = MSDOSFSROOT;
	putushort(childEntries[1].deStartCluster, parentCluster);
	putushort(childEntries[1].deCDate, newEntry.de_CDate);
	putushort(childEntries[1].deCTime, newEntry.de_CTime);
	childEntries[1].deCHundredth = newEntry.de_CHun;
	putushort(childEntries[1].deADate, newEntry.de_ADate);
	putushort(childEntries[1].deMDate, newEntry.de_MDate);
	putushort(childEntries[1].deMTime, newEntry.de_MTime);
	if (FAT32(fatVolume) == true) {
		putushort(childEntries[0].deHighClust, newCluster >> 16);
		putushort(childEntries[1].deHighClust, parentCluster >> 16);
	}

	if (DOINGASYNC(bsdParent) == true) {
		bdwrite(newData);
	} else if ((status = B_FROM_POSIX_ERROR(bwrite(newData))) != B_OK) {
		clusterfree(fatVolume, newCluster);
		RETURN_ERROR(status);
	}

	// Now build up a directory entry pointing to the newly allocated cluster. This will be
	// written to an empty slot in the parent directory.
	status = B_FROM_POSIX_ERROR(uniqdosname(fatParent, bsdName.Data(), newEntry.de_Name));
	if (status == B_OK && is_shortname_legal(newEntry.de_Name) == false)
		status = B_UNSUPPORTED;
	if (status != B_OK) {
		clusterfree(fatVolume, newCluster);
		RETURN_ERROR(status);
	}

	newEntry.de_Attributes = ATTR_DIRECTORY;
	// The FAT on-disk direntry is limited in the permissions it can store
	if ((perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
		newEntry.de_Attributes |= ATTR_READONLY;
	newEntry.de_LowerCase = 0;
	newEntry.de_StartCluster = newCluster;
	newEntry.de_FileSize = 0;

	// convert newEntry into a new direntry and write it
	rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
		// lock FAT in case parent must be extended to hold another direntry
	status = B_FROM_POSIX_ERROR(createde(&newEntry, fatParent, NULL, bsdName.Data()));
	rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
	if (status != B_OK) {
		clusterfree(fatVolume, newCluster);
		RETURN_ERROR(status);
	}

	// set up the actual node
	ino_t inode = DETOI(fatVolume, newCluster, 0);
	assign_inode(bsdVolume, &inode);
	vnode* bsdNode;
	status = _dosfs_read_vnode(bsdVolume, inode, &bsdNode);
	if (status != B_OK) {
		clusterfree(fatVolume, newCluster);
		RETURN_ERROR(status);
	}
	// parent is not accessible in _dosfs_read_vnode when the node is a directory.
	bsdNode->v_parent = fatParent->de_inode;

	status = publish_vnode(volume, inode, bsdNode, &gFATVnodeOps, S_IFDIR, 0);
	if (status != B_OK) {
		clusterfree(fatVolume, newCluster);
		RETURN_ERROR(status);
	}

	put_vnode(volume, inode);

	if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0)
		_dosfs_fsync(bsdNode);

	entry_cache_add(volume->id, fatParent->de_inode, name, inode);

	notify_entry_created(volume->id, fatParent->de_inode, name, inode);

	return B_OK;
}


static status_t
dosfs_rmdir(fs_volume* volume, fs_vnode* parent, const char* name)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	vnode* bsdParent = reinterpret_cast<vnode*>(parent->private_node);
	denode* fatParent = reinterpret_cast<denode*>(bsdParent->v_data);

	FUNCTION_START("%s in %" B_PRIu64 " at %p\n", name, fatParent->de_inode, bsdParent);

	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return B_NOT_ALLOWED;

	ComponentName bsdName(ISLASTCN, NOCRED, DELETE, 0, name);

	WriteLocker parentLocker(bsdParent->v_vnlock->haikuRW);

	daddr_t cluster;
	u_long offset;
	status_t status = B_FROM_POSIX_ERROR(
		msdosfs_lookup_ino(bsdParent, NULL, bsdName.Data(), &cluster, &offset));
	if (status != B_OK)
		RETURN_ERROR(status);

	vnode* bsdTarget;
	status = assign_inode_and_get(bsdVolume, cluster, offset, &bsdTarget);
	if (status != B_OK)
		RETURN_ERROR(status);
	WriteLocker targetLocker(bsdTarget->v_vnlock->haikuRW);
	NodePutter targetPutter(bsdTarget);
	denode* fatTarget = reinterpret_cast<denode*>(bsdTarget->v_data);

	if (bsdTarget->v_type != VDIR)
		return B_NOT_A_DIRECTORY;
	if ((bsdTarget->v_vflag & VV_ROOT) != 0)
		return B_NOT_ALLOWED;
	if (dosdirempty(fatTarget) == false)
		return B_DIRECTORY_NOT_EMPTY;

	status = _dosfs_access(bsdVolume, bsdTarget, W_OK);
	if (status != B_OK)
		RETURN_ERROR(status);

	status = B_FROM_POSIX_ERROR(removede(fatParent, fatTarget));
	if (status != B_OK)
		RETURN_ERROR(status);

	// Set the loc to a unique value. This effectively removes it from the vcache without
	// releasing its vnid for reuse. It also nicely reserves the vnid from use by other nodes.
	// This is okay because the vnode is locked in memory after this point and loc will not
	// be referenced from here on.
	status = vcache_set_entry(bsdVolume, fatTarget->de_inode, generate_unique_vnid(bsdVolume));
	if (status != B_OK)
		RETURN_ERROR(status);

	status = remove_vnode(volume, fatTarget->de_inode);
	if (status != B_OK)
		RETURN_ERROR(status);

	targetLocker.Unlock();

	if ((bsdVolume->mnt_flag & MNT_SYNCHRONOUS) != 0)
		_dosfs_sync(bsdVolume, false);

	entry_cache_remove(volume->id, fatTarget->de_inode, "..");
	entry_cache_remove(volume->id, fatParent->de_inode, name);

	notify_entry_removed(volume->id, fatParent->de_inode, name, fatTarget->de_inode);

	return B_OK;
}


static status_t
dosfs_opendir(fs_volume* volume, fs_vnode* vnode, void** _cookie)
{
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	FUNCTION_START("%p\n", bsdNode);

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	*_cookie = NULL;

	if ((bsdNode->v_type & VDIR) == 0)
		return B_NOT_A_DIRECTORY;

	DirCookie* cookie = new(std::nothrow) DirCookie;
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	cookie->fIndex = 0;

	*_cookie = cookie;

	return B_OK;
}


status_t
dosfs_closedir(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	FUNCTION_START("%p\n", vnode->private_node);

	return B_OK;
}


status_t
dosfs_free_dircookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	delete reinterpret_cast<DirCookie*>(cookie);

	return B_OK;
}


static status_t
dosfs_readdir(fs_volume* volume, fs_vnode* vnode, void* cookie, struct dirent* buffer,
	size_t bufferSize, uint32* _num)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdVolume->mnt_data);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	denode* fatNode = reinterpret_cast<denode*>(bsdNode->v_data);

	FUNCTION_START("vp %p(%" B_PRIu64 "), bufferSize %lu, entries to be read %" B_PRIu32 "\n",
		bsdNode, fatNode->de_inode, bufferSize, *_num);

	WriteLocker locker(bsdNode->v_vnlock->haikuRW);

	if ((fatNode->de_Attributes & ATTR_DIRECTORY) == 0)
		RETURN_ERROR(B_NOT_A_DIRECTORY);

	uint32 entriesRequested = *_num;
	*_num = 0;

	// struct dirent is defined differently in Haiku and FreeBSD. In the ported driver,
	// Haiku's definition is the relevant one.
	dirent* dirBuf = reinterpret_cast<dirent*>(alloca(sizeof(struct dirent) + MAXNAMLEN + 1));
	memset(dirBuf, 0, sizeof(struct dirent) + MAXNAMLEN + 1);

	char* byteBuffer = reinterpret_cast<char*>(buffer);

	// If they are reading from the root directory then, we simulate the . and .. entries since
	// these don't exist in the root directory. We also set the offset bias to make up for having
	// to simulate these entries. By this I mean that at file offset 64 we read the first entry in
	// the root directory that lives on disk.

	// directory-relative index of the current direntry or winentry;
	// it is incremented for the simulated . and .. entries in the root directory too
	uint32* entryIndex = &reinterpret_cast<DirCookie*>(cookie)->fIndex;

	int32 bias = 0;
		// disk offset = virtual offset - bias
	if (static_cast<ino_t>(fatNode->de_inode) == root_inode(fatVolume))
		bias += 2 * sizeof(direntry);

	if (*entryIndex * sizeof(direntry) >= fatNode->de_FileSize + bias)
		return B_OK;

	if (static_cast<ino_t>(fatNode->de_inode) == root_inode(fatVolume)) {
		for (; *entryIndex < 2 && *_num < entriesRequested; ++*entryIndex, ++*_num) {
			dirBuf->d_ino = fatNode->de_inode;
			dirBuf->d_dev = volume->id;
			switch (*entryIndex) {
				case 0:
					dirBuf->d_name[0] = '.';
					dirBuf->d_name[1] = '\0';
					break;
				case 1:
					dirBuf->d_name[0] = '.';
					dirBuf->d_name[1] = '.';
					dirBuf->d_name[2] = '\0';
					break;
			}
			dirBuf->d_reclen = GENERIC_DIRSIZ(dirBuf);

			if (bufferSize < dirBuf->d_reclen) {
				if (*_num == 0)
					RETURN_ERROR(B_BUFFER_OVERFLOW)
				else
					return B_OK;
			}

			memcpy(byteBuffer, dirBuf, dirBuf->d_reclen);

			bufferSize -= dirBuf->d_reclen;
			byteBuffer += dirBuf->d_reclen;
		}
	}

	buf* entriesBuf;
		// disk entries being read from
	mbnambuf longName;
		// filename after extraction and conversion from winentries
	mbnambuf_init(&longName);
	int chkSum = -1;
		// checksum of the filename
	int32 winChain = 0;
		// number of consecutive winentries we have found before reaching the corresponding
		// direntry
	bool done = false;
	status_t status = B_OK;
	while (bufferSize > 0 && *_num < entriesRequested && done == false) {
		int32 logicalCluster = de_cluster(fatVolume, (*entryIndex * sizeof(direntry)) - bias);
			// file-relative cluster number containing the next entry to read
		int32 clusterOffset = ((*entryIndex * sizeof(direntry)) - bias) & fatVolume->pm_crbomask;
			// byte offset into buf::b_data at which the inner loop starts reading
		int32 fileDiff = fatNode->de_FileSize - (*entryIndex * sizeof(direntry) - bias);
			// remaining data in the directory file
		if (fileDiff <= 0)
			break;
		int32 bytesLeft
			= min_c(static_cast<int32>(fatVolume->pm_bpcluster) - clusterOffset, fileDiff);
			// remaining data in the struct buf, excluding any area past EOF

		int readSize;
			// how many bytes to read into the struct buf at a time; usually cluster size but
			// 512 bytes for the FAT12/16 root directory
		daddr_t readBlock;
			// volume-relative index of the readSize-sized block into entriesBuf
		size_t volumeCluster;
			// volume-relative cluster number containing the next entry to read
		status = B_FROM_POSIX_ERROR(
			pcbmap(fatNode, logicalCluster, &readBlock, &volumeCluster, &readSize));
		if (status != B_OK)
			break;

		status = B_FROM_POSIX_ERROR(
			bread(fatVolume->pm_devvp, readBlock, readSize, NOCRED, &entriesBuf));
		if (status != B_OK)
			break;

		bytesLeft = min_c(bytesLeft, readSize - entriesBuf->b_resid);
		if (bytesLeft == 0) {
			brelse(entriesBuf);
			status = B_IO_ERROR;
			break;
		}

		// convert from DOS directory entries to FS-independent directory entries
		direntry* fatEntry;
		for (fatEntry = reinterpret_cast<direntry*>(entriesBuf->b_data + clusterOffset);
			reinterpret_cast<char*>(fatEntry) < entriesBuf->b_data + clusterOffset + bytesLeft
			 && *_num < entriesRequested; fatEntry++, (*entryIndex)++) {
			// fatEntry is assumed to point to a struct direntry for now, but it may in fact
			// be a struct winentry; that case will handled below

			// ff this is an unused entry, we can stop
			if (fatEntry->deName[0] == SLOT_EMPTY) {
				done = true;
				break;
			}

			// skip deleted entries
			if (fatEntry->deName[0] == SLOT_DELETED) {
				chkSum = -1;
				mbnambuf_init(&longName);
				continue;
			}

			// handle Win95 long directory entries
			if (fatEntry->deAttributes == ATTR_WIN95) {
				chkSum = win2unixfn(&longName, reinterpret_cast<winentry*>(fatEntry), chkSum,
					fatVolume);
#ifdef DEBUG
				dprintf_winentry(fatVolume, reinterpret_cast<winentry*>(fatEntry), entryIndex);
#endif
				winChain++;
				continue;
			}

			// skip volume labels
			if (fatEntry->deAttributes & ATTR_VOLUME) {
				chkSum = -1;
				mbnambuf_init(&longName);
				continue;
			}

			// Found a direntry. First, populate d_ino.
			ino_t ino;
			if (fatEntry->deAttributes & ATTR_DIRECTORY) {
				u_long entryCluster = getushort(fatEntry->deStartCluster);
				if (FAT32(fatVolume) != 0)
					entryCluster |= getushort(fatEntry->deHighClust) << 16;
				if (entryCluster == MSDOSFSROOT)
					ino = root_inode(fatVolume);
				else
					ino = DETOI(fatVolume, entryCluster, 0);
			} else {
				u_long dirOffset = *entryIndex * sizeof(direntry) - bias;
				if (IS_FIXED_ROOT(fatNode) == 0) {
					// we want a cluster-relative offset
					dirOffset = (dirOffset % fatVolume->pm_bpcluster);
				}
				ino = DETOI(fatVolume, volumeCluster, dirOffset);
			}
			status = assign_inode(bsdVolume, &ino);
			if (status != B_OK)
				break;
			dirBuf->d_ino = ino;

			dirBuf->d_dev = volume->id;

			// Is this direntry associated with a chain of previous winentries?
			if (chkSum != winChksum(fatEntry->deName)) {
				// no, just read the short file name from this direntry
				dos2unixfn(fatEntry->deName, reinterpret_cast<u_char*>(dirBuf->d_name),
					fatEntry->deLowerCase, fatVolume);
				dirBuf->d_reclen = GENERIC_DIRSIZ(dirBuf);
				mbnambuf_init(&longName);
			} else {
				// yes, use the long file name that was assembled from the previous winentry/ies
				mbnambuf_flush(&longName, dirBuf);
			}
			chkSum = -1;

			if (bufferSize < dirBuf->d_reclen) {
				if (*_num == 0) {
					RETURN_ERROR(B_BUFFER_OVERFLOW);
				} else {
					done = true;
					// rewind to the start of the chain of winentries that precedes this direntry
					*entryIndex -= winChain;
					break;
				}
			}
			winChain = 0;

			memcpy(byteBuffer, dirBuf, dirBuf->d_reclen);

			// A single VFS dirent corresponds to 0 or more FAT winentries plus 1 FAT direntry.
			// Iteration code associated with direntries is placed here, instead of in the for
			// loop header, so it won't execute when the for loop continues early
			// after a winentry is found.
			bufferSize -= dirBuf->d_reclen;
			byteBuffer += dirBuf->d_reclen;
			++*_num;
		}
		brelse(entriesBuf);
	}

#ifdef DEBUG
	PRINT("dosfs_readdir returning %" B_PRIu32 " dirents:\n", *_num);
	uint8* printCursor = reinterpret_cast<uint8*>(buffer);
	for (uint32 i = 0; i < *_num; i++) {
		dirent* bufferSlot = reinterpret_cast<dirent*>(printCursor);
		PRINT("buffer offset: %ld, d_dev: %" B_PRIdDEV ", d_ino: %" B_PRIdINO
			", d_name: %s, d_reclen: %d\n", bufferSlot - buffer, bufferSlot->d_dev,
			bufferSlot->d_ino, bufferSlot->d_name, bufferSlot->d_reclen);
		printCursor += bufferSlot->d_reclen;
	}
#endif

	RETURN_ERROR(status);
}


static status_t
dosfs_rewinddir(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	DirCookie* fatCookie = reinterpret_cast<DirCookie*>(cookie);

	FUNCTION_START("%p\n", bsdNode);

	WriteLocker locker(bsdNode->v_vnlock->haikuRW);

	fatCookie->fIndex = 0;

	return B_OK;
}


static status_t
dosfs_open_attrdir(fs_volume* volume, fs_vnode* vnode, void** _cookie)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	FUNCTION_START("%p\n", bsdNode);

	if (_dosfs_access(bsdVolume, bsdNode, O_RDONLY) != B_OK)
		RETURN_ERROR(B_NOT_ALLOWED);

	if ((*_cookie = new(std::nothrow) int32) == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*reinterpret_cast<int32*>(*_cookie) = 0;

	return B_OK;
}


static status_t
dosfs_close_attrdir(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	FUNCTION_START("%p\n", vnode->private_node);

	*reinterpret_cast<int32*>(cookie) = 1;

	return B_OK;
}


static status_t
dosfs_free_attrdir_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	FUNCTION_START("%p\n", vnode->private_node);

	if (cookie == NULL)
		return B_BAD_VALUE;

	delete reinterpret_cast<int32*>(cookie);

	return B_OK;
}


static status_t
dosfs_read_attrdir(fs_volume* volume, fs_vnode* vnode, void* cookie, struct dirent* buffer,
	size_t bufferSize, uint32* _num)
{
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);
	int32* fatCookie = reinterpret_cast<int32*>(cookie);

	FUNCTION_START("%p\n", bsdNode);

	*_num = 0;

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	if ((*fatCookie == 0) && (bsdNode->v_mime != NULL)) {
		*_num = 1;
		strcpy(buffer->d_name, "BEOS:TYPE");
		buffer->d_reclen = offsetof(struct dirent, d_name) + 10;
	}

	*fatCookie = 1;

	return B_OK;
}


static status_t
dosfs_rewind_attrdir(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	FUNCTION_START("%p\n", vnode->private_node);

	if (cookie == NULL)
		return B_BAD_VALUE;

	*reinterpret_cast<int32*>(cookie) = 0;

	return B_OK;
}


static status_t
dosfs_create_attr(fs_volume* volume, fs_vnode* vnode, const char* name, uint32 type, int openMode,
	void** _cookie)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	FUNCTION_START("%p\n", bsdNode);

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	if (_dosfs_access(bsdVolume, bsdNode, open_mode_to_access(openMode)) != B_OK)
		RETURN_ERROR(B_NOT_ALLOWED);

	if (strcmp(name, "BEOS:TYPE") != 0)
		return B_UNSUPPORTED;

	if (bsdNode->v_mime == NULL)
		return B_BAD_VALUE;

	AttrCookie* cookie = new(std::nothrow) AttrCookie;
	cookie->fMode = openMode;
	cookie->fType = FAT_ATTR_MIME;
	*_cookie = cookie;

	return B_OK;
}


static status_t
dosfs_open_attr(fs_volume* volume, fs_vnode* vnode, const char* name, int openMode, void** _cookie)
{
	mount* bsdVolume = reinterpret_cast<mount*>(volume->private_volume);
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	FUNCTION_START("%p\n", bsdNode);

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	if (_dosfs_access(bsdVolume, bsdNode, open_mode_to_access(openMode)) != B_OK)
		RETURN_ERROR(B_NOT_ALLOWED);

	if (strcmp(name, "BEOS:TYPE") != 0)
		return B_UNSUPPORTED;

	if (bsdNode->v_mime == NULL)
		return B_BAD_VALUE;

	AttrCookie* cookie = new(std::nothrow) AttrCookie;
	cookie->fMode = openMode;
	cookie->fType = FAT_ATTR_MIME;
	*_cookie = cookie;

	return B_OK;
}


static status_t
dosfs_close_attr(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	return B_OK;
}


static status_t
dosfs_free_attr_cookie(fs_volume* volume, fs_vnode* vnode, void* cookie)
{
	delete reinterpret_cast<AttrCookie*>(cookie);

	return B_OK;
}


static status_t
dosfs_read_attr(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, void* buffer,
	size_t* length)
{
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	FUNCTION_START("%p\n", bsdNode);

	AttrCookie* fatCookie = reinterpret_cast<AttrCookie*>(cookie);
	if (fatCookie->fType != FAT_ATTR_MIME)
		return B_NOT_ALLOWED;
	if ((fatCookie->fMode & O_RWMASK) == O_WRONLY)
		return B_NOT_ALLOWED;

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	if (bsdNode->v_mime == NULL)
		return B_BAD_VALUE;

	if ((pos < 0) || (pos > static_cast<off_t>(strlen(bsdNode->v_mime))))
		return B_BAD_VALUE;

	ssize_t copied = user_strlcpy(reinterpret_cast<char*>(buffer),
		bsdNode->v_mime + pos, *length);
	if (copied < 0)
		return B_BAD_ADDRESS;

	if (static_cast<size_t>(copied) < *length)
		*length = copied + 1;

	return B_OK;
}


/*! suck up application attempts to set mime types; this hides an unsightly
	error message printed out by zip
*/
static status_t
dosfs_write_attr(fs_volume* volume, fs_vnode* vnode, void* cookie, off_t pos, const void* buffer,
	size_t* length)
{
	FUNCTION_START("%p\n", vnode->private_node);

	AttrCookie* fatCookie = reinterpret_cast<AttrCookie*>(cookie);
	if (fatCookie->fType != FAT_ATTR_MIME)
		return B_NOT_ALLOWED;
	if ((fatCookie->fMode & O_RWMASK) == O_RDONLY)
		return B_NOT_ALLOWED;

	return B_OK;
}


static status_t
dosfs_read_attr_stat(fs_volume* volume, fs_vnode* vnode, void* cookie, struct stat* stat)
{
	struct vnode* bsdNode = reinterpret_cast<struct vnode*>(vnode->private_node);

	FUNCTION_START("%p\n", bsdNode);

	AttrCookie* fatCookie = reinterpret_cast<AttrCookie*>(cookie);
	if (fatCookie->fType != FAT_ATTR_MIME)
		return B_NOT_ALLOWED;
	if ((fatCookie->fMode & O_RWMASK) == O_WRONLY)
		return B_NOT_ALLOWED;

	ReadLocker locker(bsdNode->v_vnlock->haikuRW);

	if (bsdNode->v_mime == NULL)
		return B_BAD_VALUE;

	stat->st_type = B_MIME_STRING_TYPE;
	stat->st_size = strlen(bsdNode->v_mime) + 1;

	return B_OK;
}


status_t
dosfs_initialize(int fd, partition_id partitionID, const char* name, const char* parameterString,
	off_t partitionSize, disk_job_id job)
{
	return _dosfs_initialize(fd, partitionID, name, parameterString, partitionSize, job);
}


status_t
dosfs_uninitialize(int fd, partition_id partitionID, off_t partitionSize, uint32 blockSize,
	disk_job_id job)
{
	return _dosfs_uninitialize(fd, partitionID, partitionSize, blockSize, job);
}


/*! Initialize a FreeBSD-style struct cdev.
	@param _readOnly As input, reflects the user-selected mount options; as output, will be set to
	true if the device is read-only.
*/
static status_t
bsd_device_init(mount* bsdVolume, const dev_t devID, const char* deviceFile, cdev** bsdDevice,
	bool* _readOnly)
{
	cdev* device = new(std::nothrow) cdev;
	if (device == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<cdev> deviceDeleter(device);

	device->si_fd = -1;
	device->si_refcount = 0;
	device->si_mountpt = bsdVolume;
	device->si_name[0] = '\0';
	strncpy(device->si_device, deviceFile, B_PATH_NAME_LENGTH - 1);
	device->si_mediasize = 0;
	device->si_id = devID;

	device->si_geometry = new(std::nothrow) device_geometry;
	if (device->si_geometry == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<device_geometry> geomDeleter(device->si_geometry);

	// open read-only for now
	device->si_fd = open(deviceFile, O_RDONLY | O_NOCACHE);
	if (device->si_fd < 0) {
		if (errno == B_BUSY)
			INFORM("FAT driver does not permit multiple mount points at the same time\n");
		RETURN_ERROR(B_FROM_POSIX_ERROR(errno));
	}

	// get device characteristics
	device_geometry* geometry = device->si_geometry;
	if (ioctl(device->si_fd, B_GET_GEOMETRY, geometry, sizeof(device_geometry)) == -1) {
		// support mounting disk images
		struct stat imageStat;
		if (fstat(device->si_fd, &imageStat) >= 0 && S_ISREG(imageStat.st_mode)) {
			geometry->bytes_per_sector = 0x200;
			geometry->sectors_per_track = 1;
			geometry->cylinder_count = imageStat.st_size / 0x200;
			geometry->head_count = 1;
			geometry->removable = true;
			geometry->read_only = !(imageStat.st_mode & S_IWUSR);
			geometry->write_once = false;
#ifndef FS_SHELL
			geometry->bytes_per_physical_sector = 0x200;
#endif
			device->si_mediasize = imageStat.st_size;
		} else {
			close(device->si_fd);
			RETURN_ERROR(B_FROM_POSIX_ERROR(errno));
		}
	} else {
		device->si_mediasize = 1ULL * geometry->head_count * geometry->cylinder_count
			* geometry->sectors_per_track * geometry->bytes_per_sector;
	}

	if (static_cast<uint64>(device->si_mediasize) > 2ULL << 34) {
		// the driver has not been tested on volumes > 32 GB
		INFORM("The FAT driver does not currently support volumes larger than 32 GB.\n");
		close(device->si_fd);
		return B_UNSUPPORTED;
	}

	if (geometry->bytes_per_sector != 0x200) {
		// FAT is compatible with 0x400, 0x800, and 0x1000 as well, but this driver has not
		// been tested with those values
		INFORM("driver does not yet support devices with > 1 block per sector\n");
		close(device->si_fd);
		return B_UNSUPPORTED;
	}

	if (geometry->read_only) {
		PRINT("%s is read-only\n", deviceFile);
		*_readOnly = true;
	}

	if (*_readOnly == false) {
		// reopen it with read/write permissions
		close(device->si_fd);
		device->si_fd = open(deviceFile, O_RDWR | O_NOCACHE);
		if (device->si_fd < 0)
			RETURN_ERROR(B_FROM_POSIX_ERROR(errno));
	}

	// Prevent multiple simultaneous mounts.
#ifndef FS_SHELL
	status_t status = _kern_lock_node(device->si_fd);
	if (status != B_OK) {
		close(device->si_fd);
		RETURN_ERROR(status);
	}
#endif

	deviceDeleter.Detach();
	geomDeleter.Detach();

	*bsdDevice = device;

	return B_OK;
}


status_t
bsd_device_uninit(cdev* device)
{
	if (device == NULL)
		return B_OK;

	if (device->si_fd >= 0) {
		if (close(device->si_fd) != 0)
			RETURN_ERROR(B_FROM_POSIX_ERROR(errno));
	} else {
		RETURN_ERROR(B_ERROR);
	}

	delete device->si_geometry;

#ifndef FS_SHELL
	_kern_unlock_node(device->si_fd);
#endif // FS_SHELL

	delete device;

	return B_OK;
}


/*! Create a FreeBSD-format vnode representing the device, to simulate a FreeBSD VFS environment
	for the ported driver code.
*/
static status_t
dev_bsd_node_init(cdev* bsdDevice, vnode** devNode)
{
	vnode* node;
	status_t status = B_FROM_POSIX_ERROR(getnewvnode(NULL, bsdDevice->si_mountpt, NULL, &node));
	if (status != B_OK)
		RETURN_ERROR(status);

	// Set up the device node members that are accessed by the driver.
	// We don't give this node any private data.
	node->v_type = VBLK;
	node->v_rdev = bsdDevice;
	node->v_mount = NULL;
	SLIST_INIT(&node->v_bufobj.bo_clusterbufs);
	SLIST_INIT(&node->v_bufobj.bo_fatbufs);
	SLIST_INIT(&node->v_bufobj.bo_emptybufs);
	node->v_bufobj.bo_clusters = 0;
	node->v_bufobj.bo_fatblocks = 0;
	node->v_bufobj.bo_empties = 0;
	rw_lock_init(&node->v_bufobj.bo_lock.haikuRW, "FAT v_bufobj");

	*devNode = node;

	return B_OK;
}


status_t
dev_bsd_node_uninit(vnode* devNode)
{
	if (devNode == NULL)
		return B_OK;

	rw_lock_write_lock(&devNode->v_bufobj.bo_lock.haikuRW);

	// free cluster-size struct bufs:  first b_data, and then the bufs themselves
	buf* listEntry;
	SLIST_FOREACH(listEntry, &devNode->v_bufobj.bo_clusterbufs, link)
	{
		free(listEntry->b_data);
	}
	while (!SLIST_EMPTY(&devNode->v_bufobj.bo_clusterbufs)) {
		listEntry = SLIST_FIRST(&devNode->v_bufobj.bo_clusterbufs);
		SLIST_REMOVE_HEAD(&devNode->v_bufobj.bo_clusterbufs, link);
		free(listEntry);
	}

	// free the FAT-block size bufs
	listEntry = NULL;
	SLIST_FOREACH(listEntry, &devNode->v_bufobj.bo_fatbufs, link)
	{
		free(listEntry->b_data);
	}
	while (!SLIST_EMPTY(&devNode->v_bufobj.bo_fatbufs)) {
		listEntry = SLIST_FIRST(&devNode->v_bufobj.bo_fatbufs);
		SLIST_REMOVE_HEAD(&devNode->v_bufobj.bo_fatbufs, link);
		free(listEntry);
	}

	// free the bufs that were just used as pointers to the block cache
	while (!SLIST_EMPTY(&devNode->v_bufobj.bo_emptybufs)) {
		listEntry = SLIST_FIRST(&devNode->v_bufobj.bo_emptybufs);
		SLIST_REMOVE_HEAD(&devNode->v_bufobj.bo_emptybufs, link);
		free(listEntry);
	}

	rw_lock_destroy(&devNode->v_bufobj.bo_lock.haikuRW);

	free(devNode);

	return B_OK;
}


/*! Further setup will be done later for mnt_data, mnt_stat.f_mntonname, mnt_volentry,
	and mnt_cache.
*/
static status_t
bsd_volume_init(fs_volume* fsVolume, const uint32 flags, mount** volume)
{
	mount* bsdVolume = new(std::nothrow) mount;
	if (bsdVolume == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<mount> volDeleter(bsdVolume);

	bsdVolume->mnt_kern_flag = 0;
	bsdVolume->mnt_flag = 0;
	if ((flags & B_MOUNT_READ_ONLY) != 0)
		bsdVolume->mnt_flag |= MNT_RDONLY;

	bsdVolume->mnt_vfc = new(std::nothrow) vfsconf;
	if ((bsdVolume)->mnt_vfc == NULL)
		return B_NO_MEMORY;
	bsdVolume->mnt_vfc->vfc_typenum = 1;
		// For the port, 1 is arbitrarily assigned as the type of this file system.
		// The member is accessed by the ported FreeBSD code but never used for anything.

	bsdVolume->mnt_stat.f_iosize = FAT_IO_SIZE;

	bsdVolume->mnt_data = NULL;

	bsdVolume->mnt_iosize_max = FAT_IO_SIZE;

	mutex_init(&bsdVolume->mnt_mtx.haikuMutex, "FAT volume");

	bsdVolume->mnt_fsvolume = fsVolume;

	bsdVolume->mnt_volentry = -1;

	if (init_vcache(bsdVolume) != B_OK) {
		mutex_destroy(&(bsdVolume)->mnt_mtx.haikuMutex);
		delete bsdVolume->mnt_vfc;
		return B_ERROR;
	}

	bsdVolume->mnt_cache = NULL;

	*volume = bsdVolume;

	volDeleter.Detach();

	return B_OK;
}


status_t
bsd_volume_uninit(struct mount* volume)
{
	if (volume == NULL)
		return B_OK;

	delete volume->mnt_vfc;

	mutex_destroy(&volume->mnt_mtx.haikuMutex);

	uninit_vcache(volume);

	delete volume;

	return B_OK;
}


/*! Set up a msdosfsmount as mount::mnt_data and initialize the block cache.

*/
static status_t
fat_volume_init(vnode* devvp, mount* bsdVolume, const uint64_t fatFlags, const char* oemPref)
{
	// Read the boot sector of the filesystem, and then check the boot signature.
	// If not a dos boot sector then error out.
	cdev* dev = devvp->v_rdev;

	uint8* bootsectorBuffer = static_cast<uint8*>(calloc(512, sizeof(char)));
	if (bootsectorBuffer == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	MemoryDeleter bootsectorDeleter(bootsectorBuffer);
	if (read(dev->si_fd, bootsectorBuffer, 512) != 512)
		RETURN_ERROR(B_IO_ERROR);

	enum FatType fatType;
	bool dos33;
	status_t status = check_bootsector(bootsectorBuffer, fatType, dos33);
	if (status != B_OK)
		RETURN_ERROR(status);

	msdosfsmount* fatVolume = new(std::nothrow) msdosfsmount;
	if (fatVolume == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<msdosfsmount> volumeDeleter(fatVolume);

	fatVolume->pm_cp = NULL;
		// Not implemented in port

	fatVolume->pm_fsinfo = 0;
	fatVolume->pm_curfat = 0;
	fatVolume->pm_rootdirsize = 0;
	fatVolume->pm_fmod = 0;

	fatVolume->pm_mountp = bsdVolume;
	fatVolume->pm_devvp = devvp;
	fatVolume->pm_odevvp = devvp;
	fatVolume->pm_bo = &devvp->v_bufobj;
	fatVolume->pm_dev = dev;

	fatVolume->pm_flags = 0;

	switch (fatType) {
		case fat12:
			fatVolume->pm_fatmask = FAT12_MASK;
			break;
		case fat16:
			fatVolume->pm_fatmask = FAT16_MASK;
			break;
		case fat32:
			fatVolume->pm_fatmask = FAT32_MASK;
			break;
		default:
			panic("invalid FAT type\n");
	}

	fatVolume->pm_uid = geteuid();
	fatVolume->pm_gid = getegid();

	fatVolume->pm_dirmask = S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR;
	fatVolume->pm_mask = S_IXUSR | S_IXGRP | S_IXOTH | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR;

	// populate those msdosfsmount members that are pulled directly from the BPB
	status = parse_bpb(fatVolume, reinterpret_cast<bootsector*>(bootsectorBuffer), dos33);
	if (status != B_OK)
		RETURN_ERROR(status);

	fatVolume->pm_BlkPerSec = fatVolume->pm_BytesPerSec / DEV_BSIZE;
	if (static_cast<off_t>(fatVolume->pm_HugeSectors * fatVolume->pm_BlkPerSec) * DEV_BSIZE
		> dev->si_mediasize) {
		INFORM("sector count exceeds media size (%" B_PRIdOFF " > %" B_PRIdOFF ")\n",
			static_cast<off_t>(fatVolume->pm_HugeSectors) * fatVolume->pm_BlkPerSec * DEV_BSIZE,
			dev->si_mediasize);
		return B_BAD_VALUE;
	}
	uint8 SecPerClust = fatVolume->pm_bpb.bpbSecPerClust;

	// like FreeBSD, the port uses 512-byte blocks as the primary unit of disk data
	// rather then the device-dependent sector size
	fatVolume->pm_fsinfo *= fatVolume->pm_BlkPerSec;
	fatVolume->pm_HugeSectors *= fatVolume->pm_BlkPerSec;
	fatVolume->pm_HiddenSects *= fatVolume->pm_BlkPerSec;
	fatVolume->pm_FATsecs *= fatVolume->pm_BlkPerSec;
	SecPerClust *= fatVolume->pm_BlkPerSec;

	fatVolume->pm_fatblk = fatVolume->pm_ResSectors * fatVolume->pm_BlkPerSec;

	if (FAT32(fatVolume) == true) {
		fatVolume->pm_fatmult = 4;
		fatVolume->pm_fatdiv = 1;
		fatVolume->pm_firstcluster
			= fatVolume->pm_fatblk + fatVolume->pm_FATs * fatVolume->pm_FATsecs;
	} else {
		fatVolume->pm_curfat = 0;
		fatVolume->pm_rootdirblk
			= fatVolume->pm_fatblk + fatVolume->pm_FATs * fatVolume->pm_FATsecs;
		fatVolume->pm_rootdirsize = howmany(fatVolume->pm_RootDirEnts * sizeof(direntry),
			DEV_BSIZE); // in blocks
		fatVolume->pm_firstcluster = fatVolume->pm_rootdirblk + fatVolume->pm_rootdirsize;
	}

	if (fatVolume->pm_HugeSectors <= fatVolume->pm_firstcluster)
		RETURN_ERROR(B_BAD_VALUE);

	fatVolume->pm_maxcluster
		= (fatVolume->pm_HugeSectors - fatVolume->pm_firstcluster) / SecPerClust + 1;

	if (FAT32(fatVolume) == false) {
		if (fatVolume->pm_maxcluster <= ((CLUST_RSRVD - CLUST_FIRST) & FAT12_MASK)) {
			// This will usually be a floppy disk. This size makes sure that one FAT entry will
			// not be split across multiple blocks.
			fatVolume->pm_fatmult = 3;
			fatVolume->pm_fatdiv = 2;
		} else {
			fatVolume->pm_fatmult = 2;
			fatVolume->pm_fatdiv = 1;
		}
	}

	fatVolume->pm_fatsize = fatVolume->pm_FATsecs * DEV_BSIZE;

	uint32 fatCapacity = (fatVolume->pm_fatsize / fatVolume->pm_fatmult) * fatVolume->pm_fatdiv;
	if (fatVolume->pm_maxcluster >= fatCapacity) {
		INFORM("number of clusters (%ld) exceeds FAT capacity (%" B_PRIu32 ") "
			"(some clusters are inaccessible)\n", fatVolume->pm_maxcluster + 1, fatCapacity);
		fatVolume->pm_maxcluster = fatCapacity - 1;
	}

	if (FAT12(fatVolume) != 0)
		fatVolume->pm_fatblocksize = 3 * 512;
	else
		fatVolume->pm_fatblocksize = DEV_BSIZE;
	fatVolume->pm_fatblocksec = fatVolume->pm_fatblocksize / DEV_BSIZE;
	fatVolume->pm_bnshift = ffs(DEV_BSIZE) - 1;

	// compute mask and shift value for isolating cluster relative byte offsets and cluster
	// numbers from a file offset
	fatVolume->pm_bpcluster = SecPerClust * DEV_BSIZE;
	fatVolume->pm_crbomask = fatVolume->pm_bpcluster - 1;
	fatVolume->pm_cnshift = ffs(fatVolume->pm_bpcluster) - 1;

	// this will be updated later if fsinfo exists
	fatVolume->pm_nxtfree = 3;

	// check for valid cluster size - must be a power of 2
	if ((fatVolume->pm_bpcluster ^ (1 << fatVolume->pm_cnshift)) != 0)
		RETURN_ERROR(B_BAD_VALUE);

	status = check_fat(fatVolume);
	if (status != B_OK)
		RETURN_ERROR(status);

	// check that the partition is large enough to contain the file system
	bool readOnly = (fatFlags & MSDOSFSMNT_RONLY) != 0;
	device_geometry* geometry = dev->si_geometry;
	if (geometry != NULL
		&& fatVolume->pm_HugeSectors
			> geometry->sectors_per_track * geometry->cylinder_count * geometry->head_count) {
		INFORM("dosfs: volume extends past end of partition, mounting read-only\n");
		readOnly = true;
	}

	status = read_label(fatVolume, dev->si_fd, bootsectorBuffer, dev->si_name);
	if (status != B_OK)
		RETURN_ERROR(status);

	// Set up the block cache.
	// If the cached block size is ever changed, functions that work with the block cache
	// will need to be re-examined because they assume a size of 512 bytes
	// (e.g. dosfs_fsync, read_fsinfo, write_fsinfo, sync_clusters, discard_clusters,
	// dosfs_write_fs_stat, and the functions defined in vfs_bio.c).
	bsdVolume->mnt_cache
		= block_cache_create(dev->si_fd, fatVolume->pm_HugeSectors, CACHED_BLOCK_SIZE, readOnly);
	if (bsdVolume->mnt_cache == NULL)
		return B_ERROR;

	status = read_fsinfo(fatVolume, devvp);
	if (status != B_OK) {
		block_cache_delete(bsdVolume->mnt_cache, false);
		return status;
	}

	bsdVolume->mnt_data = fatVolume;

	// allocate memory for the bitmap of allocated clusters, and then fill it in
	fatVolume->pm_inusemap = reinterpret_cast<u_int*>(malloc(
		howmany(fatVolume->pm_maxcluster + 1, N_INUSEBITS) * sizeof(*fatVolume->pm_inusemap)));
	if (fatVolume->pm_inusemap == NULL) {
		block_cache_delete(bsdVolume->mnt_cache, false);
		bsdVolume->mnt_data = NULL;
		RETURN_ERROR(B_NO_MEMORY);
	}
	MemoryDeleter inusemapDeleter(fatVolume->pm_inusemap);

	rw_lock_init(&fatVolume->pm_fatlock.haikuRW, "fatlock");

	// have the inuse map filled in
	rw_lock_write_lock(&fatVolume->pm_fatlock.haikuRW);
	status = B_FROM_POSIX_ERROR(fillinusemap(fatVolume));
	rw_lock_write_unlock(&fatVolume->pm_fatlock.haikuRW);
	if (status != 0) {
		rw_lock_destroy(&fatVolume->pm_fatlock.haikuRW);
		block_cache_delete(bsdVolume->mnt_cache, false);
		bsdVolume->mnt_data = NULL;
		RETURN_ERROR(status);
	}

	// some flags from the FreeBSD driver are not supported in the port
	ASSERT((fatVolume->pm_flags
			   & (MSDOSFSMNT_SHORTNAME | MSDOSFSMNT_LONGNAME | MSDOSFSMNT_NOWIN95 | MSDOSFS_ERR_RO))
		== 0);
	fatVolume->pm_flags |= fatFlags;

	if (readOnly == true) {
		fatVolume->pm_flags |= MSDOSFSMNT_RONLY;
		bsdVolume->mnt_flag |= MNT_RDONLY;
	} else {
		status = B_FROM_POSIX_ERROR(markvoldirty(fatVolume, 1));
		if (status != B_OK) {
			rw_lock_destroy(&fatVolume->pm_fatlock.haikuRW);
			block_cache_delete(bsdVolume->mnt_cache, false);
			bsdVolume->mnt_data = NULL;
			RETURN_ERROR(status);
		}
		fatVolume->pm_fmod = 1;
	}

	status = iconv_init(fatVolume, oemPref);
	if (status != B_OK) {
		rw_lock_destroy(&fatVolume->pm_fatlock.haikuRW);
		block_cache_delete(bsdVolume->mnt_cache, false);
		bsdVolume->mnt_data = NULL;
		RETURN_ERROR(status);
	}

	rw_lock_init(&fatVolume->pm_checkpath_lock.haikuRW, "fat cp");

	volumeDeleter.Detach();
	inusemapDeleter.Detach();

	return B_OK;
}


/*! Clean up the msdosfsmount and the block cache.
	@pre pm_devvp and pm_dev still exist.
*/
status_t
fat_volume_uninit(msdosfsmount* volume)
{
	if (volume == NULL)
		return B_OK;

	status_t status = B_OK;
	if (((volume)->pm_flags & MSDOSFSMNT_RONLY) == 0) {
		rw_lock_write_lock(&volume->pm_fatlock.haikuRW);
		status = B_FROM_POSIX_ERROR(markvoldirty((volume), 0));
		rw_lock_write_unlock(&volume->pm_fatlock.haikuRW);
		if (status != B_OK) {
			markvoldirty((volume), 1);
			REPORT_ERROR(status);
		}
	}

	if ((volume->pm_flags & MSDOSFSMNT_KICONV) != 0 && msdosfs_iconv != NULL) {
		if (volume->pm_w2u != NULL)
			msdosfs_iconv->close(volume->pm_w2u);
		if (volume->pm_u2w != NULL)
			msdosfs_iconv->close(volume->pm_u2w);
		if (volume->pm_d2u != NULL)
			msdosfs_iconv->close(volume->pm_d2u);
		if (volume->pm_u2d != NULL)
			msdosfs_iconv->close(volume->pm_u2d);
		delete msdosfs_iconv;
		msdosfs_iconv = NULL;
	}

	status = write_fsinfo(volume);
	if (status != B_OK)
		REPORT_ERROR(status);

	if (volume->pm_mountp->mnt_cache != NULL) {
		block_cache_delete(volume->pm_mountp->mnt_cache,
			(volume->pm_flags & MSDOSFSMNT_RONLY) == 0);
		volume->pm_mountp->mnt_cache = NULL;
	}

	free(volume->pm_inusemap);

	rw_lock_destroy(&volume->pm_fatlock.haikuRW);
	rw_lock_destroy(&volume->pm_checkpath_lock.haikuRW);

	delete volume;

	return status;
}


static status_t
iterative_io_get_vecs_hook(void* cookie, io_request* request, off_t offset, size_t size,
	struct file_io_vec* vecs, size_t* _count)
{
	vnode* bsdNode = reinterpret_cast<vnode*>(cookie);
	msdosfsmount* fatVolume = reinterpret_cast<msdosfsmount*>(bsdNode->v_mount->mnt_data);

	return file_map_translate(bsdNode->v_file_map, offset, size, vecs, _count,
		fatVolume->pm_bpcluster);
}


static status_t
iterative_io_finished_hook(void* cookie, io_request* request, status_t status, bool partialTransfer,
	size_t bytesTransferred)
{
	vnode* bsdNode = reinterpret_cast<vnode*>(cookie);

	rw_lock_read_unlock(&bsdNode->v_vnlock->haikuRW);

	return B_OK;
}


static uint32
dosfs_get_supported_operations(partition_data* partition, uint32 mask)
{
	FUNCTION();

	return B_DISK_SYSTEM_SUPPORTS_INITIALIZING | B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
		| B_DISK_SYSTEM_SUPPORTS_WRITING;
}


static status_t
dos_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			FUNCTION_START("B_MODULE_INIT\n");
#ifdef _KERNEL_MODE
			add_debugger_command("fat", kprintf_volume, "dump a FAT private volume");
			add_debugger_command("fat_node", kprintf_node, "dump a FAT private node");
#endif // _KERNEL_MODE
			break;

		case B_MODULE_UNINIT:
			FUNCTION_START("B_MODULE_UNINIT\n");
#ifdef _KERNEL_MODE
			remove_debugger_command("fat", kprintf_volume);
			remove_debugger_command("fat_node", kprintf_node);
#endif // _KERNEL_MODE
			break;

		default:
			return B_ERROR;
	}

	return B_OK;
}


fs_volume_ops gFATVolumeOps = {
	&dosfs_unmount,
	&dosfs_read_fs_stat,
	&dosfs_write_fs_stat,
	&dosfs_sync,
	&dosfs_read_vnode,

	// index directory & index operations
	NULL, //&fs_open_index_dir,
	NULL, //&fs_close_index_dir,
	NULL, //&fs_free_index_dir_cookie,
	NULL, //&fs_read_index_dir,
	NULL, //&fs_rewind_index_dir,

	NULL, //&fs_create_index,
	NULL, //&fs_remove_index,
	NULL, //&fs_stat_index,

	// query operations
	NULL, //&fs_open_query,
	NULL, //&fs_close_query,
	NULL, //&fs_free_query_cookie,
	NULL, //&fs_read_query,
	NULL, //&fs_rewind_query,
};


fs_vnode_ops gFATVnodeOps = {
	// vnode operations
	&dosfs_walk,
	NULL, // fs_get_vnode_name,
	&dosfs_release_vnode,
	&dosfs_remove_vnode,

	// VM file access
	&dosfs_can_page,
	&dosfs_read_pages,
	&dosfs_write_pages,

	&dosfs_io,
	NULL, // cancel_io()

	&dosfs_get_file_map,

	NULL, // fs_ioctl()
	NULL, // fs_set_flags,
	NULL, // fs_select
	NULL, // fs_deselect
	&dosfs_fsync,

	NULL, // fs_read_symlink,
	NULL, // fs_create_symlink,

	&dosfs_link,
	&dosfs_unlink,
	&dosfs_rename,

	&dosfs_access,
	&dosfs_rstat,
	&dosfs_wstat,
	NULL, // fs_preallocate,

	// file operations
	&dosfs_create,
	&dosfs_open,
	&dosfs_close,
	&dosfs_free_cookie,
	&dosfs_read,
	&dosfs_write,

	// directory operations
	&dosfs_mkdir,
	&dosfs_rmdir,
	&dosfs_opendir,
	&dosfs_closedir,
	&dosfs_free_dircookie,
	&dosfs_readdir,
	&dosfs_rewinddir,

	// attribute directory operations
	&dosfs_open_attrdir,
	&dosfs_close_attrdir,
	&dosfs_free_attrdir_cookie,
	&dosfs_read_attrdir,
	&dosfs_rewind_attrdir,

	// attribute operations
	&dosfs_create_attr,
	&dosfs_open_attr,
	&dosfs_close_attr,
	&dosfs_free_attr_cookie,
	&dosfs_read_attr,
	&dosfs_write_attr,

	&dosfs_read_attr_stat,
	NULL, // fs_write_attr_stat,
	NULL, // fs_rename_attr,
	NULL, // fs_remove_attr
};


static file_system_module_info sFATBSDFileSystem = {
	{
		"file_systems/fat" B_CURRENT_FS_API_VERSION,
		0,
		dos_std_ops,
	},
	"fat", // short_name
	"FAT32 File System", // pretty_name

	// DDM flags
	0
		//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
		//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
		//	| B_DISK_SYSTEM_SUPPORTS_RESIZING
		//	| B_DISK_SYSTEM_SUPPORTS_MOVING
		//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
		//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
		| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
		| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
		//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING
		//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED
		//	| B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED
		//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED
		//	| B_DISK_SYSTEM_SUPPORTS_RESIZING_WHILE_MOUNTED
		//	| B_DISK_SYSTEM_SUPPORTS_MOVING_WHILE_MOUNTED
		//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED
		//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED
		| B_DISK_SYSTEM_SUPPORTS_WRITING,

	// scanning
	&dosfs_identify_partition,
	&dosfs_scan_partition,
	&dosfs_free_identify_partition_cookie,
	NULL, // free_partition_content_cookie

	&dosfs_mount,

	// capability querying operations
	&dosfs_get_supported_operations,

	NULL, // validate_resize
	NULL, // validate_move
	NULL, // validate_set_content_name
	NULL, // validate_set_content_parameters
	NULL, // validate_initialize

	// shadow partition modification
	NULL, // shadow_changed

	// writing
	NULL, // defragment
	NULL, // repair
	NULL, // resize
	NULL, // move
	NULL, // set_content_name
	NULL, // set_content_parameters
	dosfs_initialize,
dosfs_uninitialize};


module_info* modules[] = {
	reinterpret_cast<module_info*>(&sFATBSDFileSystem),
	NULL,
};
