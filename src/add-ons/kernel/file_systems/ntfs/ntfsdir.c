/* ntfsdir.c - directory functions
 *
 * Copyright (c) 2006-2007 Troeglazov Gerasim (3dEyes**)
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

#include "ntfs.h"
#include "attributes.h"
#include "lock.h"
#include "ntfsdir.h"

//callback function for readdir()
static 
int _ntfs_dirent_dot_filler(void *_dirent, const ntfschar *name,
		const int name_len, const int name_type, const s64 pos,
		const MFT_REF mref, const unsigned dt_type)
{
	char *filename = NULL;
	dircookie	*cookie = (dircookie*)_dirent;	
	
	if (name_type == FILE_NAME_DOS)
		return 0;
	
	if (ntfs_ucstombs(name, name_len, &filename, 0) < 0) {
		ERRPRINT("Skipping unrepresentable filename\n");
		return 0;
	}
	
	if(strcmp(filename,".")==0 || strcmp(filename,"..")==0)	{
		if (MREF(mref) == FILE_root || MREF(mref) >= FILE_first_user ||	false) {
	 	  struct direntry *ent = (direntry*)ntfs_calloc(sizeof(direntry));
	 	  ent->name = (char*)ntfs_calloc(strlen(filename)+1);
		  strcpy(ent->name,filename);
		  ent->dev=cookie->dev;
		  ent->ino=MREF(mref);
		  ent->next = NULL;

		  if(cookie->root==NULL) {
		  		cookie->root = ent;
		  		cookie->walk = ent;
		  	}
		  
		  if(cookie->last != NULL)
		     cookie->last->next = ent;

	      cookie->last = ent;		     
	 	 } else { 	 	
			free(filename);
			return -1;
	 	}
	} 	 
	free(filename);
	return 0;
}

static
int _ntfs_dirent_filler(void *_dirent, const ntfschar *name,
		const int name_len, const int name_type, const s64 pos,
		const MFT_REF mref, const unsigned dt_type)
{
	char *filename = NULL;
	dircookie	*cookie = (dircookie*)_dirent;	
	
	if (name_type == FILE_NAME_DOS)
		return 0;
	
	if (ntfs_ucstombs(name, name_len, &filename, 0) < 0) {
		ERRPRINT("Skipping unrepresentable filename\n");
		return 0;
	}
	
	if(strcmp(filename,".")==0 || strcmp(filename,"..")==0)
		return 0;
	
	if (MREF(mref) == FILE_root || MREF(mref) >= FILE_first_user ||	false) {
	 	  struct direntry *ent = (direntry*)ntfs_calloc(sizeof(direntry));
	 	  ent->name = (char*)ntfs_calloc(strlen(filename)+1);
		  strcpy(ent->name,filename);
		  ent->dev=cookie->dev;
		  ent->ino=MREF(mref);
		  ent->next = NULL;

		  if(cookie->root==NULL) {
		  		cookie->root = ent;
		  		cookie->walk = ent;
		  	}
		  
		  if(cookie->last != NULL)
		     cookie->last->next = ent;

	      cookie->last = ent;		     
 	 }
	free(filename);
	return 0;
}


#ifdef __HAIKU__
status_t
fs_free_dircookie( void *_ns, void *node, void *cookie )
#else
int
fs_free_dircookie( void *_ns, void *node, void *cookie )
#endif
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

#ifdef __HAIKU__
status_t
fs_opendir( void *_ns, void *_node, void **_cookie )
#else
int
fs_opendir( void *_ns, void *_node, void **_cookie )
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;
	int			result = B_NO_ERROR;
	ntfs_inode	*ni=NULL;
	dircookie	*cookie = (dircookie*)ntfs_calloc( sizeof(dircookie) );	

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
		cookie->dev = ns->id;
		cookie->pos = 0;
		//cookie->walk_dir = ni;
		cookie->vnid = node->vnid;
		cookie->root = NULL;
		cookie->last = NULL;
		cookie->walk = NULL;
		*_cookie = (void*)cookie;
	} else {
		result = ENOMEM;
	}
exit:
		
	if(ni)
		ntfs_inode_close(ni);		
		
	ERRPRINT("fs_opendir - EXIT\n");
	
	UNLOCK_VOL(ns);
	
	return result;
}

#ifdef __HAIKU__
status_t
fs_closedir( void *_ns, void *node, void *_cookie )
#else
int
fs_closedir( void *_ns, void *node, void *_cookie )
#endif
{
	nspace		*ns = (nspace*)_ns;
	dircookie	*cookie = (dircookie*)_cookie;
	struct 		direntry  	*entry,*entrynext;

	LOCK_VOL(ns);
	
	ERRPRINT("fs_closedir - ENTER\n");
	
	entry=cookie->root;
	if(entry) {
		for(;;) {
	 		entrynext = entry->next;

		 	if(entry->name)
		 		free(entry->name);

		 	if(entry)
		 		free(entry);

		 	entry = entrynext;

		 	if(!entry)
		 		break;
	 }
	}
	
	ERRPRINT("fs_closedir - EXIT\n");
	
	UNLOCK_VOL(ns);
	
	return B_NO_ERROR;
}

#ifdef __HAIKU__
status_t
fs_readdir( void *_ns, void *_node, void *_cookie, struct dirent *buf, size_t bufsize, uint32 *num )
#else
int
fs_readdir(void *_ns, void *_node, void *_cookie, long *num, struct dirent *buf, size_t bufsize)
#endif
{
	int 		result = B_NO_ERROR;
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;
	dircookie	*cookie = (dircookie*)_cookie;
	ntfs_inode	*ni=NULL;
	uint32 		nameLength = bufsize - sizeof(buf) + 1, realLen;
	u64			pos=0;
	
	LOCK_VOL(ns);
	
	ERRPRINT("fs_readdir - ENTER:\n");
	
	if (!ns || !node || !cookie || !num || bufsize < sizeof(buf)) {
	 	result = -1;
		goto quit;
	 }

	if(cookie->pos==0) {
		ni = ntfs_inode_open(ns->ntvol, node->vnid);		
		if(ni==NULL) {
			result = ENOENT;
			goto quit;
		}		
		ntfs_readdir(ni, &pos, cookie, (ntfs_filldir_t)_ntfs_dirent_dot_filler);
		cookie->pos+=2;
	} else {
		if(cookie->pos==2) {
			ni = ntfs_inode_open(ns->ntvol, node->vnid);		
		if(ni==NULL) {
			result = ENOENT;
			goto quit;
		}		
		ntfs_readdir(ni, &pos, cookie, (ntfs_filldir_t)_ntfs_dirent_filler);
		cookie->pos++;
		}
	}
		
	
	if(cookie->root==NULL || cookie->last==NULL) {
		result = -1;
		goto quit;
	 }
	 
	if(cookie->walk==NULL) {
	  	result = ENOENT;
	  	goto quit;
	  }
	
		
	realLen = ( strlen(cookie->walk->name)>=nameLength?(nameLength):(strlen(cookie->walk->name)) )+1;
	buf->d_dev = cookie->walk->dev;
	buf->d_ino = cookie->walk->ino;	
	memcpy(buf->d_name,cookie->walk->name, realLen);
	buf->d_reclen = sizeof(buf) + realLen - 1;
			
	cookie->walk = cookie->walk->next;

		
	ERRPRINT("fs_readdir - FILE: %s\n",buf->d_name);

quit:

	if ( result == B_NO_ERROR )
		*num = 1;
	else
		*num = 0;
	
	if ( result == ENOENT )
		result = B_NO_ERROR;
		
	if(ni)
		ntfs_inode_close(ni);		

	ERRPRINT("fs_readdir - EXIT, result is %s\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;
}
		
#ifdef __HAIKU__			
status_t
fs_rewinddir( void *_ns, void *_node, void *_cookie )
#else
int
fs_rewinddir( void *_ns, void *_node, void *_cookie )
#endif
{
	nspace		*ns = (nspace*)_ns;
	int			result = EINVAL;
	dircookie	*cookie = (dircookie*)_cookie;
	
	LOCK_VOL(ns);

	ERRPRINT("fs_rewinddir - ENTER\n");
	if ( cookie != NULL ) {
		cookie->pos = 0;
		cookie->walk = cookie->root;
		result = B_NO_ERROR;
	}
	ERRPRINT("fs_rewinddir - EXIT, result is %s\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;
}

