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

#include "fs_func.h"
#include "ntfsdir.h"
#include "attributes.h"


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

fs_volume_ops gNTFSVolumeOps = {
	&fs_unmount,
	&fs_rfsstat,
	&fs_wfsstat,
	&fs_sync,
	&fs_read_vnode,

	/* index directory & index operations */
	NULL,	//&fs_open_index_dir,
	NULL,	//&fs_close_index_dir,
	NULL,	//&fs_free_index_dir_cookie,
	NULL,	//&fs_read_index_dir,
	NULL,	//&fs_rewind_index_dir,

	NULL,	//&fs_create_index,
	NULL,	//&fs_remove_index,
	NULL,	//&fs_stat_index,

	/* query operations */
	NULL,	//&fs_open_query,
	NULL,	//&fs_close_query,
	NULL, 	//&fs_free_query_cookie,
	NULL, 	//&fs_read_query,
	NULL, 	//&fs_rewind_query,
};

fs_vnode_ops gNTFSVnodeOps = {
	/* vnode operations */
	&fs_walk,
	&fs_get_vnode_name,
	&fs_write_vnode,
	&fs_remove_vnode,

	/* VM file access */
	NULL,
	NULL,
	NULL,

	NULL,	// io()
	NULL,	// cancel_io()

	NULL,

	NULL,
	NULL,	//&fs_set_flags,
	NULL,	//&fs_select
	NULL,	//&fs_deselect
	&fs_fsync,

	&fs_readlink,
	&fs_create_symlink,	

	NULL,	//&fs_link,
	&fs_unlink,
	&fs_rename,

	&fs_access,
	&fs_rstat,
	&fs_wstat,
	NULL,	// fs_preallocate

	/* file operations */
	&fs_create,
	&fs_open,
	&fs_close,
	&fs_free_cookie,
	&fs_read,
	&fs_write,

	/* directory operations */
	&fs_mkdir,
	&fs_rmdir,
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
	&fs_create_attrib,
	&fs_open_attrib,
	&fs_close_attrib,
	&fs_free_attrib_cookie,
	&fs_read_attrib,
	&fs_write_attrib,

	&fs_read_attrib_stat,
	NULL,	//&fs_write_attr_stat,
	NULL,	//&fs_rename_attr,
	&fs_remove_attrib,
};



static file_system_module_info sNTFSFileSystem = {
	{
		"file_systems/ntfs" B_CURRENT_FS_API_VERSION,
		0,
		ntfs_std_ops,
	},

	"ntfs",						// short_name
	"Windows NT File System",	// pretty_name
	B_DISK_SYSTEM_SUPPORTS_WRITING,							// DDM flags

	// scanning
	fs_identify_partition,
	fs_scan_partition,
	fs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&fs_mount,
};


module_info *modules[] = {
	(module_info *)&sNTFSFileSystem,
	NULL,
};
