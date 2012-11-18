/* ntfsdir.h - directory functions
 *
 * Copyright (c) 2006 Troeglazov Gerasim (3dEyes**)
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef NTFSDIR_H
#define NTFSDIR_H


#include "ntfs.h"


typedef struct cache_entry {
	struct dirent *ent;
	struct cache_entry *next;
} cache_entry;


typedef struct dircookie {
	u64					pos;
	dev_t				dev_id;
	BOOL 				show_sys_files;
	cache_entry 		*cache_root;
	cache_entry 		*entry;
} dircookie;


status_t	fs_closedir(fs_volume *volume, fs_vnode *vnode, void *cookie);
status_t	fs_free_dircookie(fs_volume *volume, fs_vnode *vnode, void *cookie);
status_t	fs_opendir(fs_volume *volume, fs_vnode *vnode, void** cookie);
status_t  	fs_readdir(fs_volume *volume, fs_vnode *vnode, void *_cookie,
				struct dirent *buf, size_t bufsize, uint32 *num );
status_t	fs_rewinddir(fs_volume *volume, fs_vnode *vnode, void *cookie);

void		fs_ntfs_update_times(fs_volume *vol, ntfs_inode *ni,
				ntfs_time_update_flags mask);

#endif // NTFSDIR_H

