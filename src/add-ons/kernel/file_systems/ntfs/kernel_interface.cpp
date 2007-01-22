/* kernel_interface - file system interface to Haiku's vnode layer
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
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <lock.h>
#include <malloc.h>
#include <stdio.h>

#include <ByteOrder.h>
#include <fs_info.h>
#include <KernelExport.h>

#include <NodeMonitor.h>

#include <fs_interface.h>
#include <fs_cache.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <fs_index.h>
#include <fs_query.h>
#include <fs_volume.h>

#include <util/kernel_cpp.h>


extern "C"{

 status_t	fs_mount( mount_id nsid, const char *device, ulong flags, const char *args, void **data, vnode_id *vnid );
 status_t	fs_unmount(void *_ns);
 status_t	fs_rfsstat(void *_ns, struct fs_info *);
 status_t  	fs_wfsstat(void *_vol, const struct fs_info *fss, uint32 mask);
 status_t 	fs_sync(void *_ns);
 status_t	fs_walk(void *_ns, void *_base, const char *file, vnode_id *vnid,int *_type);
 status_t	fs_get_vnode_name(void *_ns, void *_node, char *buffer, size_t bufferSize);
 status_t	fs_read_vnode(void *_ns, vnode_id vnid, void **_node, bool reenter);
 status_t	fs_write_vnode(void *_ns, void *_node, bool reenter);
 status_t   fs_remove_vnode( void *_ns, void *_node, bool reenter );
 status_t	fs_access( void *ns, void *node, int mode ); 
 status_t	fs_rstat(void *_ns, void *_node, struct stat *st); 
 status_t 	fs_wstat(void *_vol, void *_node, const struct stat *st, uint32 mask);
 status_t   fs_create(void *_ns, void *_dir, const char *name, int omode, int perms, void **_cookie, vnode_id *_vnid);
 status_t	fs_open(void *_ns, void *_node, int omode, void **cookie);
 status_t	fs_close(void *ns, void *node, void *cookie);
 status_t	fs_free_cookie(void *ns, void *node, void *cookie);
 status_t	fs_read(void *_ns, void *_node, void *cookie, off_t pos, void *buf, size_t *len);
 status_t	fs_write(void *ns, void *node, void *cookie, off_t pos, const void *buf, size_t *len); 
 status_t	fs_mkdir(void *_ns, void *_node, const char *name,	int perms, vnode_id *_vnid);
 status_t 	fs_rmdir(void *_ns, void *dir, const char *name);
 status_t	fs_opendir(void* _ns, void* _node, void** cookie);
 status_t   fs_readdir( void *_ns, void *_node, void *_cookie, struct dirent *buf, size_t bufsize, uint32 *num );
 status_t	fs_rewinddir(void *_ns, void *_node, void *cookie);
 status_t	fs_closedir(void *_ns, void *_node, void *cookie);
 status_t	fs_free_dircookie(void *_ns, void *_node, void *cookie);
 status_t 	fs_readlink(void *_ns, void *_node, char *buf, size_t *bufsize);
 status_t 	fs_fsync(void *_ns, void *_node);

 status_t   fs_rename(void *_ns, void *_odir, const char *oldname, void *_ndir, const char *newname);
 status_t   fs_unlink(void *_ns, void *_node, const char *name);
 status_t	fs_create_symlink(void *_ns, void *_dir, const char *name, const char *target, int mode);

 status_t 	fs_open_attrib_dir(void *_vol, void *_node, void **_cookie);
 status_t 	fs_close_attrib_dir(void *_vol, void *_node, void *_cookie);
 status_t 	fs_free_attrib_dir_cookie(void *_vol, void *_node, void *_cookie);
 status_t 	fs_rewind_attrib_dir(void *_vol, void *_node, void *_cookie);
 status_t 	fs_read_attrib_dir(void *_vol, void *_node, void *_cookie, struct dirent *buf, size_t bufsize, uint32 *num);
 status_t 	fs_open_attrib(void *_vol, void *_node, const char *name, int openMode,fs_cookie *_cookie);
 status_t 	fs_close_attrib(void *_vol, void *_node, fs_cookie cookie);
 status_t 	fs_free_attrib_cookie(void *_vol, void *_node, fs_cookie cookie);
 status_t 	fs_read_attrib_stat(void *_vol, void *_node, fs_cookie _cookie, struct stat *stat);
 status_t 	fs_read_attrib(void *_vol, void *_node, fs_cookie _cookie, off_t pos,void *buffer, size_t *_length);
 status_t 	fs_write_attrib(void *_vol, void *_node, fs_cookie _cookie, off_t pos,	const void *buffer, size_t *_length);

 void 		fs_free_identify_partition_cookie(partition_data *partition, void *_cookie);
 status_t	fs_scan_partition(int fd, partition_data *partition, void *_cookie);
 float		fs_identify_partition(int fd, partition_data *partition, void **_cookie);
};


static status_t
ntfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;
		case B_MODULE_UNINIT:
			return B_OK;
		default:
			return B_ERROR;
	}
}


static file_system_module_info sNTFSFileSystem = {
	{
		"file_systems/ntfs" B_CURRENT_FS_API_VERSION,
		0,
		ntfs_std_ops,
	},

	"ntfs File System",

	// scanning
	fs_identify_partition,
	fs_scan_partition,
	fs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&fs_mount,
	&fs_unmount,
	&fs_rfsstat,
#ifdef _READ_ONLY_
	NULL,
#else
	&fs_wfsstat,
#endif
	NULL,

	/* vnode operations */
	&fs_walk,
	&fs_get_vnode_name,
	&fs_read_vnode,
	&fs_write_vnode,
#ifdef _READ_ONLY_
	NULL,
#else	
	&fs_remove_vnode,
#endif

	/* VM file access */
	NULL, 	// &fs_can_page
	NULL,	// &fs_read_pages
	NULL, 	// &fs_write_pages

	NULL,	// &fs_get_file_map

	NULL, 	// &fs_ioctl
	NULL, 	// &fs_set_flags
	NULL,	// &fs_select
	NULL,	// &fs_deselect

#ifdef _READ_ONLY_
	NULL,
#else		
	&fs_fsync,
#endif	

	&fs_readlink,
	
#ifdef _READ_ONLY_
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
#else	
	NULL,	// write link
	&fs_create_symlink,
	NULL, 	// &fs_link,
	&fs_unlink,
	&fs_rename,
#endif

	&fs_access,
	&fs_rstat,
#ifdef _READ_ONLY_
	NULL,
#else		
	&fs_wstat,
#endif

	/* file operations */
#ifdef _READ_ONLY_
	NULL,
#else			
	&fs_create,
#endif	
	&fs_open,
	&fs_close,
	&fs_free_cookie,
	&fs_read,
#ifdef _READ_ONLY_
	NULL,
#else			
	&fs_write,
#endif	

	/* directory operations */
#ifdef _READ_ONLY_
	NULL,
	NULL,
#else			
	&fs_mkdir,
	&fs_rmdir,
#endif	
	&fs_opendir,
	&fs_closedir,
	&fs_free_dircookie,
	&fs_readdir,
	&fs_rewinddir,
	
	/* attribute directory operations */
	&fs_open_attrib_dir,
	&fs_close_attrib_dir,
	&fs_free_attrib_dir_cookie,
	&fs_read_attrib_dir,
	&fs_rewind_attrib_dir,

	/* attribute operations */
	NULL,	//&fs_create_attr,
	&fs_open_attrib,
	&fs_close_attrib,
	&fs_free_attrib_cookie,
	&fs_read_attrib,
	&fs_write_attrib,

	&fs_read_attrib_stat,
	NULL,	//&fs_write_attr_stat,
	NULL,	//&fs_rename_attr,
	NULL,	//&fs_remove_attr,

	/* index directory & index operations */
	NULL,	// &fs_open_index_dir
	NULL,	// &fs_close_index_dir
	NULL,	// &fs_free_index_dir_cookie
	NULL,	// &fs_read_index_dir
	NULL,	// &fs_rewind_index_dir

	NULL,	// &fs_create_index
	NULL,	// &fs_remove_index
	NULL,	// &fs_stat_index

	/* query operations */
	NULL,	// &fs_open_query
	NULL,	// &fs_close_query
	NULL,	// &fs_free_query_cookie
	NULL,	// &fs_read_query
	NULL,	// &fs_rewind_query
};

module_info *modules[] = {
	(module_info *)&sNTFSFileSystem,
	NULL,
};
