/* ntfsdir.c - directory functions
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


#include "ntfsdir.h"

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


static int
_ntfs_dirent_filler(void *_dirent, const ntfschar *name,
	const int name_len, const int name_type, const s64 pos, const MFT_REF mref,
	const unsigned dt_type)
{
	char *filename = NULL;
	dircookie *cookie = (dircookie*)_dirent;

	if (name_type == FILE_NAME_DOS)
		return 0;

	if (MREF(mref) == FILE_root || MREF(mref) >= FILE_first_user
		|| cookie->show_sys_files) {
		int len = ntfs_ucstombs(name, name_len, &filename, 0);
		if (len >= 0 && filename != NULL) {			 
			cache_entry* new_entry =
				(cache_entry*)ntfs_calloc(sizeof(cache_entry));
			if (new_entry == NULL) {
				free(filename);
				return -1;
			}
			
			new_entry->ent = 
				(struct dirent*)ntfs_calloc(sizeof(struct dirent) + len);
			new_entry->ent->d_dev = cookie->dev_id;
			new_entry->ent->d_ino = MREF(mref);
			memcpy(new_entry->ent->d_name,filename, len + 1);
			new_entry->ent->d_reclen =  sizeof(struct dirent) + len;
			
			if(cookie->cache_root == NULL || cookie->entry == NULL) {
				cookie->cache_root = new_entry;
				cookie->entry = cookie->cache_root;
				cookie->entry->next = NULL;
			} else {
				cookie->entry->next = (void*)new_entry;
				cookie->entry = cookie->entry->next;
				cookie->entry->next = NULL;
			}

		   	free(filename);
		   	return 0;
		}
		return -1;
	}
	return 0;
}


status_t
fs_free_dircookie(fs_volume *_vol, fs_vnode *vnode, void *_cookie)
{
	nspace		*ns = (nspace*)_vol->private_volume;
	dircookie	*cookie = (dircookie*)_cookie;
	
	LOCK_VOL(ns);	
	TRACE("fs_free_dircookie - ENTER\n");
	
	if (cookie != NULL) {
		cache_entry *entry = cookie->cache_root;
		for(;entry!=NULL;) {
			cache_entry *next = entry->next;
			if(entry->ent != NULL)
				free(entry->ent);
			free(entry);
			entry = next;
		}
		free(cookie);
	}

	TRACE("fs_free_dircookie - EXIT\n");
	UNLOCK_VOL(ns);

	return B_NO_ERROR;
}


status_t
fs_opendir(fs_volume *_vol, fs_vnode *_node, void** _cookie)
{
	nspace		*ns = (nspace*)_vol->private_volume;
	vnode		*node = (vnode*)_node->private_node;
	dircookie	*cookie = NULL;
	ntfs_inode	*ni = NULL;
	int			result = B_NO_ERROR;

	LOCK_VOL(ns);
	TRACE("fs_opendir - ENTER\n");

	ni = ntfs_inode_open(ns->ntvol, node->vnid);
	if (ni == NULL) {
		result = ENOENT;
		goto exit;
	}

	if (!(ni->mrec->flags & MFT_RECORD_IS_DIRECTORY)) {
		result = EMFILE;
		goto exit;
	}

	cookie = (dircookie*)ntfs_calloc(sizeof(dircookie));
	if (cookie != NULL) {
		cookie->pos = 0;
		cookie->dev_id = ns->id;
		cookie->show_sys_files = ns->show_sys_files;
		cookie->cache_root = NULL;
		cookie->entry = cookie->cache_root;
		*_cookie = (void*)cookie;
	} else
		result = ENOMEM;

exit:
	if (ni != NULL)
		ntfs_inode_close(ni);

	TRACE("fs_opendir - EXIT\n");
	UNLOCK_VOL(ns);

	return result;
}


status_t
fs_closedir(fs_volume *_vol, fs_vnode *_node, void *cookie)
{
	return B_NO_ERROR;
}


status_t
fs_readdir(fs_volume *_vol, fs_vnode *_node, void *_cookie, struct dirent *buf,
	size_t bufsize, uint32 *num)
{
	nspace		*ns = (nspace*)_vol->private_volume;
	vnode		*node = (vnode*)_node->private_node;
	dircookie	*cookie = (dircookie*)_cookie;
	ntfs_inode	*ni = NULL;

	uint32 		nameLength = bufsize - sizeof(struct dirent), realLen;
	int 		result = B_NO_ERROR;

	LOCK_VOL(ns);
	TRACE("fs_readdir - ENTER (sizeof(buf)=%d, bufsize=%d, num=%d\n",
		sizeof(buf), bufsize, *num);

	if (!ns || !node || !cookie || !num || bufsize < sizeof(*buf)) {
	 	result = EINVAL;
		goto exit;
	}

	ni = ntfs_inode_open(ns->ntvol, node->vnid);
	if (ni == NULL) {
		TRACE("fs_readdir - dir not opened\n");
		result = ENOENT;
		goto exit;
	}

	if(cookie->cache_root == NULL) {
		cookie->entry = NULL;
		result = ntfs_readdir(ni, &cookie->pos, cookie,
			(ntfs_filldir_t)_ntfs_dirent_filler);
		cookie->entry = cookie->cache_root;
		if(result) {
			result = ENOENT;
			goto exit;			
		}			
	}	

	if(cookie->entry == NULL) {
		result = ENOENT;
		goto exit;
	}
	
	if(cookie->entry->ent == NULL) {
		result = ENOENT;
		goto exit;
	}	
	
	realLen = nameLength > 255 ? 255 : nameLength;

	buf->d_dev = ns->id;
	buf->d_ino = cookie->entry->ent->d_ino;
	strlcpy(buf->d_name, cookie->entry->ent->d_name, realLen + 1);
	buf->d_reclen = sizeof(struct dirent) + realLen;
	
	cookie->entry = (cache_entry*)cookie->entry->next;

	TRACE("fs_readdir - FILE: [%s]\n",buf->d_name);

exit:
	if (ni != NULL)
		ntfs_inode_close(ni);

	if (result == B_NO_ERROR)
		*num = 1;
	else
		*num = 0;

	if (result == ENOENT)
		result = B_NO_ERROR;

	TRACE("fs_readdir - EXIT num=%d result (%s)\n",*num, strerror(result));
	UNLOCK_VOL(ns);

	return result;
}


status_t
fs_rewinddir(fs_volume *_vol, fs_vnode *vnode, void *_cookie)
{
	nspace		*ns = (nspace*)_vol->private_volume;
	dircookie	*cookie = (dircookie*)_cookie;
	int			result = EINVAL;

	LOCK_VOL(ns);
	TRACE("fs_rewinddir - ENTER\n");
	
	if (cookie != NULL) {
		cache_entry *entry = cookie->cache_root;
		for(;entry!=NULL;) {
			cache_entry *next = entry->next;
			if(entry->ent != NULL)
				free(entry->ent);
			free(entry);
			entry = next;
		}
		cookie->pos = 0;
		cookie->dev_id = ns->id;
		cookie->show_sys_files = ns->show_sys_files;
		cookie->cache_root = NULL;
		cookie->entry = cookie->cache_root;
		result = B_NO_ERROR;
	}

	TRACE("fs_rewinddir - EXIT, result is %s\n", strerror(result));
	UNLOCK_VOL(ns);

	return result;
}
