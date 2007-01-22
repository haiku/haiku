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

#include <KernelExport.h>
#include <NodeMonitor.h>
#include <fs_interface.h>
#include <fs_cache.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <fs_index.h>
#include <fs_query.h>
#include <fs_volume.h>

#include "ntfs.h"
#include "attributes.h"
#include "lock.h"
#include "ntfsdir.h"

//callback function for readdir()
static	int _ntfs_dirent_filler(void *_dirent, const ntfschar *name,
		const int name_len, const int name_type, const s64 pos,
		const MFT_REF mref, const unsigned dt_type);

static int _ntfs_dirent_filler(void *_dirent, const ntfschar *name,
		const int name_len, const int name_type, const s64 pos,
		const MFT_REF mref, const unsigned dt_type)
{
	char *filename = NULL;
	dircookie	*cookie = (dircookie*)_dirent;	
	
	if (name_type == FILE_NAME_DOS)
		return 0;
		
	if (MREF(mref) == FILE_root || MREF(mref) >= FILE_first_user ||	cookie->show_sys_files) {
	 	  if(cookie->readed==1) {
	 	  	cookie->pos=pos;
	 	  	cookie->readed = 0;
			return -1;	 	  	
	 	  } else {
			if (ntfs_ucstombs(name, name_len, &filename, 0) < 0)
				return -1;			
			strcpy(cookie->name,filename);
		    cookie->ino=MREF(mref);
		    cookie->readed = 1;
		    return 0;
		  }
 	 }
	return 0;
}


status_t
fs_free_dircookie( void *_ns, void *node, void *cookie )
{
	nspace		*ns = (nspace*)_ns;
	
	LOCK_VOL(ns);
	ERRPRINT("fs_free_dircookie - ENTER\n");
	if ( cookie != NULL )
		free( cookie );

	ERRPRINT("fs_free_dircookie - EXIT\n");

	UNLOCK_VOL(ns);

	return B_NO_ERROR;
}


status_t
fs_opendir( void *_ns, void *_node, void **_cookie )
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;
	dircookie	*cookie = (dircookie*)ntfs_calloc( sizeof(dircookie) );	
	int			result = B_NO_ERROR;
	ntfs_inode	*ni=NULL;
		
	LOCK_VOL(ns);

	ERRPRINT("fs_opendir - ENTER\n"); 
	
	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
			result = ENOENT;
			goto exit;
	}	

	if (!(ni->mrec->flags & MFT_RECORD_IS_DIRECTORY)) {
		result = EMFILE;
		goto exit;
	}

	if ( cookie != NULL ) {
		cookie->pos = 0;	
		cookie->ino = 0;	
		cookie->readed = 0;	
		cookie->name[0] = 0;
		cookie->show_sys_files = ns->show_sys_files;
		*_cookie = (void*)cookie;
	}
	else
		result = ENOMEM;
exit:		
	if(ni)
		ntfs_inode_close(ni);	
		
	ERRPRINT("fs_opendir - EXIT\n");

	UNLOCK_VOL(ns);

	return result;
}

status_t
fs_closedir( void *_ns, void *node, void *_cookie )
{
	nspace		*ns = (nspace*)_ns;	

	LOCK_VOL(ns);
	
	ERRPRINT("fs_closedir - ENTER\n");
		
	ERRPRINT("fs_closedir - EXIT\n");
	
	UNLOCK_VOL(ns);
	
	return B_NO_ERROR;
}


status_t
fs_readdir( void *_ns, void *_node, void *_cookie, struct dirent *buf, size_t bufsize, uint32 *num )
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;
	dircookie	*cookie = (dircookie*)_cookie;
	uint32 		nameLength = bufsize - sizeof(buf) + 1, realLen;
	int 		result = B_NO_ERROR;
	ntfs_inode	*ni=NULL;
	
	LOCK_VOL(ns);
	
	ERRPRINT("fs_readdir - ENTER\n");
	
	if (!ns || !node || !cookie || !num || bufsize < sizeof(buf)) {
	 	result = EINVAL;
		goto exit;
	 }
	
	if(cookie->readed == 1) {
		 result = ENOENT;
		 goto exit;
	 }
	 
	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
		result = ENOENT;
		goto exit;
	}	 
		
	result = ntfs_readdir(ni, &cookie->pos, cookie, (ntfs_filldir_t)_ntfs_dirent_filler);		
	if(result==0) {
		realLen = nameLength>255?255:nameLength;
		buf->d_dev = ns->id;
		buf->d_ino = cookie->ino;	
		memcpy(buf->d_name,cookie->name, realLen+1);
		buf->d_reclen = sizeof(buf) + realLen - 1;
		result = B_NO_ERROR;
	}
	else
		result = ENOENT;	
			
	ERRPRINT("fs_readdir - FILE: [%s]\n",buf->d_name);

exit:
	if(ni)
		ntfs_inode_close(ni);
		
	if ( result == B_NO_ERROR )
		*num = 1;
	else
		*num = 0;
	
	if ( result == ENOENT )
		result = B_NO_ERROR;

	ERRPRINT("fs_readdir - EXIT result (%s)\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;
}
			
status_t
fs_rewinddir( void *_ns, void *_node, void *_cookie )
{
	nspace		*ns = (nspace*)_ns;
	dircookie	*cookie = (dircookie*)_cookie;
	int			result = EINVAL;

	LOCK_VOL(ns);

	ERRPRINT("fs_rewinddir - ENTER\n");
	if ( cookie != NULL ) {
		cookie->pos = 0;	
		cookie->ino = 0;	
		cookie->readed = 0;	
		cookie->name[0] = 0;
		result = B_NO_ERROR;
	}
	ERRPRINT("fs_rewinddir - EXIT, result is %s\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;
}

