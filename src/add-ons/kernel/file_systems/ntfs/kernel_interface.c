/* kernel_interface - file system interface to Haiku's vnode layer
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
#include <ctype.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <lock.h>
#include <malloc.h>
#include <stdio.h>

#ifdef __HAIKU__

#include <fs_interface.h>
#include <kernel/lock.h>
#include <fs_info.h>
#include <fs_cache.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <fs_index.h>
#include <fs_query.h>
#include <fs_volume.h>
#include <NodeMonitor.h>
#include <util/kernel_cpp.h>

#else

#include "fsproto.h"
#include "lock.h"

#define publish_vnode new_vnode

#endif

#include "fs_func.h"
#include "ntfsdir.h"
#include "attributes.h"


#ifdef __HAIKU__

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

#else

int32	api_version = B_CUR_FS_API_VERSION;

vnode_ops fs_entry =  {
	&fs_read_vnode,						/* read_vnode func ptr */
	&fs_write_vnode,					/* write_vnode func ptr */
	&fs_remove_vnode,					/* remove_vnode func ptr */
	NULL,								/* secure_vnode func ptr */
	&fs_walk,							/* walk func ptr */
	&fs_access,							/* access func ptr */
	&fs_create,							/* create func ptr */
	&fs_mkdir, 							/* mkdir func ptr */
	&fs_create_symlink,
	NULL,
	&fs_rename,
	&fs_unlink,							/* unlink func ptr */
	&fs_rmdir, 							/* rmdir func ptr */
	&fs_readlink,						/* readlink func ptr */
	&fs_opendir,						/* opendir func ptr */
	&fs_closedir,						/* closedir func ptr */
	&fs_free_dircookie,					/* free_dircookie func ptr */
	&fs_rewinddir,						/* rewinddir func ptr */
	&fs_readdir,						/* readdir func ptr */
	&fs_open,							/* open file func ptr */
	&fs_close,							/* close file func ptr */
	&fs_free_cookie,					/* free cookie func ptr */
	&fs_read,							/* read file func ptr */
	&fs_write, 							/* write file func ptr */
	NULL, /* readv */
	NULL, /* writev */
	NULL,								/* ioctl func ptr */
	NULL,								/* setflags file func ptr */
	&fs_rstat,							/* rstat func ptr */
	&fs_wstat, 							/* wstat func ptr */
	&fs_fsync,
	NULL, 								/* initialize func ptr */
	&fs_mount,							/* mount func ptr */
	&fs_unmount,						/* unmount func ptr */
	&fs_sync,							/* sync func ptr */
	&fs_rfsstat,						/* rfsstat func ptr */
	&fs_wfsstat,						/* wfsstat func ptr */
	NULL, 								// select
	NULL, 								// deselect

	NULL,								// open_indexdir
	NULL, 								// close_indexdir
	NULL, 								// free_indexdircookie
	NULL, 								// rewind_indexdir
	NULL, 								// read_indexdir

	NULL, 								// create_index
	NULL, 								// remove_index
	NULL, 								// rename_index
	NULL, 								// stat_index

	&fs_open_attrib_dir, 				// open_attrdir
	&fs_close_attrib_dir,				// close_attrdir
	&fs_free_attrib_dir_cookie, 		// free_attrdircookie
	&fs_rewind_attrib_dir, 				// rewind_attrdir
	&fs_read_attrib_dir, 				// read_attrdir

	&fs_write_attrib, 					// write_attr
	&fs_read_attrib, 					// read_attr
	NULL, 								// remove_attr
	NULL, 								// rename_attr
	&fs_read_attrib_stat, 				// stat_attr

	NULL, 								// open_query
	NULL, 								// close_query
	NULL, 								// free_querycookie
	NULL  								// read_query
};

#endif
