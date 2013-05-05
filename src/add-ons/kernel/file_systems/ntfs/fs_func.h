/*
 * Copyright (c) 2000-2004 Anton Altaparmakov
 * Copyright (c) 2002-2006 Szabolcs Szakacsits
 *
 * Copyright (c) 2006 Troeglazov Gerasim (3dEyes**)
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program/include file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program (in the main directory of the Linux-NTFS distribution in the
 * file COPYING); if not, write to the Free Software Foundation,Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef NTFS_FS_FUNC_H
#define NTFS_FS_FUNC_H


#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <KernelExport.h>

#include "attributes.h"
#include "lock.h"
#include "ntfs.h"
#include "volume_util.h"


#define FS_DIR_MODE	S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | \
	S_IWGRP | S_IWOTH
#define FS_FILE_MODE S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | \
	S_IWGRP | S_IWOTH
#define FS_SLNK_MODE S_IFLNK | S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | \
	S_IWGRP | S_IWOTH


extern fs_vnode_ops gNTFSVnodeOps;
extern fs_volume_ops gNTFSVolumeOps;

void 		fs_ntfs_update_times(fs_volume *vol, ntfs_inode *ni, 
				ntfs_time_update_flags mask);

float		fs_identify_partition(int fd, partition_data *partition,
				void **_cookie);
status_t	fs_scan_partition(int fd, partition_data *partition, void *_cookie);
void		fs_free_identify_partition_cookie(partition_data *partition,
				void *_cookie);

status_t	fs_mount(fs_volume *_vol, const char *device, ulong flags,
				const char *args, ino_t *vnid);
status_t	fs_create_symlink(fs_volume *volume, fs_vnode *dir, const char *name,
				const char *target, int mode);
status_t   	fs_create(fs_volume *volume, fs_vnode *dir, const char *name,
				int omode, int perms, void **_cookie, ino_t *_vnid);
status_t	fs_walk(fs_volume *_vol, fs_vnode *_dir, const char *file,
				ino_t *vnid);
status_t	fs_get_vnode_name(fs_volume *volume, fs_vnode *vnode, char *buffer,
				size_t bufferSize);
status_t	fs_unmount(fs_volume *_vol);
status_t	fs_rfsstat(fs_volume *_vol, struct fs_info *);
status_t  	fs_wfsstat(fs_volume *_vol, const struct fs_info *fss, uint32 mask);
status_t 	fs_sync(fs_volume *_vol);
status_t	fs_read_vnode(fs_volume *_vol, ino_t vnid, fs_vnode *_node,
				int *_type, uint32 *_flags, bool reenter);
status_t	fs_write_vnode(fs_volume *volume, fs_vnode *vnode, bool reenter);
status_t	fs_remove_vnode(fs_volume *volume, fs_vnode *vnode, bool reenter);
status_t	fs_access(fs_volume *volume, fs_vnode *vnode, int mode);
status_t	fs_rstat(fs_volume *volume, fs_vnode *vnode, struct stat *st);
status_t	fs_wstat(fs_volume *volume, fs_vnode *vnode, const struct stat *st,
				uint32 mask);
status_t	fs_open(fs_volume *volume, fs_vnode *vnode, int omode,
				void **cookie);
status_t	fs_close(fs_volume *volume, fs_vnode *vnode, void *cookie);
status_t	fs_free_cookie(fs_volume *volume, fs_vnode *vnode, void *cookie);
status_t	fs_read(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
				void *buf, size_t *len);
status_t	fs_write(fs_volume *volume, fs_vnode *vnode, void *cookie, off_t pos,
				const void *buf, size_t *len);
status_t	fs_mkdir(fs_volume *volume, fs_vnode *parent, const char *name,
				int perms);
status_t 	fs_rmdir(fs_volume *volume, fs_vnode *parent, const char *name);
status_t	fs_opendir(fs_volume *volume, fs_vnode *vnode, void** cookie);
status_t  	fs_readdir(fs_volume *volume, fs_vnode *vnode, void *_cookie,
				struct dirent *buf, size_t bufsize, uint32 *num);
status_t	fs_rewinddir(fs_volume *volume, fs_vnode *vnode, void *cookie);
status_t	fs_closedir(fs_volume *volume, fs_vnode *vnode, void *cookie);
status_t	fs_free_dircookie(fs_volume *volume, fs_vnode *vnode, void *cookie);
status_t 	fs_readlink(fs_volume *volume, fs_vnode *link, char *buf,
				size_t *bufsize);
status_t 	fs_fsync(fs_volume *_vol, fs_vnode *_node);
status_t	fs_rename(fs_volume *volume, fs_vnode *fromDir, const char *fromName,
				fs_vnode *toDir, const char *toName);
status_t	fs_unlink(fs_volume *volume, fs_vnode *dir, const char *name);
status_t	fs_initialize(int fd, partition_id partitionID, const char* name,
				const char* parameterString, off_t partitionSize, disk_job_id job);
uint32		fs_get_supported_operations(partition_data* partition, uint32 mask);

#endif	// NTFS_FS_FUNC_H

