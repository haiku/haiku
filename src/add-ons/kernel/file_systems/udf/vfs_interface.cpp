//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Mucho respecto to Axel Dörfler and his BFS implementation, from
//  which this UDF implementation draws much influence (and a little
//  code :-P).
//---------------------------------------------------------------------

/*! \file vfs_interface.cpp
*/

#include "Debug.h"
#include "cpp.h"

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// BeOS vnode layer stuff
#include <KernelExport.h>
#ifndef _IMPEXP_KERNEL
#	define _IMPEXP_KERNEL
#endif

extern "C" {
	#include <fsproto.h>
}	// end extern "C"

#ifdef USER
#	define dprintf printf
#endif


extern "C" {
	// general/volume stuff
	static int udf_mount(nspace_id nsid, const char *device, ulong flags,
					void *parms, size_t len, void **data, vnode_id *vnid);
	static int udf_unmount(void *_ns);
	static int udf_read_fs_stat(void *_ns, struct fs_info *);
	static int udf_write_fs_stat(void *ns, struct fs_info *, long mode);
	static int udf_initialize(const char *devname, void *parms, size_t len);

	static int udf_sync(void *ns);

	static int udf_read_vnode(void *_ns, vnode_id vnid, char r, void **node);
	static int udf_release_vnode(void *_ns, void *_node, char r);
	static int udf_remove_vnode(void *ns, void *node, char r);

	static int udf_walk(void *_ns, void *_base, const char *file,
					char **newpath, vnode_id *vnid);
	
	static int udf_ioctl(void *ns, void *node, void *cookie, int cmd, void *buf,size_t len);
	static int udf_setflags(void *ns, void *node, void *cookie, int flags);

	static int udf_select(void *ns, void *node, void *cookie, uint8 event,
					uint32 ref, selectsync *sync);
	static int udf_deselect(void *ns, void *node, void *cookie, uint8 event,
					selectsync *sync);
	static int udf_fsync(void *ns,void *node);

	// file stuff
	static int udf_create(void *ns, void *dir, const char *name,
					int perms, int omode, vnode_id *vnid, void **cookie);
	static int udf_symlink(void *ns, void *dir, const char *name,
					const char *path);
	static int udf_link(void *ns, void *dir, const char *name, void *node);
	static int udf_unlink(void *ns, void *dir, const char *name);
	static int udf_rename(void *ns, void *oldDir, const char *oldName, void *newDir, const char *newName);


	static int udf_read_stat(void *_ns, void *_node, struct stat *st);
	static int udf_write_stat(void *ns, void *node, struct stat *st, long mask);

	static int udf_open(void *_ns, void *_node, int omode, void **cookie);
	static int udf_read(void *_ns, void *_node, void *cookie, off_t pos,
					void *buf, size_t *len);
	static int udf_write(void *ns, void *node, void *cookie, off_t pos,
					const void *buf, size_t *len);
	static int udf_free_cookie(void *ns, void *node, void *cookie);
	static int udf_close(void *ns, void *node, void *cookie);
	
	static int udf_access(void *_ns, void *_node, int mode);
	static int udf_read_link(void *_ns, void *_node, char *buffer, size_t *bufferSize);

	// directory stuff
	static int udf_mkdir(void *ns, void *dir, const char *name, int perms);
	static int udf_rmdir(void *ns, void *dir, const char *name);
	static int udf_open_dir(void *_ns, void *_node, void **cookie);
	static int udf_read_dir(void *_ns, void *_node, void *cookie,
					long *num, struct dirent *dirent, size_t bufferSize);
	static int udf_rewind_dir(void *_ns, void *_node, void *cookie);
	static int udf_close_dir(void *_ns, void *_node, void *cookie);
	static int udf_free_dir_cookie(void *_ns, void *_node, void *cookie);

	// attribute stuff
	static int udf_open_attrdir(void *ns, void *node, void **cookie);
	static int udf_close_attrdir(void *ns, void *node, void *cookie);
	static int udf_free_attrdir_cookie(void *ns, void *node, void *cookie);
	static int udf_rewind_attrdir(void *ns, void *node, void *cookie);
	static int udf_read_attrdir(void *ns, void *node, void *cookie, long *num,
					struct dirent *buf, size_t bufferSize);
	static int udf_remove_attr(void *ns, void *node, const char *name);
	static int udf_rename_attr(void *ns, void *node, const char *oldname,
					const char *newname);
	static int udf_stat_attr(void *ns, void *node, const char *name,
					struct attr_info *buf);
	static int udf_write_attr(void *ns, void *node, const char *name, int type,
					const void *buf, size_t *len, off_t pos);
	static int udf_read_attr(void *ns, void *node, const char *name, int type,
					void *buf, size_t *len, off_t pos);

	// index stuff
	static int udf_open_indexdir(void *ns, void **cookie);
	static int udf_close_indexdir(void *ns, void *cookie);
	static int udf_free_indexdir_cookie(void *ns, void *node, void *cookie);
	static int udf_rewind_indexdir(void *ns, void *cookie);
	static int udf_read_indexdir(void *ns, void *cookie, long *num,struct dirent *dirent,
					size_t bufferSize);
	static int udf_create_index(void *ns, const char *name, int type, int flags);
	static int udf_remove_index(void *ns, const char *name);
	static int udf_rename_index(void *ns, const char *oldname, const char *newname);
	static int udf_stat_index(void *ns, const char *name, struct index_info *indexInfo);

	// query stuff
	static int udf_open_query(void *ns, const char *query, ulong flags,
					port_id port, long token, void **cookie);
	static int udf_close_query(void *ns, void *cookie);
	static int udf_free_query_cookie(void *ns, void *node, void *cookie);
	static int udf_read_query(void *ns, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);

	// dano stuff (for Mr. Dörfler)
	static int udf_wake_vnode(void *ns, void *node);
	static int udf_suspend_vnode(void *ns, void *node);
};	// end extern "C"

vnode_ops fs_entry =  {
	udf_read_vnode,				// read vnode
	udf_release_vnode,			// write vnode
	udf_remove_vnode,			// remove vnode
	NULL,						// secure vnode (unused)
	udf_walk,					// walk
	udf_access,					// access
	udf_create,					// create
	udf_mkdir,					// mkdir
	udf_symlink,				// symlink
	udf_link,					// link
	udf_rename,					// rename
	udf_unlink,					// unlink
	udf_rmdir,					// rmdir
	udf_read_link,				// readlink
	udf_open_dir,				// opendir
	udf_close_dir,				// closedir
	udf_free_dir_cookie,		// free dir cookie
	udf_rewind_dir,				// rewinddir
	udf_read_dir,				// readdir
	udf_open,					// open file
	udf_close,					// close file
	udf_free_cookie,			// free cookie
	udf_read,					// read file
	udf_write,					// write file
	NULL,						// readv
	NULL,						// writev
	udf_ioctl,					// ioctl
	udf_setflags,				// setflags file
	udf_read_stat,				// read stat
	udf_write_stat,				// write stat
	udf_fsync,					// fsync
	udf_initialize,				// initialize
	udf_mount,					// mount
	udf_unmount,				// unmount
	udf_sync,					// sync
	udf_read_fs_stat,			// read fs stat
	udf_write_fs_stat,			// write fs stat
	udf_select,					// select
	udf_deselect,				// deselect

	udf_open_indexdir,			// open index dir
	udf_close_indexdir,			// close index dir
	udf_free_indexdir_cookie,	// free index dir cookie
	udf_rewind_indexdir,		// rewind index dir
	udf_read_indexdir,			// read index dir
	udf_create_index,			// create index
	udf_remove_index,			// remove index
	udf_rename_index,			// rename index
	udf_stat_index,				// stat index

	udf_open_attrdir,			// open attr dir
	udf_close_attrdir,			// close attr dir
	udf_free_attrdir_cookie,	// free attr dir cookie
	udf_rewind_attrdir,			// rewind attr dir
	udf_read_attrdir,			// read attr dir
	udf_write_attr,				// write attr
	udf_read_attr,				// read attr
	udf_remove_attr,			// remove attr
	udf_rename_attr,			// rename attr
	udf_stat_attr,				// stat attr

	udf_open_query,				// open query
	udf_close_query,			// close query
	udf_free_query_cookie,		// free query cookie
	udf_read_query,				// read query
	
	udf_wake_vnode,				// dano compatibility
	udf_suspend_vnode			// dano compatibility
};

int32 api_version = B_CUR_FS_API_VERSION;

//----------------------------------------------------------------------
// General/volume functions
//----------------------------------------------------------------------

int
udf_mount(nspace_id nsid, const char *deviceName, ulong flags, void *parms,
		size_t parmsLength, void **volumeCookie, vnode_id *rootID)
{
	FUNCTION();

	if (rootID)
		*rootID = 0;
	if (volumeCookie)
		*volumeCookie = new char[1];
	dprintf("new_vnode(root) = %d\n", new_vnode(nsid, *rootID, (void*)23));

	return B_OK;	
}


int
udf_unmount(void *ns)
{
	FUNCTION();
	return B_OK;	
}


int
udf_read_fs_stat(void *ns, struct fs_info *info)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_write_fs_stat(void *ns, struct fs_info *info, long mask)
{
	FUNCTION_START(("mask = %ld\n",mask));
	return B_ERROR;
}


int 
udf_initialize(const char *deviceName, void *parms, size_t parmsLength)
{
	FUNCTION_START(("deviceName = %s, parameter len = %ld\n", deviceName, parmsLength));
	return B_ERROR;
}


int 
udf_sync(void *ns)
{
	FUNCTION();
	return B_OK;
}


int
udf_read_vnode(void *ns, vnode_id id, char reenter, void **node)
{
	dprintf("read_vnode\n");
	FUNCTION_START(("vnode_id = %lld\n", id));

	if (id == 1) {
		*node = (void*)0;
		return B_OK;
	} else
		return B_ERROR;
}


int
udf_release_vnode(void *ns, void *node, char reenter)
{
	FUNCTION_START(("node = %p\n",node));
	return B_ERROR;
}


int 
udf_remove_vnode(void *ns, void *node, char reenter)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_wake_vnode(void *ns, void *node)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_suspend_vnode(void *ns, void *node)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_walk(void *ns, void *dir, const char *filename, char **resolvedPath, vnode_id *vnodeID)
{
	FUNCTION_START(("dir = %p, filename = `%s'\n", dir, filename));
	return B_ERROR;
}


int 
udf_ioctl(void *ns, void *node, void *cookie, int cmd, void *buffer, size_t bufferLength)
{
	FUNCTION_START(("node = %p, cmd = %d, buf = %p, len = %ld\n", node, cmd, buffer, bufferLength));
	return B_ERROR;
}


int 
udf_setflags(void *ns, void *node, void *cookie, int flags)
{
	FUNCTION_START(("node = %p, flags = %d", node, flags));
	return B_ERROR;
}


int 
udf_select(void *ns, void *node, void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	FUNCTION_START(("event = %d, ref = %lu, sync = %p\n", event, ref, sync));
	return B_ERROR;
}


int 
udf_deselect(void *ns, void *node, void *cookie, uint8 event, selectsync *sync)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_fsync(void *_ns, void *_node)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_read_stat(void *ns, void *node, struct stat *st)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_write_stat(void *ns, void *node, struct stat *stat, long mask)
{
	FUNCTION();
	return B_ERROR;
}


//----------------------------------------------------------------------
// File functions
//----------------------------------------------------------------------


int 
udf_create(void *ns, void *dir, const char *name, int omode, int mode,
	vnode_id *newID, void **newNode)
{
	FUNCTION_START(("name = \"%s\", perms = %d, omode = %d\n", name, mode, omode));
	return B_ERROR;
}


int 
udf_symlink(void *ns, void *dir, const char *name, const char *path)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_link(void *ns, void *dir, const char *name, void *node)
{
	FUNCTION_START(("name = \"%s\"\n", name));
	return B_ERROR;
}


int 
udf_unlink(void *ns, void *dir, const char *name)
{
	FUNCTION_START(("name = \"%s\"\n",name));
	return B_ERROR;
}


int 
udf_rename(void *ns, void *oldDir, const char *oldName, void *newDir, const char *newName)
{
	FUNCTION_START(("oldDir = %p, oldName = \"%s\", newDir = %p, newName = \"%s\"\n", oldDir, oldName, newDir, newName));
	return B_ERROR;
}

int
udf_open(void *_ns, void *_node, int omode, void **_cookie)
{
	FUNCTION();
	return B_ERROR;
}

int
udf_read(void *_ns, void *_node, void *_cookie, off_t pos, void *buffer, size_t *_length)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_write(void *_ns, void *_node, void *_cookie, off_t pos, const void *buffer, size_t *_length)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_close(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_free_cookie(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_access(void *_ns, void *_node, int accessMode)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_read_link(void *_ns, void *_node, char *buffer, size_t *bufferSize)
{
	FUNCTION();
	return B_ERROR;
}


//----------------------------------------------------------------------
//	Directory functions
//----------------------------------------------------------------------


int 
udf_mkdir(void *ns, void *dir, const char *name, int mode)
{
	FUNCTION_START(("name = \"%s\", perms = %d\n", name, mode));
	return B_ERROR;
}


int 
udf_rmdir(void *ns, void *dir, const char *name)
{
	FUNCTION_START(("name = \"%s\"\n", name));
	return B_ERROR;
}


int
udf_open_dir(void *ns, void *node, void **cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_read_dir(void *ns, void *node, void *cookie, long *num, 
	struct dirent *dirent, size_t bufferSize)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_rewind_dir(void *ns, void *node, void *cookie)
{
	FUNCTION();
	return B_ERROR;
}


int		
udf_close_dir(void *ns, void *node, void *cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_free_dir_cookie(void *ns, void *node, void *cookie)
{
	FUNCTION();
	return B_ERROR;
}


//----------------------------------------------------------------------
//	Attribute functions
//----------------------------------------------------------------------


int 
udf_open_attrdir(void *ns, void *node, void **cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_close_attrdir(void *ns, void *node, void *cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_free_attrdir_cookie(void *ns, void *node, void *_cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_rewind_attrdir(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_read_attrdir(void *_ns, void *node, void *_cookie, long *num, struct dirent *dirent, size_t bufsize)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_remove_attr(void *_ns, void *_node, const char *name)
{
	FUNCTION_START(("name = \"%s\"\n",name));
	return B_ERROR;
}


int
udf_rename_attr(void *ns, void *node, const char *oldname, const char *newname)
{
	FUNCTION_START(("name = \"%s\",to = \"%s\"\n", oldname, newname));
	return B_ERROR;
}


int
udf_stat_attr(void *ns, void *_node, const char *name, struct attr_info *attrInfo)
{
	FUNCTION_START(("name = \"%s\"\n",name));
	return B_ERROR;
}


int
udf_write_attr(void *_ns, void *_node, const char *name, int type, const void *buffer,
	size_t *_length, off_t pos)
{
	FUNCTION_START(("name = \"%s\"\n",name));
	return B_ERROR;
}


int
udf_read_attr(void *_ns, void *_node, const char *name, int type, void *buffer,
	size_t *_length, off_t pos)
{
	FUNCTION();
	return B_ERROR;
}


//----------------------------------------------------------------------
//	Index functions
//----------------------------------------------------------------------


int 
udf_open_indexdir(void *_ns, void **_cookie)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_close_indexdir(void *_ns, void *_cookie)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_free_indexdir_cookie(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_rewind_indexdir(void *_ns, void *_cookie)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_read_indexdir(void *_ns, void *_cookie, long *num, struct dirent *dirent, size_t bufferSize)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_create_index(void *_ns, const char *name, int type, int flags)
{
	FUNCTION_START(("name = \"%s\", type = %d, flags = %d\n", name, type, flags));
	return B_ERROR;
}


int 
udf_remove_index(void *_ns, const char *name)
{
	FUNCTION();
	return B_ERROR;
}


int 
udf_rename_index(void *ns, const char *oldname, const char *newname)
{
	FUNCTION_START(("from = %s, to = %s\n", oldname, newname));
	return B_ERROR;
}


int 
udf_stat_index(void *_ns, const char *name, struct index_info *indexInfo)
{
	FUNCTION_START(("name = %s\n",name));
	return B_ERROR;
}

//----------------------------------------------------------------------
//	Query functions
//----------------------------------------------------------------------

int 
udf_open_query(void *_ns, const char *queryString, ulong flags, port_id port,
	long token, void **cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_close_query(void *ns, void *cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_free_query_cookie(void *ns, void *node, void *cookie)
{
	FUNCTION();
	return B_ERROR;
}


int
udf_read_query(void *ns, void *cookie, long *num, struct dirent *dirent, size_t bufferSize)
{
	FUNCTION();
	return B_ERROR;
}

