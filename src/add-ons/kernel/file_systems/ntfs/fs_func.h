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
 
#ifndef _fs_FUNC_H_
#define _fs_FUNC_H_ 
 
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <KernelExport.h>
#include <time.h>
#include <malloc.h>

#include "ntfs.h"
#include "attributes.h"
#include "lock.h"
#include "volume_util.h"


#ifdef __HAIKU__

float		fs_identify_partition(int fd, partition_data *partition, void **_cookie);
status_t	fs_scan_partition(int fd, partition_data *partition, void *_cookie);
void		fs_free_identify_partition_cookie(partition_data *partition, void *_cookie);

status_t	fs_mount( mount_id nsid, const char *device, ulong flags, const char *args, void **data, vnode_id *vnid );
status_t	fs_create_symlink(void *_ns, void *_dir, const char *name, const char *target, int mode);
status_t   	fs_create(void *_ns, void *_dir, const char *name, int omode, int perms, void **_cookie, vnode_id *_vnid);
status_t	fs_walk(void *_ns, void *_base, const char *file, vnode_id *vnid,int *_type);
status_t	fs_get_vnode_name(void *_ns, void *_node, char *buffer, size_t bufferSize);
status_t	fs_unmount(void *_ns);
status_t	fs_rfsstat(void *_ns, struct fs_info *);
status_t  	fs_wfsstat(void *_vol, const struct fs_info *fss, uint32 mask);
status_t 	fs_sync(void *_ns);
status_t	fs_read_vnode(void *_ns, vnode_id vnid, void **_node, bool reenter);
status_t	fs_write_vnode(void *_ns, void *_node, bool reenter);
status_t    fs_remove_vnode( void *_ns, void *_node, bool reenter );
status_t	fs_access( void *ns, void *node, int mode );
status_t	fs_rstat(void *_ns, void *_node, struct stat *st);
status_t	fs_wstat(void *_vol, void *_node, const struct stat *st, uint32 mask);
status_t	fs_open(void *_ns, void *_node, int omode, void **cookie);
status_t	fs_close(void *ns, void *node, void *cookie);
status_t	fs_free_cookie(void *ns, void *node, void *cookie);
status_t	fs_read(void *_ns, void *_node, void *cookie, off_t pos, void *buf, size_t *len);
status_t	fs_write(void *ns, void *node, void *cookie, off_t pos, const void *buf, size_t *len);
status_t	fs_mkdir(void *_ns, void *_node, const char *name,	int perms, vnode_id *_vnid);
status_t 	fs_rmdir(void *_ns, void *dir, const char *name);
status_t	fs_opendir(void* _ns, void* _node, void** cookie);
status_t  	fs_readdir( void *_ns, void *_node, void *_cookie, struct dirent *buf, size_t bufsize, uint32 *num );
status_t	fs_rewinddir(void *_ns, void *_node, void *cookie);
status_t	fs_closedir(void *_ns, void *_node, void *cookie);
status_t	fs_free_dircookie(void *_ns, void *_node, void *cookie);
status_t 	fs_readlink(void *_ns, void *_node, char *buf, size_t *bufsize);
status_t 	fs_fsync(void *_ns, void *_node);
status_t    fs_rename(void *_ns, void *_odir, const char *oldname, void *_ndir, const char *newname);
status_t    fs_unlink(void *_ns, void *_node, const char *name);

#else

int			fs_mount(nspace_id nsid, const char *device, ulong flags, void *parms, size_t len, void **data, vnode_id *vnid);
int			fs_unmount(void *_ns);
int			fs_walk(void *_ns, void *_base, const char *file, char **newpath, vnode_id *vnid);
int			fs_read_vnode(void *_ns, vnode_id vnid, char r, void **node);
int			fs_write_vnode(void *_ns, void *_node, char r);
int			fs_rstat(void *_ns, void *_node, struct stat *st);
int			fs_wstat(void *_vol, void *_node, struct stat *st, long mask);
int			fs_open(void *_ns, void *_node, int omode, void **cookie);
int			fs_read(void *_ns, void *_node, void *cookie, off_t pos, void *buf, size_t *len);
int			fs_write(void *ns, void *node, void *cookie, off_t pos, const void *buf, size_t *len);
int			fs_free_cookie(void *ns, void *node, void *cookie);
int			fs_close(void *ns, void *node, void *cookie);
int			fs_access(void *_ns, void *_node, int mode);
int			fs_opendir(void* _ns, void* _node, void** cookie);
int			fs_readdir(void *_ns, void *_node, void *cookie, long *num, struct dirent *buf, size_t bufsize);
int			fs_rewinddir(void *_ns, void *_node, void *cookie);
int			fs_closedir(void *_ns, void *_node, void *cookie);
int			fs_free_dircookie(void *_ns, void *_node, void *cookie);
int			fs_rfsstat(void *_ns, struct fs_info *);
int			fs_wfsstat(void *_vol, struct fs_info *fss, long mask);
int			fs_readlink(void *_ns, void *_node, char *buf, size_t *bufsize);
int			fs_mkdir(void *_ns, void *_node, const char *name,	int perms);
int			fs_rmdir(void *_ns, void *dir, const char *name);
int			fs_remove_vnode( void *_ns, void *_node, char reenter );
int			fs_rename(void *_ns, void *_odir, const char *oldname, void *_ndir, const char *newname);
int			fs_create(void *_ns, void *_dir, const char *name, int omode, int perms, vnode_id *_vnid, void **_cookie);
int			fs_unlink(void *_ns, void *_node, const char *name);
int			fs_fsync(void *_ns, void *_node);
int			fs_sync(void *_ns);
int			fs_create_symlink(void *ns, void *_dir, const char *name, const char *path);

#endif //__HAIKU__

#ifndef _READ_ONLY_
status_t do_unlink(nspace *vol, vnode *dir, const char *name, bool isdir);
#endif

#endif
