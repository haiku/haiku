/*
 * Copyright (c) 2000-2004 Anton Altaparmakov
 * Copyright (c) 2002-2006 Szabolcs Szakacsits
 *
 * Copyright (c) 2006-2007 Troeglazov Gerasim (3dEyes**)
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
#include <driver_settings.h>

#include "ntfs.h"
#include "attributes.h"
#include "lock.h"
#include "volume_util.h"
#include "fs_func.h"

#ifdef __HAIKU__

typedef struct identify_cookie {
	NTFS_BOOT_SECTOR boot;
} identify_cookie;

float
fs_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	NTFS_BOOT_SECTOR boot;
	identify_cookie *cookie;
	uint8			*buf=(uint8*)&boot;
	
	// read in the boot sector
	ERRPRINT("fs_identify_partition: read in the boot sector\n");
	if (read_pos(fd, 0, (void*)&boot, 512) != 512) {
		return -1;
	}

	// only check boot signature on hard disks to account for broken mtools
	// behavior
	
	// check boot signature
	if (((buf[0x1fe] != 0x55) || (buf[0x1ff] != 0xaa)) && (buf[0x15] == 0xf8))
		return -1;

	//check boot signature NTFS
	if (memcmp(buf+3, "NTFS    ", 8)!=0)
		return -1;

	//allocate identify_cookie
	cookie = (identify_cookie *)malloc(sizeof(identify_cookie));
	if (!cookie)
		return -1;

	memcpy(&(cookie->boot),&boot,512);

	*_cookie = cookie;

	return 0.8f;
}

status_t
fs_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = sle64_to_cpu(cookie->boot.number_of_sectors) * le16_to_cpu(cookie->boot.bpb.bytes_per_sector);
	partition->block_size = le16_to_cpu(cookie->boot.bpb.bytes_per_sector);
	partition->content_name = strdup("NTFS Volume");
	return B_OK;
}

void
fs_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;
	free(cookie);
}
#endif //__HAIKU__

#ifdef __HAIKU__
status_t 
fs_mount( mount_id nsid, const char *device, ulong flags, const char *args, void **data, vnode_id *vnid )
#else
int 
fs_mount(nspace_id nsid, const char *device, ulong flags, void *parms, size_t len, void **data, vnode_id *vnid)
#endif
{
	nspace		*ns;
	vnode		*newNode = NULL;
	char 		lockname[32];
	void 		*handle;	
	unsigned long mnt_flags = 0;	
	status_t	result = B_NO_ERROR;

	ERRPRINT("fs_mount - ENTER\n");
				
	ns = ntfs_malloc(sizeof(nspace));
	if (!ns) {
		result = ENOMEM;
		goto	exit;
	}
		
			
	*ns = (nspace) {
		.state = NF_FreeClustersOutdate | NF_FreeMFTOutdate,
		.show_sys_files = false,
		.ro = false
	};
	
	ns->flags = flags;
		
	strcpy(ns->devicePath,device);
		
	sprintf(lockname, "ntfs_lock %lx", ns->id);
	
#ifdef __HAIKU__	
	if ((result = recursive_lock_init(&(ns->vlock), lockname)) != 0)
#else
	if ((result = new_lock(&(ns->vlock), lockname)) != B_OK)
#endif		
	{
			ERRPRINT("fs_mount - error creating lock (%s)\n", strerror(result));
			goto exit;
	}
	
	handle = load_driver_settings("ntfs");
	ns->show_sys_files = ! (strcasecmp(get_driver_parameter(handle, "hide_sys_files", "true", "true"), "true") == 0);
	ns->ro = strcasecmp(get_driver_parameter(handle, "read_only", "false", "false"), "false") != 0;
	ns->noatime = strcasecmp(get_driver_parameter(handle, "no_atime", "true", "true"), "true") == 0;
	unload_driver_settings(handle);
	
	if (ns->ro || ns->flags & B_MOUNT_READ_ONLY) {
		mnt_flags |= MS_RDONLY;
		ns->flags |= B_MOUNT_READ_ONLY;
	}
	if (ns->noatime)
		mnt_flags |= MS_NOATIME;	
		
	ns->ntvol=utils_mount_volume(device,mnt_flags,true);
	if(ns->ntvol!=NULL)
		result = B_NO_ERROR;
	else
		result = errno;
				
	if (result == B_NO_ERROR) {
		*vnid = FILE_root;
		*data = (void*)ns;
		ns->id = nsid;
		
		newNode = (vnode*)ntfs_calloc( sizeof(vnode) );
		if ( newNode == NULL )
			result = ENOMEM;
		else {
			
			newNode->vnid = *vnid;
			newNode->parent_vnid = -1;
			
			result = publish_vnode( nsid, *vnid, (void*)newNode );
			if ( result != B_NO_ERROR )	{
				free( ns );
				result = EINVAL;
				goto	exit;
			}
			else {
				result = B_NO_ERROR;
				ntfs_mark_free_space_outdated(ns);	
				ntfs_calc_free_space(ns);				
			}			
		}
	}
		
exit:

	ERRPRINT("fs_mount - EXIT, result code is %s\n", strerror(result));
	
	return result;
}

#ifdef __HAIKU__
status_t 
fs_unmount(void *_ns)
#else
int
fs_unmount(void *_ns)
#endif
{
	nspace		*ns = (nspace*)_ns;
	status_t	result = B_NO_ERROR;
	
	ERRPRINT("fs_unmount - ENTER\n");
	
	ntfs_device_umount( ns->ntvol, true );

#ifdef __HAIKU__	
	recursive_lock_destroy(&(ns->vlock));
#else
	free_lock(&(ns->vlock));	
#endif	
	
	free( ns );

	ERRPRINT("fs_unmount - EXIT, result is %s\n", strerror(result));
	
	return result;
}

#ifdef __HAIKU__
status_t
fs_rfsstat( void *_ns, struct fs_info * fss )
#else
int
fs_rfsstat( void *_ns, struct fs_info * fss )
#endif
{
	nspace	*ns = (nspace*)_ns;
	int 	i;
	
	LOCK_VOL(ns);

	ERRPRINT("fs_rfsstat - ENTER\n");
	
	ntfs_calc_free_space(ns);
		
	fss->dev = ns->id;
	fss->root = FILE_root;
	fss->flags = B_FS_IS_PERSISTENT|B_FS_HAS_MIME|B_FS_HAS_ATTR;
	fss->block_size = ns->ntvol->cluster_size;
	fss->io_size = 65536;
	fss->total_blocks = ns->ntvol->nr_clusters;
	fss->free_blocks = ns->free_clusters;
	strncpy( fss->device_name, ns->devicePath, sizeof(fss->device_name));
	strncpy( fss->volume_name, ns->ntvol->vol_name, sizeof(fss->volume_name) );
	
	for (i = strlen(fss->volume_name)-1; i >=0 ; i--)
		if (fss->volume_name[i] != ' ')
			break;	
	if (i < 0)
		strcpy(fss->volume_name, "NTFS Untitled");
	else
		fss->volume_name[i + 1] = 0;
					
	strcpy( fss->fsh_name, "NTFS" );

	ERRPRINT("fs_rfsstat - EXIT\n");

	UNLOCK_VOL(ns);

	return B_NO_ERROR;
}

#ifdef __HAIKU__
status_t
fs_wfsstat(void *_vol, const struct fs_info * fss, uint32 mask)
#else
int
fs_wfsstat(void *_vol, struct fs_info *fss, long mask)
#endif
{
	nspace* 	ns = (nspace*)_vol;
	status_t	result = B_NO_ERROR;

	if (ns->flags & B_FS_IS_READONLY) {
		ERRPRINT("ntfs is read-only\n");
		return EROFS;
	}

	LOCK_VOL(ns);

#ifdef __HAIKU__
	if (mask & FS_WRITE_FSINFO_NAME) {	
#else
	if (mask & WFSSTAT_NAME) {
#endif		
		result = ntfs_change_label( ns->ntvol,  (char*)fss->volume_name );
		goto exit;	
	}
	
exit:

	UNLOCK_VOL(ns);

	return result;
}

#ifdef __HAIKU__
status_t
fs_walk(void *_ns, void *_base, const char *file, vnode_id *vnid,int *_type)
#else
int
fs_walk(void *_ns, void *_base, const char *file, char **newpath, vnode_id *vnid)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*baseNode = (vnode*)_base;
	vnode		*newNode = NULL;
	ntfschar 	*unicode = NULL;
	ntfs_inode 	*bi = NULL;
	status_t	result = B_NO_ERROR;
	int			len;

	LOCK_VOL(ns);

	ERRPRINT("fs_walk - ENTER : find for \"%s\"\n",file);

	if(ns == NULL || _base == NULL || file == NULL || vnid == NULL) {
	  result = EINVAL;
	  goto	exit;
	 }

	if ( !strcmp( file, "." ) )	{
		*vnid = baseNode->vnid;
		if ( get_vnode( ns->id, *vnid, (void**)&newNode ) != 0 )
			result = ENOENT;
	} else if ( !strcmp( file, ".." ) && baseNode->vnid != FILE_root ) {
		*vnid = baseNode->parent_vnid;
		if ( get_vnode( ns->id, *vnid, (void**)&newNode ) != 0 )
			result = ENOENT;
	} else {
			unicode = ntfs_calloc(MAX_PATH);
			len = ntfs_mbstoucs(file, &unicode, MAX_PATH);
			if (len < 0) {
				result = EILSEQ;
				goto exit;
		 	}
		
			bi = ntfs_inode_open(ns->ntvol, baseNode->vnid);
			if(!bi) {
				result = ENOENT;
				goto exit;
			}
						
			*vnid = MREF( ntfs_inode_lookup_by_name(bi, unicode, len) );
			
			ntfs_inode_close(bi);				
			free(unicode);
		
			if ( *vnid == (u64) -1 ) {
				result = EINVAL;
				goto exit;
			}

			if( get_vnode( ns->id, *vnid, (void**)&newNode ) !=0 )
				result = ENOENT;
				
			if(newNode!=NULL)
				newNode->parent_vnid = baseNode->vnid;				
	}

exit:

	ERRPRINT("fs_walk - EXIT, result is %s\n", strerror(result));

	UNLOCK_VOL(ns);

	return result;
}

#ifdef __HAIKU__
status_t
fs_get_vnode_name(void *_ns, void *_node, char *buffer, size_t bufferSize)
{
	nspace		*ns = (nspace*)_ns;
	vnode   	*node = (vnode*)_node;
	ntfs_inode	*ni=NULL;
	status_t	result = B_NO_ERROR;
	
	char 	path[MAX_PATH];
	char 	*name;	
	
	LOCK_VOL(ns);
	
	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
			result = ENOENT;
			goto exit;
	}	
	
	if(utils_inode_get_name(ni, path, MAX_PATH)==0) {
		result = EINVAL;
		goto	exit;
	}
	name = strrchr(path, '/');
	name++;
	
	strlcpy(buffer, name, bufferSize);
	
exit:

	if(ni)
		ntfs_inode_close(ni);
		
	UNLOCK_VOL(ns);

	return result;
}
#endif



#ifdef __HAIKU__
status_t 
fs_read_vnode(void *_ns, vnode_id vnid, void **node, bool reenter)
#else
int
fs_read_vnode(void *_ns, vnode_id vnid, char reenter, void **node)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*newNode = NULL;
	ntfs_inode	*ni=NULL;
	status_t	result = B_NO_ERROR;
	
	if ( !reenter )
		LOCK_VOL(ns);
	
	ERRPRINT("fs_read_vnode - ENTER\n");
	
	newNode = (vnode*)ntfs_calloc( sizeof(vnode) );
	if ( newNode != NULL ) {
		char *name = NULL;
		
		ni = ntfs_inode_open(ns->ntvol, vnid);		
		if(ni==NULL) {
			result = ENOENT;
			goto exit;
		 }	
		 
		newNode->vnid = vnid;
		newNode->parent_vnid = ntfs_get_parent_ref(ni);
		
		if( ni->mrec->flags & MFT_RECORD_IS_DIRECTORY )
			set_mime(newNode, ".***");
		else {		
			name = (char*)malloc(MAX_PATH);
			if(name!=NULL) {			
				if(utils_inode_get_name(ni, name,MAX_PATH)==1)
					set_mime(newNode, name);
				free(name);
			}
		}		
		
		*node = (void*)newNode;
	}
	else
		result = ENOMEM;
exit:
	
	if(ni)
		ntfs_inode_close(ni);
		
	ERRPRINT("fs_read_vnode - EXIT, result is %s\n", strerror(result));
	
	if ( !reenter )
		UNLOCK_VOL(ns);
	
	return result;
}

#ifdef __HAIKU__
status_t
fs_write_vnode( void *_ns, void *_node, bool reenter )
#else
int
fs_write_vnode( void *_ns, void *_node, char reenter )
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;
	status_t	result = B_NO_ERROR;
	
	if ( !reenter )
		LOCK_VOL(ns);
	
	ERRPRINT("fs_write_vnode - ENTER (%Ld)\n", node->vnid);

	if (node)
		free( node );
	
	ERRPRINT("fs_write_vnode - EXIT\n");
	
	if ( !reenter )
		UNLOCK_VOL(ns);
	
	return result;
}

#ifdef __HAIKU__
status_t
fs_remove_vnode( void *_ns, void *_node, bool reenter )
#else
int
fs_remove_vnode( void *_ns, void *_node, char reenter )
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;
	status_t	result = B_NO_ERROR;
	
	if ( !reenter )
		LOCK_VOL(ns);
	
	ERRPRINT("fs_remove_vnode - ENTER (%Ld)\n", node->vnid);
			
	if(node)
		free(node);
				
	ERRPRINT("fs_remove_vnode - EXIT, result is %s\n", strerror(result));
	
	if ( !reenter )
	 	UNLOCK_VOL(ns);
	
	return result;
}

#ifdef __HAIKU__ 
status_t
fs_rstat( void *_ns, void *_node, struct stat *stbuf )
#else
int
fs_rstat( void *_ns, void *_node, struct stat *stbuf )
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;
	ntfs_inode 	*ni = NULL;
	ntfs_attr 	*na;
	status_t	result = B_NO_ERROR;	
	
	LOCK_VOL(ns);

	ERRPRINT("fs_rstat - ENTER:\n");	
			
	if(ns==NULL || node ==NULL ||stbuf==NULL) {
		result = ENOENT;
		goto exit;		
	}
	
	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
		result = ENOENT;
		goto exit;
	}
	
	stbuf->st_dev = ns->id;
	stbuf->st_ino = MREF(ni->mft_no);
	
	if ( ni->mrec->flags & MFT_RECORD_IS_DIRECTORY ) {
		stbuf->st_mode = S_IFDIR | 0777;
		na = ntfs_attr_open(ni, AT_INDEX_ALLOCATION, NTFS_INDEX_I30, 4);
		if (na) {
			stbuf->st_size = na->data_size;
			ntfs_attr_close(na);
		} else {
			stbuf->st_size = 0;
		}
		stbuf->st_nlink = 1; // Needed for correct find work. 
	} else {
		// Regular or Interix (INTX) file. 
		stbuf->st_mode = S_IFREG;
		stbuf->st_size = ni->data_size;
		stbuf->st_nlink = le16_to_cpu(ni->mrec->link_count);

		na = ntfs_attr_open(ni, AT_DATA, NULL,0);
		if (!na) {
			result = ENOENT;
			goto exit;
		}

		stbuf->st_size = na->data_size;

		if (!ni->flags & FILE_ATTR_HIDDEN) {
			if (na->data_size == 0)
			stbuf->st_mode = S_IFIFO;
		}

		if (na->data_size <= sizeof(INTX_FILE_TYPES) + sizeof(ntfschar) * MAX_PATH && na->data_size >sizeof(INTX_FILE_TYPES)) {
			INTX_FILE *intx_file;

			intx_file = ntfs_malloc(na->data_size);
			if (!intx_file)	{
				result = EINVAL;
				ntfs_attr_close(na);
				goto exit;
			}
			if (ntfs_attr_pread(na, 0, na->data_size,intx_file) != na->data_size) {
				result = EINVAL;
				free(intx_file);
				ntfs_attr_close(na);
				goto exit;
			}
			if (intx_file->magic == INTX_SYMBOLIC_LINK)
				stbuf->st_mode = S_IFLNK;
			free(intx_file);
		}
		ntfs_attr_close(na);
		stbuf->st_mode |= 0666;			
	}

	if (ns->flags & B_FS_IS_READONLY) {
			stbuf->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
	}
	
	stbuf->st_uid = 0;
	stbuf->st_gid = 0;
	stbuf->st_atime = ni->last_access_time;
	stbuf->st_ctime = ni->last_mft_change_time;
	stbuf->st_mtime = ni->last_data_change_time;
	
exit:

	if(ni)
		ntfs_inode_close(ni);
		
	ERRPRINT("fs_rstat - EXIT, result is %s\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;

}

#ifdef __HAIKU__
status_t
fs_wstat(void *_vol, void *_node, const struct stat *st, uint32 mask)
#else
int
fs_wstat(void *_vol, void *_node, struct stat *st, long mask)
#endif
{
	nspace		*ns = (nspace*)_vol;
	vnode		*node = (vnode*)_node;
	ntfs_inode 	*ni = NULL;
	status_t	result = B_NO_ERROR;	
	
	LOCK_VOL(ns);
	
	ERRPRINT("fs_wstat: ENTER\n");
	
	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
			result = ENOENT;
			goto exit;
	}

#ifdef __HAIKU__
	if ( mask & FS_WRITE_STAT_SIZE ) {
#else
	if (mask & WSTAT_SIZE) {
#endif			
		ERRPRINT("fs_wstat: setting file size to %Lx\n", st->st_size);
		
		if ( ni->mrec->flags & MFT_RECORD_IS_DIRECTORY ) {
			result = EISDIR;		
		} else {
		
			ntfs_attr *na = ntfs_attr_open(ni, AT_DATA, NULL, 0);
			if (!na) {
				result = EINVAL;
				goto exit;
			}
						
			ntfs_attr_truncate(na, st->st_size);
			ntfs_attr_close(na);
			
			ntfs_mark_free_space_outdated(ns);

#ifdef __HAIKU__			
			notify_stat_changed(ns->id, MREF(ni->mft_no), mask);
#else
			notify_listener(B_STAT_CHANGED, ns->id, 0, 0, MREF(ni->mft_no), NULL);
#endif			

		}
	}
	
#ifdef __HAIKU__	
	if (mask & FS_WRITE_STAT_MTIME) {
#else	
	if (mask & WSTAT_MTIME) {
#endif	
		ERRPRINT("fs_wstat: setting modification time\n");
		
		ni->last_access_time = st->st_atime;
		ni->last_data_change_time = st->st_mtime;
		ni->last_mft_change_time = st->st_ctime;
#ifdef __HAIKU__			
		notify_stat_changed(ns->id, MREF(ni->mft_no), mask);
#else		
		notify_listener(B_STAT_CHANGED, ns->id, 0, 0, MREF(ni->mft_no), NULL);
#endif		
	}

exit:

	if(ni)
		ntfs_inode_close(ni);
		
	ERRPRINT("fs_wstat: EXIT with (%s)\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;
}


#ifdef __HAIKU__
status_t
fs_sync(void *_ns)
#else
int
fs_sync(void *_ns)
#endif
{
	nspace *ns = (nspace *)_ns;
	
	LOCK_VOL(ns);

	ERRPRINT("fs_sync - ENTER\n");

	ERRPRINT("fs_sync - EXIT\n");

	UNLOCK_VOL(ns);

	return B_NO_ERROR;
}


#ifdef __HAIKU__
status_t
fs_fsync(void *_ns, void *_node)
#else
int
fs_fsync(void *_ns, void *_node)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;
	ntfs_inode 	*ni = NULL;
	status_t	result = B_NO_ERROR;	
	
	LOCK_VOL(ns);
	
	ERRPRINT("fs_fsync: ENTER\n");
	
	if (ns ==NULL || node== NULL) {
		result = ENOENT;
		goto exit;
	}
	
	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
			result = ENOENT;
			goto exit;
	}
		
	ntfs_inode_sync(ni);
	
exit:	

	if(ni)
		ntfs_inode_close(ni);
		
	ERRPRINT("fs_fsync: EXIT\n");
	
	UNLOCK_VOL(ns);
	
	return result;
}

#ifdef __HAIKU__
status_t	
fs_open(void *_ns, void *_node, int omode, void **_cookie)
#else
int
fs_open(void *_ns, void *_node, int omode, void **_cookie)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;
	filecookie	*cookie=NULL;
	ntfs_inode	*ni=NULL;
	ntfs_attr 	*na = NULL;
	status_t	result = B_NO_ERROR;	
	
	LOCK_VOL(ns);
	
	ERRPRINT("fs_open - ENTER\n");
	
	if(node==NULL) {
		result = EINVAL;
		goto	exit;
	}

	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
			result = errno;
			goto exit;
	}	

	if(omode & O_TRUNC) {			
			na = ntfs_attr_open(ni, AT_DATA, NULL, 0);
			if(na) {
				if(ntfs_attr_truncate(na, 0))
					result = errno;
			} else 
				result = errno;
		}
		
	cookie = (filecookie*)ntfs_calloc( sizeof(filecookie) );	

	if ( cookie != NULL )
	{
		cookie->omode = omode;
		*_cookie = (void*)cookie;
	}
	else
		result = ENOMEM;	
		
exit:

	if(na)
		ntfs_attr_close(na);
		
	if(ni)
		ntfs_inode_close(ni);
		
	ERRPRINT("fs_open - EXIT\n");
	
	UNLOCK_VOL(ns);
	
	return result;
}


#ifdef __HAIKU__
status_t   
fs_create(void *_ns, void *_dir, const char *name, int omode, int perms, void **_cookie, vnode_id *_vnid)
#else
int
fs_create(void *_ns, void *_dir, const char *name, int omode, int perms, vnode_id *_vnid, void **_cookie)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*dir = (vnode*)_dir;
	filecookie	*cookie = NULL;
	vnode		*newNode = NULL;	
	ntfs_attr 	*na = NULL;
	ntfs_inode	*ni=NULL;
	ntfs_inode	*bi=NULL;			
	ntfschar 	*uname = NULL;
	status_t	result = B_NO_ERROR;	
	int 		uname_len;

	if (ns->flags & B_FS_IS_READONLY) {
		ERRPRINT("ntfs is read-only\n");
		return EROFS;
	}	
				
	LOCK_VOL(ns);
		
	ERRPRINT("fs_create - ENTER: name=%s\n",name); 
	
	if(_ns==NULL || _dir==NULL) {
		result = EINVAL;
		goto exit;
	}
	
	if (name == NULL || *name == '\0' || strchr(name, '/') || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		result =  EINVAL;
		goto exit;
	}
	
	bi = ntfs_inode_open(ns->ntvol, dir->vnid);		
	if(bi==NULL) {
			result = ENOENT;
			goto exit;
	}
	
	if ( !bi->mrec->flags & MFT_RECORD_IS_DIRECTORY ) {
		result =  EINVAL;
		goto exit;
	}
		
	uname_len = ntfs_mbstoucs(name, &uname, 0);
	if (uname_len < 0) {
		result = EINVAL;
		goto exit;
	}
	
	cookie = (filecookie*)ntfs_calloc( sizeof(filecookie) );	

	if ( cookie != NULL ) {
		cookie->omode = omode;
	} else {
		result = ENOMEM;
		goto	exit;
	}
	
	ni = ntfs_pathname_to_inode(ns->ntvol, bi, name);
	if(ni)	{  //file exist
		*_vnid	= MREF( ni->mft_no );
		if(omode & O_TRUNC) {			
			na = ntfs_attr_open(ni, AT_DATA, NULL, 0);
			if(na) {
				if(ntfs_attr_truncate(na, 0))
					result = errno;
			} else 
				result = errno;
		}
	} else {
		ni = ntfs_create(bi, uname, uname_len, S_IFREG);	

		if (ni)	{
			*_vnid = MREF( ni->mft_no );		
			
			newNode = (vnode*)ntfs_calloc( sizeof(vnode) );
			if(newNode==NULL) {
			 	result=ENOMEM;
			 	goto exit;
			 }
		
			newNode->vnid = *_vnid;
			newNode->parent_vnid = MREF(bi->mft_no);
			set_mime(newNode, name);		

			result = B_NO_ERROR;
			result = publish_vnode(ns->id, *_vnid, (void*)newNode);	
									
			ntfs_mark_free_space_outdated(ns);	

#ifdef __HAIKU__				
			notify_entry_created(ns->id, MREF( bi->mft_no ), name, *_vnid);				
#else			
			notify_listener(B_ENTRY_CREATED, ns->id, MREF( bi->mft_no ), 0, *_vnid, name);		
#endif
			
		} else
			result = errno;
	}	
		
	free(uname);	
			
exit:

	if(result >= B_OK) {
		*_cookie = cookie;
	} else
		free(cookie);
		
	if(na)
		ntfs_attr_close(na);
	if(ni)
		ntfs_inode_close(ni);
	if(bi)
		ntfs_inode_close(bi);
				
	ERRPRINT("fs_create - EXIT, result is %s\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return 		result;
}

#ifdef __HAIKU__
status_t
fs_read( void *_ns, void *_node, void *_cookie, off_t offset, void *buf, size_t *len )
#else
int
fs_read( void *_ns, void *_node, void *_cookie, off_t offset, void *buf, size_t *len )
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;	
	//filecookie *cookie = (filecookie*)_cookie;  //temporary not used	
	ntfs_inode 	*ni = NULL;
	ntfs_attr 	*na = NULL;
	size_t 		size = *len;
	int 		total = 0;
	status_t	result = B_NO_ERROR;
	
	LOCK_VOL(ns);

	ERRPRINT("fs_read - ENTER\n");

	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
			*len = 0;
			result = ENOENT;
			goto exit2;
	}

	if (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY) {
		ERRPRINT(("ntfs_read called on directory.\n"));
		*len = 0;
		result = EISDIR;
		goto exit2;
	}
	
	if(offset < 0) {
		*len = 0;
		result = EINVAL;
		goto exit2;
	}

	na = ntfs_attr_open(ni, AT_DATA, NULL, 0);
	if (!na) {
		*len = 0;
		result = EINVAL;
		goto exit2;
	}
	if (offset + size > na->data_size)
		size = na->data_size - offset;
	while (size) {
		result = ntfs_attr_pread(na, offset, size, buf);
		if (result < (s64)size)
			ntfs_log_error("ntfs_attr_pread returned less bytes than requested.\n");
		if (result <= 0) {
			*len = 0;
			result = EINVAL;
			goto exit;
		}
		size -= result;
		offset += result;
		total += result;
	}
	result = total;
	*len = result;	
exit:

	if (na)
		ntfs_attr_close(na);

exit2:

	if (ni)
		ntfs_inode_close(ni);
		
	UNLOCK_VOL(ns);

	ERRPRINT("fs_read - EXIT, result is %s\n", strerror(result));
	
	return result;
}


#ifdef __HAIKU__
status_t
fs_write( void *_ns, void *_node, void *_cookie, off_t offset, const void *buf, size_t *len )
#else
int
fs_write( void *_ns, void *_node, void *_cookie, off_t offset, const void *buf, size_t *len )
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;	
	filecookie	*cookie = (filecookie*)_cookie;	
	ntfs_inode 	*ni = NULL;
	ntfs_attr 	*na = NULL;
	int 		total = 0;
	size_t 		size=*len;
	status_t	result = B_NO_ERROR;
	
	if (ns->flags & B_FS_IS_READONLY) {
		ERRPRINT("ntfs is read-only\n");
		return EROFS;
	}	
	
	LOCK_VOL(ns);

	ERRPRINT("fs_write - ENTER, offset=%d, len=%d\n",(int)offset, (int)(*len));

	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
			*len = 0;
			result = ENOENT;
			goto exit2;
	}

	if ( ni->mrec->flags & MFT_RECORD_IS_DIRECTORY ) {
		ERRPRINT(("fs_write - called on directory.\n"));
		*len = 0;
		result = EISDIR;
		goto exit2;
	}
	
	if(offset < 0) {
		ERRPRINT(("fs_write - offset < 0.\n"));
		*len = 0;
		result = EINVAL;
		goto exit2;
	}

	na = ntfs_attr_open(ni, AT_DATA, NULL, 0);
	if (!na) {
		ERRPRINT(("fs_write - ntfs_attr_open()==NULL\n"));
		*len = 0;
		result = EINVAL;
		goto exit2;
	}
	
	if (cookie->omode & O_APPEND)
		offset = na->data_size;	
	
	if (offset + size > na->data_size) {
		ntfs_mark_free_space_outdated(ns);	 
		if(ntfs_attr_truncate(na, offset + size))
			size = na->data_size - offset;
		else
#ifdef __HAIKU__		
			notify_stat_changed(ns->id, MREF(ni->mft_no), B_STAT_SIZE);	
#else
			notify_listener(B_STAT_CHANGED, ns->id, 0, 0, MREF(ni->mft_no), NULL);
#endif			
	}

	while (size) {
		result = ntfs_attr_pwrite(na, offset, size, buf);
		if (result < (s64)size)
			ERRPRINT("fs_write - ntfs_attr_pwrite returned less bytes than requested.\n");
		if (result <= 0) {
			ERRPRINT(("fs_write - ntfs_attr_pwrite()<=0\n"));
			*len = 0;
			result = EINVAL;
			goto exit;
		}
		size -= result;
		offset += result;
		total += result;
	}
	*len = total;
	ERRPRINT(("fs_write - OK\n"));
	result = B_NO_ERROR;
exit:
	if (na)
		ntfs_attr_close(na);
exit2:

	if (ni)
		ntfs_inode_close(ni);

	ERRPRINT("fs_write - EXIT, result is %s, writed %d bytes\n", strerror(result),(int)(*len));
	
	UNLOCK_VOL(ns);
	
	return result;	
}


#ifdef __HAIKU__
status_t	
fs_close(void *ns, void *node, void *cookie)
#else
int
fs_close(void *ns, void *node, void *cookie)
#endif
{
	ERRPRINT("fs_close - ENTER\n");
	
	ERRPRINT("fs_close - EXIT\n");
	return B_NO_ERROR;
}

#ifdef __HAIKU__
status_t
fs_free_cookie( void *ns, void *node, void *cookie )
#else
int
fs_free_cookie( void *ns, void *node, void *cookie )
#endif
{
	ERRPRINT("fs_free_cookie - ENTER\n");
	
	if(cookie)
		free(cookie);
	
	ERRPRINT("fs_free_cookie - EXIT\n");
	return B_NO_ERROR;
}

#ifdef __HAIKU__
status_t
fs_access( void *ns, void *node, int mode )
#else
int
fs_access( void *ns, void *node, int mode )
#endif
{
	ERRPRINT("fs_access - ENTER\n");	
	
	ERRPRINT("fs_access - EXIT\n");
	return B_NO_ERROR;
}

#ifdef __HAIKU__
status_t
fs_readlink(void *_ns, void *_node, char *buff, size_t *buff_size)
#else
int
fs_readlink(void *_ns, void *_node, char *buff, size_t *buff_size)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*node = (vnode*)_node;	
	ntfs_attr 	*na = NULL;
	INTX_FILE 	*intx_file = NULL;
	ntfs_inode 	*ni = NULL;	
	char 		*buf = NULL;
	status_t	result = B_NO_ERROR;	
	int 		l=0;
	
	LOCK_VOL(ns);
	
	ERRPRINT("fs_readlink - ENTER\n"); 
	
	if(ns==NULL || node==NULL || buff ==NULL || buff_size ==NULL) {
		result = EINVAL;
		goto	exit;
	}
	
	ni = ntfs_inode_open(ns->ntvol, node->vnid);		
	if(ni==NULL) {
			result = ENOENT;
			goto exit;
	}
	
	/* Sanity checks. */
	if (!(ni->flags & FILE_ATTR_SYSTEM)) {
		result = EINVAL;
		goto exit;
	}
	na = ntfs_attr_open(ni, AT_DATA, AT_UNNAMED, 0);
	if (!na) {
		result = errno;
		goto exit;
	}
	if (na->data_size <= sizeof(INTX_FILE_TYPES)) {
		result = EINVAL;
		goto exit;
	}
	if (na->data_size > sizeof(INTX_FILE_TYPES) +
			sizeof(ntfschar) * MAX_PATH) {
		result = ENAMETOOLONG;
		goto exit;
	}
	/* Receive file content. */
	intx_file = ntfs_malloc(na->data_size);
	if (!intx_file) {
		result = errno;
		goto exit;
	}
	if (ntfs_attr_pread(na, 0, na->data_size, intx_file) != na->data_size) {
		result = errno;
		goto exit;
	}
	/* Sanity check. */
	if (intx_file->magic != INTX_SYMBOLIC_LINK) {
		result = EINVAL;
		goto exit;
	}
	/* Convert link from unicode to local encoding. */
	l = ntfs_ucstombs(intx_file->target, (na->data_size - offsetof(INTX_FILE, target)) / sizeof(ntfschar), &buf, 0);
	
	if ( l < 0)	{
		result = errno;
		goto exit;
	}
	
	ERRPRINT("fs_readlink - LINK:[%s]\n",buf);
	
	strcpy(buff,buf);
	if(buf)
		free(buf);
	
	*buff_size = l+1;
	
	result = B_NO_ERROR;
	
exit:

	if (intx_file)
		free(intx_file);
	if (na)
		ntfs_attr_close(na);
	if(ni)
		ntfs_inode_close(ni);			
	
	ERRPRINT("fs_readlink - EXIT, result is %s\n", strerror(result));

	UNLOCK_VOL(ns);
	
	return result;
}

#ifdef __HAIKU__
status_t
fs_create_symlink(void *_ns, void *_dir, const char *name, const char *target, int mode)
#else
int
fs_create_symlink(void *_ns, void *_dir, const char *name, const char *target)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*dir = (vnode*)_dir;
	ntfs_inode	*sym = NULL;
	ntfs_inode	*bi = NULL;	
	vnode		*symnode = NULL;
	ntfschar 	*uname = NULL,
				*utarget = NULL;
	int			uname_len,
				utarget_len;
	status_t	result = B_NO_ERROR;	
		
	LOCK_VOL(ns);
	
	ERRPRINT("fs_symlink - ENTER, name=%s, path=%s\n",name, target); 
	
	if(ns == NULL || dir == NULL || name == NULL || target == NULL) {
		result = EINVAL;
		goto	exit;
	}
	
	bi = ntfs_inode_open(ns->ntvol, dir->vnid);		
	if(bi==NULL) {
			result = ENOENT;
			goto exit;
	}
	
	uname_len = ntfs_mbstoucs(name, &uname, 0);
	if (uname_len < 0) {
		result = EINVAL;
		goto exit;
	}
	
	utarget_len = ntfs_mbstoucs(target, &utarget, 0);
	if (utarget_len < 0) {
		result = EINVAL;
		goto exit;
	}
	
	sym = ntfs_create_symlink(bi, uname, uname_len, utarget, utarget_len);
	if(sym==NULL) {
		result = EINVAL;
		goto exit;
	}	
	
	symnode = (vnode*)ntfs_calloc( sizeof(vnode) );
	if(symnode==NULL) {
	 	result = ENOMEM;
	 	goto exit;
	 }
	
	symnode->vnid = MREF(sym->mft_no);
	symnode->parent_vnid = MREF(bi->mft_no);
	
	if(sym->mrec->flags & MFT_RECORD_IS_DIRECTORY)
			set_mime(symnode, ".***");
	else
			set_mime(symnode, name);
	
	if ((result = publish_vnode(ns->id, MREF(sym->mft_no), symnode)) != 0)
		ERRPRINT("fs_symlink - new_vnode failed for vnid %Ld: %s\n", MREF(sym->mft_no), strerror(result));

	put_vnode(ns->id, MREF(sym->mft_no) );

#ifdef __HAIKU__	
	notify_entry_created(ns->id, MREF( bi->mft_no ), name, MREF(sym->mft_no));	
#else
	notify_listener(B_ENTRY_CREATED, ns->id, MREF(bi->mft_no), 0, MREF(sym->mft_no), name);
#endif
	
exit:	
	
	if(sym)
		ntfs_inode_close(sym);
	if(bi)
		ntfs_inode_close(bi);
	
	ERRPRINT("fs_symlink - EXIT, result is %s\n", strerror(result));

	UNLOCK_VOL(ns);
	
	return result;
}


#ifdef __HAIKU__
status_t
fs_mkdir(void *_ns, void *_node, const char *name,	int perms, vnode_id *_vnid)
#else
int
fs_mkdir(void *_ns, void *_node, const char *name,	int perms)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*dir = (vnode*)_node;
	vnode		*newNode =NULL;	
	ntfschar 	*uname = NULL;
	int 		uname_len;			
	ntfs_inode	*ni = NULL;
	ntfs_inode	*bi = NULL;
	status_t	result = B_NO_ERROR;		

	if (ns->flags & B_FS_IS_READONLY) {
		ERRPRINT("ntfs is read-only\n");
		return EROFS;
	}
	
	LOCK_VOL(ns);
	
	ERRPRINT("fs_mkdir - ENTER: name=%s\n",name); 
	
	if(_ns==NULL || _node==NULL || name==NULL) {
	 	result = EINVAL;
	 	goto exit;
	}
	
	bi = ntfs_inode_open(ns->ntvol, dir->vnid);		
	if(bi==NULL) {
		result = ENOENT;
		goto exit;
	}

#ifndef __HAIKU__	
	if (is_vnode_removed(ns->id, MREF(bi->mft_no)) > 0) {
		ERRPRINT("fs_mkdir - called in removed directory\n");
		result = EPERM;
		goto exit;
	}
#endif
	
	if (! bi->mrec->flags & MFT_RECORD_IS_DIRECTORY) {
		result =  EINVAL;
		goto exit;
	}
	
	uname_len = ntfs_mbstoucs(name, &uname, 0);
	if (uname_len < 0) {
		result = EINVAL;
		goto exit;
	}
	
	ni = ntfs_create(bi, uname, uname_len, S_IFDIR);
		
	if (ni)	{
		vnode_id vnid = MREF( ni->mft_no );		

		newNode = (vnode*)ntfs_calloc( sizeof(vnode) );
		if(newNode==NULL) {
		 	result=ENOMEM;
		 	ntfs_inode_close(ni);
		 	goto exit;
		 }

		newNode->vnid = vnid;
		newNode->parent_vnid = MREF(bi->mft_no);
		set_mime(newNode, ".***");
		
		result = publish_vnode(ns->id, vnid, (void*)newNode);
#ifdef __HAIKU__		
		*_vnid = vnid;
#endif		
		put_vnode(ns->id, MREF(ni->mft_no));	
		
		ntfs_mark_free_space_outdated(ns);	

#ifdef __HAIKU__		
		notify_entry_created(ns->id, MREF( bi->mft_no ), name, vnid);	
#else
		notify_listener(B_ENTRY_CREATED, ns->id, MREF( bi->mft_no ), 0, vnid, name);
#endif

	}
	else
		result = errno;
			
exit:				

	if(ni)
		ntfs_inode_close(ni);
	if(bi)
		ntfs_inode_close(bi);

	ERRPRINT("fs_mkdir - EXIT, result is %s\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return 		result;
}

#ifdef __HAIKU__
status_t
fs_rename(void *_ns, void *_odir, const char *oldname, void *_ndir, const char *newname)
#else
int
fs_rename(void *_ns, void *_odir, const char *oldname, void *_ndir, const char *newname)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*odir = (vnode*)_odir;
	vnode		*ndir = (vnode*)_ndir;
	
	vnode		*onode = NULL;
	vnode		*nnode = NULL;
	
	vnode_id	ovnid,nvnid;
	
	ntfs_inode	*oi  = NULL;				
	ntfs_inode	*ndi = NULL; 
	ntfs_inode	*odi = NULL;
	
	ntfschar 	*unewname = NULL,
				*uoldname=NULL;
	int			unewname_len,
				uoldname_len;
	
	status_t	result = B_NO_ERROR;	
	
	if (ns->flags & B_FS_IS_READONLY) {
		ERRPRINT("ntfs is read-only\n");
		return EROFS;
	}	
	
	LOCK_VOL(ns);
	
	ERRPRINT("fs_rename - oldname:%s newname:%s\n", oldname, newname);		

	if (_ns == NULL || _odir == NULL || _ndir == NULL
		|| oldname == NULL || *oldname == '\0'
		|| newname == NULL || *newname == '\0'
		|| !strcmp(oldname, ".") || !strcmp(oldname, "..")
		|| !strcmp(newname, ".") || !strcmp(newname, "..")
		|| strchr(newname, '/') != NULL) { 
		result = EINVAL;
		goto	exit;
	}
	
	//stupid renaming check
	if (odir == ndir && !strcmp(oldname, newname))
		goto	exit;
	
	//convert names from utf8 to unicode string
	unewname_len = ntfs_mbstoucs(newname, &unewname, 0);
	if (unewname_len < 0) {
		result = EINVAL;
		goto exit;
	}

	uoldname_len = ntfs_mbstoucs(oldname, &uoldname, 0);
	if (uoldname_len < 0) {
		result = EINVAL;
		goto exit;
	}
	
	//open source directory inode	
	odi = ntfs_inode_open(ns->ntvol, odir->vnid);	
	if(odi==NULL) {
		result = ENOENT;
		goto exit;
	}					
					
	ovnid = MREF( ntfs_inode_lookup_by_name(odi, uoldname, uoldname_len) );
	if (ovnid == (u64) -1) {
		result = EINVAL;
		goto exit;
	}

	result = get_vnode( ns->id, ovnid, (void**)&onode );
	if( result != B_NO_ERROR)
		goto	exit;
	
	
	if(odir != ndir) {	//moving
			ndi = ntfs_inode_open(ns->ntvol, ndir->vnid);	
			if(ndi!=NULL) {
				nvnid = MREF( ntfs_inode_lookup_by_name(ndi, unewname, unewname_len) );
				if (nvnid != (u64) -1)
					get_vnode( ns->id, nvnid, (void**)&nnode );
			}	
				
			if(nnode!=NULL) {
				result = EINVAL;
				put_vnode(ns->id, nnode->vnid);
				goto exit;
			}		
											
			oi = ntfs_inode_open(ns->ntvol, onode->vnid);	
			if(oi==NULL) {
				result = EINVAL;
				goto exit;
			}
											
			if (ntfs_link(oi, ndi, unewname, unewname_len))  {
	 			ntfs_inode_close(oi);
				result = EINVAL;		
				goto exit;
			}		 
	
			if(oi->mrec->flags & MFT_RECORD_IS_DIRECTORY)
					set_mime(onode, ".***");
			else
					set_mime(onode, newname);		
			
			ntfs_inode_close(oi);	
	
			oi = ntfs_inode_open(ns->ntvol, onode->vnid);	
			if(oi==NULL) {
				result = EINVAL;
				goto exit;
			}	
			
			ntfs_delete(oi, odi, uoldname, uoldname_len);
	
			onode->parent_vnid = MREF( ndi->mft_no );	 

#ifdef __HAIKU__		
			notify_entry_moved(ns->id, MREF( odi->mft_no ), oldname, MREF( ndi->mft_no ), newname, onode->vnid );
			notify_attribute_changed(ns->id, onode->vnid, "BEOS:TYPE", B_ATTR_CHANGED);			
#else
			notify_listener(B_ENTRY_MOVED,  ns->id, MREF( odi->mft_no ), MREF( ndi->mft_no ), MREF( onode->vnid ), newname);
			notify_listener(B_ATTR_CHANGED, ns->id, 0, 0, onode->vnid, "BEOS:TYPE");
#endif
			
			put_vnode(ns->id, onode->vnid );			
			
	} else { //renaming
	
			nvnid = MREF( ntfs_inode_lookup_by_name(odi, unewname, unewname_len) );
			if (nvnid != (u64) -1)
					get_vnode( ns->id, nvnid, (void**)&nnode );
				
			if(nnode!=NULL) {
				result = EINVAL;
				put_vnode(ns->id, nnode->vnid);
				goto exit;
			}		
											
			oi = ntfs_inode_open(ns->ntvol, onode->vnid);	
			if(oi==NULL) {
				result = EINVAL;
				goto exit;
			}
											
			if (ntfs_link(oi, odi, unewname, unewname_len))  {
	 			ntfs_inode_close(oi);
				result = EINVAL;		
				goto exit;
			}		 
	
			if(oi->mrec->flags & MFT_RECORD_IS_DIRECTORY)
					set_mime(onode, ".***");
			else
					set_mime(onode, newname);		
			
			ntfs_inode_close(oi);	
	
			oi = ntfs_inode_open(ns->ntvol, onode->vnid);	
			if(oi==NULL) {
				result = EINVAL;
				goto exit;
			}	
			
			ntfs_delete(oi, odi, uoldname, uoldname_len);
#ifdef __HAIKU__
			notify_entry_moved(ns->id, MREF( odi->mft_no ), oldname, MREF( odi->mft_no ), newname, onode->vnid );
			notify_attribute_changed(ns->id, onode->vnid, "BEOS:TYPE", B_ATTR_CHANGED);
#else			
			notify_listener(B_ENTRY_MOVED,  ns->id, MREF( odi->mft_no ), MREF( odi->mft_no ), MREF( onode->vnid ), newname);
			notify_listener(B_ATTR_CHANGED, ns->id, 0, 0, onode->vnid, "BEOS:TYPE");			
#endif			
			put_vnode(ns->id, onode->vnid );
	}
			
	
exit:
	if(unewname!=NULL)free(unewname);		
	if(uoldname!=NULL)free(uoldname);	
	
	if(odi)
		ntfs_inode_close(odi);	
	if(ndi)
		ntfs_inode_close(ndi);
		
	ERRPRINT("fs_rename - EXIT, result is %s\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;	
	
}

status_t
do_unlink(nspace *vol, vnode *dir, const char *name, bool	isdir)
{
	vnode_id 	vnid;
	vnode 		*node = NULL;
	ntfs_inode	*ni = NULL;
	ntfs_inode 	*bi = NULL;
	ntfschar 	*uname = NULL;
	int 		uname_len;	
	status_t	result = B_NO_ERROR;
	
	uname_len = ntfs_mbstoucs(name, &uname, 0);
	if (uname_len < 0) {
		result = EINVAL;
		goto exit1;
	}

	bi = ntfs_inode_open(vol->ntvol, dir->vnid);		
	if(bi==NULL) {
		result = ENOENT;
		goto exit1;
	}	

	vnid = MREF( ntfs_inode_lookup_by_name(bi, uname, uname_len) );
	
	if ( vnid == (u64) -1 || vnid == FILE_root)	{			
		result = EINVAL;
		goto exit1;
	}
		
	result = get_vnode(vol->id, vnid, (void**) & node);
	
	if(result != B_NO_ERROR || node==NULL)
	{
		result = ENOENT;
		goto	exit1;
	}
	
	ni = ntfs_inode_open(vol->ntvol, node->vnid);		
	if(ni==NULL) {
			result = ENOENT;
			goto exit2;
	}	
	
	if(isdir) {
		if (!ni->mrec->flags & MFT_RECORD_IS_DIRECTORY) {
			result = ENOTDIR;
			goto exit2;
		}		
		if (ntfs_check_empty_dir(ni)<0)	{
			result = ENOTEMPTY;
			goto	exit2;
		}	
	} else if (ni->mrec->flags & MFT_RECORD_IS_DIRECTORY) {
			result = EISDIR;
			goto exit2;
		}
	
	if(ntfs_delete(ni, bi, uname, uname_len))
	 	result = errno;
	else
		ni = NULL;	 	
		
	node->parent_vnid = dir->vnid;

#ifdef __HAIKU__		
	notify_entry_removed(vol->id, dir->vnid, name, vnid);	
#else
	notify_listener(B_ENTRY_REMOVED, vol->id, MREF(dir->vnid), 0, vnid, NULL);	
#endif

	result = remove_vnode(vol->id, vnid);
exit2:
	put_vnode(vol->id, vnid);
exit1:	
	if(uname!=NULL)
		free(uname);

	if(ni)
		ntfs_inode_close(ni);	
			
	if(bi)
		ntfs_inode_close(bi);
				
	return result;
}


#ifdef __HAIKU__
status_t
fs_rmdir(void *_ns, void *_node, const char *name)
#else
int
fs_rmdir(void *_ns, void *_node, const char *name)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*dir = (vnode*)_node;
	status_t	result = B_NO_ERROR;

	if (ns->flags & B_FS_IS_READONLY) {
		ERRPRINT("ntfs is read-only\n");
		return EROFS;
	}

	LOCK_VOL(ns);
	
	ERRPRINT("fs_rmdir  - ENTER:  name %s\n", name==NULL?"NULL":name);
	
	if( ns == NULL || dir == NULL || name ==NULL) {
		result = EINVAL;
		goto exit1;
	}

	if (strcmp(name, ".") == 0) {
		result = EPERM;
		goto exit1;
	}
	
	if (strcmp(name, "..") == 0) {
		result = EPERM;
		goto exit1;
	}
	
	result = do_unlink(ns, dir, name, true);	
		
	ntfs_mark_free_space_outdated(ns);

exit1:
	
	ERRPRINT("fs_rmdir - EXIT, result is %s\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;
}


#ifdef __HAIKU__
status_t
fs_unlink(void *_ns, void *_node, const char *name)
#else
int
fs_unlink(void *_ns, void *_node, const char *name)
#endif
{
	nspace		*ns = (nspace*)_ns;
	vnode		*dir = (vnode*)_node;
	status_t	result = B_NO_ERROR;

	if (ns->flags & B_FS_IS_READONLY) {
		ERRPRINT("ntfs is read-only\n");
		return EROFS;
	}

	LOCK_VOL(ns);
	
	ERRPRINT("fs_unlink  - ENTER:  name %s\n", name==NULL?"NULL":name);
	
	if( ns == NULL || dir == NULL || name ==NULL) {
		result = EINVAL;
		goto exit;
	}

	if (strcmp(name, ".") == 0) {
		result = EPERM;
		goto exit;
	}
	
	if (strcmp(name, "..") == 0) {
		result = EPERM;
		goto exit;
	}
	
	result = do_unlink(ns, dir, name, false);	
		
	ntfs_mark_free_space_outdated(ns);

exit:
	
	ERRPRINT("fs_unlink - EXIT, result is %s\n", strerror(result));
	
	UNLOCK_VOL(ns);
	
	return result;
}

