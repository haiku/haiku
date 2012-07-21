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


//callback function for readdir()
static int _ntfs_dirent_filler(void *_dirent, const ntfschar *name,
	const int name_len, const int name_type, const s64 pos, const MFT_REF mref,
	const unsigned dt_type)
{
	char *filename = NULL;
	dircookie *cookie = (dircookie*)_dirent;

	if (name_type == FILE_NAME_DOS)
		return 0;

	if (MREF(mref) == FILE_root || MREF(mref) >= FILE_first_user
			||	cookie->show_sys_files) {
	 	  if (cookie->readed == 1) {
	 	  	cookie->pos=pos;
	 	  	cookie->readed = 0;
			return -1;
	 	  } else {
			if (ntfs_ucstombs(name, name_len, &filename, 0) >= 0) {
				if (filename) {
					strcpy(cookie->name,filename);
				    cookie->ino=MREF(mref);
				    cookie->readed = 1;
		    		free(filename);
		    		return 0;
				}
			}

			return -1;
		}
	}

	return 0;
}


status_t
fs_free_dircookie(fs_volume *_vol, fs_vnode *vnode, void *cookie)
{
	nspace		*ns = (nspace*)_vol->private_volume;

	LOCK_VOL(ns);
	TRACE("fs_free_dircookie - ENTER\n");
	if (cookie != NULL)
		free(cookie);

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
	int			result = B_NO_ERROR;
	ntfs_inode	*ni = NULL;

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
		cookie->ino = 0;
		cookie->readed = 0;
		cookie->last = 0;
		cookie->name[0] = 0;
		cookie->show_sys_files = ns->show_sys_files;
		*_cookie = (void*)cookie;
	} else
		result = ENOMEM;

exit:
	if (ni)
		ntfs_inode_close(ni);

	TRACE("fs_opendir - EXIT\n");

	UNLOCK_VOL(ns);

	return result;
}


status_t
fs_closedir(fs_volume *_vol, fs_vnode *_node, void *cookie)
{
	nspace		*ns = (nspace*)_vol->private_volume;
	vnode		*node = (vnode*)_node->private_node;
	int			result = B_NO_ERROR;
	ntfs_inode	*ni = NULL;

	LOCK_VOL(ns);

	TRACE("fs_closedir - ENTER\n");

	ni = ntfs_inode_open(ns->ntvol, node->vnid);
	if (ni == NULL) {
			result = ENOENT;
			goto exit;
	}

	fs_ntfs_update_times(_vol, ni, NTFS_UPDATE_ATIME);

exit:
	if (ni)
		ntfs_inode_close(ni);

	TRACE("fs_closedir - EXIT\n");

	UNLOCK_VOL(ns);

	return result;
}


status_t
fs_readdir(fs_volume *_vol, fs_vnode *_node, void *_cookie, struct dirent *buf,
	size_t bufsize, uint32 *num)
{
	nspace		*ns = (nspace*)_vol->private_volume;
	vnode		*node = (vnode*)_node->private_node;
	dircookie	*cookie = (dircookie*)_cookie;
	uint32 		nameLength = bufsize - sizeof(struct dirent), realLen;
	int 		result = B_NO_ERROR;
	ntfs_inode	*ni = NULL;

	LOCK_VOL(ns);

	TRACE("fs_readdir - ENTER (sizeof(buf)=%d, bufsize=%d, num=%d\n",
		sizeof(buf), bufsize, *num);

	if (!ns || !node || !cookie || !num || bufsize < sizeof(*buf)) {
	 	result = EINVAL;
		goto exit;
	}

	if (cookie->readed == 1 || cookie->last == 1) {
		 result = ENOENT;
		 goto exit;
	}

	ni = ntfs_inode_open(ns->ntvol, node->vnid);
	if (ni == NULL) {
		ERROR("fs_readdir - dir not opened\n");
		result = ENOENT;
		goto exit;
	}

	result = ntfs_readdir(ni, &cookie->pos, cookie,
		(ntfs_filldir_t)_ntfs_dirent_filler);

	realLen = nameLength > 255 ? 255 : nameLength;
	buf->d_dev = ns->id;
	buf->d_ino = cookie->ino;
	strlcpy(buf->d_name, cookie->name, realLen + 1);
	buf->d_reclen = sizeof(struct dirent) + realLen;

	if (result == 0)
		cookie->last = 1;

	result = B_NO_ERROR;

	TRACE("fs_readdir - FILE: [%s]\n",buf->d_name);

exit:
	if (ni)
		ntfs_inode_close(ni);

	if (result == B_NO_ERROR)
		*num = 1;
	else
		*num = 0;

	if (result == ENOENT)
		result = B_NO_ERROR;

	TRACE("fs_readdir - EXIT result (%s)\n", strerror(result));

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
		cookie->pos = 0;
		cookie->ino = 0;
		cookie->readed = 0;
		cookie->last = 0;
		cookie->name[0] = 0;
		result = B_NO_ERROR;
	}

	TRACE("fs_rewinddir - EXIT, result is %s\n", strerror(result));

	UNLOCK_VOL(ns);

	return result;
}
