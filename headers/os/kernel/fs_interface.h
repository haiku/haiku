/* File System Interface Layer Definition
** 
** Distributed under the terms of the OpenBeOS License.
*/

#ifndef _FS_INTERFACE_H
#define _FS_INTERFACE_H

#include <ktypes.h>
#include <vfs_types.h>

struct dirent;
struct stat;
struct fs_info;


/* the file system's private data structures */
typedef void *fs_volume;
typedef void *fs_cookie;
typedef void *fs_vnode;

/* passed to fs_write_stat() */
enum write_stat_mask {
	FS_WSTAT_MODE	= 0x0001,
	FS_WSTAT_UID	= 0x0002,
	FS_WSTAT_GID	= 0x0004,
	FS_WSTAT_SIZE	= 0x0008,
	FS_WSTAT_ATIME	= 0x0010,
	FS_WSTAT_MTIME	= 0x0020,
	FS_WSTAT_CRTIME	= 0x0040
};

#define	WFSSTAT_NAME	0x0001

#define	B_CURRENT_FS_API_VERSION 3

#ifdef __cplusplus
extern "C" {
#endif 

struct fs_calls {
	/* general operations */
	status_t (*fs_mount)(fs_id id, const char *device, void *args, fs_volume *_fs, vnode_id *_rootVnodeID);
	status_t (*fs_unmount)(fs_volume fs);

	status_t (*fs_read_fs_info)(fs_volume fs, struct fs_info *info);
	status_t (*fs_write_fs_info)(fs_volume fs, const struct fs_info *info, int mask);
	status_t (*fs_sync)(fs_volume fs);

	/* vnode operations */
	status_t (*fs_lookup)(fs_volume fs, fs_vnode dir, const char *name, vnode_id *_id, int *_type);
	status_t (*fs_get_vnode_name)(fs_volume fs, fs_vnode vnode, char *buffer, size_t bufferSize);

	status_t (*fs_get_vnode)(fs_volume fs, vnode_id id, fs_vnode *_vnode, bool reenter);
	status_t (*fs_put_vnode)(fs_volume fs, fs_vnode vnode, bool reenter);
	status_t (*fs_remove_vnode)(fs_volume fs, fs_vnode vnode, bool reenter);

	/* VM file access */
	status_t (*fs_can_page)(fs_volume fs, fs_vnode v);
	ssize_t (*fs_read_page)(fs_volume fs, fs_vnode v, iovecs *vecs, off_t pos);
	ssize_t (*fs_write_page)(fs_volume fs, fs_vnode v, iovecs *vecs, off_t pos);

	/* common operations */
	status_t (*fs_ioctl)(fs_volume fs, fs_vnode v, fs_cookie cookie, ulong op, void *buffer, size_t length);
	status_t (*fs_fsync)(fs_volume fs, fs_vnode v);

	status_t (*fs_read_link)(fs_volume fs, fs_vnode link, char *buffer, size_t bufferSize);
	status_t (*fs_write_link)(fs_volume fs, fs_vnode link, char *toPath);
	status_t (*fs_create_symlink)(fs_volume fs, fs_vnode dir, const char *name, const char *path, int mode);

	status_t (*fs_link)(fs_volume fs, fs_vnode dir, const char *name, fs_vnode vnode);
	status_t (*fs_unlink)(fs_volume fs, fs_vnode dir, const char *name);
	status_t (*fs_rename)(fs_volume fs, fs_vnode olddir, const char *oldname, fs_vnode newdir, const char *newname);

	status_t (*fs_access)(fs_volume fs, fs_vnode vnode, int mode);
	status_t (*fs_read_stat)(fs_volume fs, fs_vnode vnode, struct stat *stat);
	status_t (*fs_write_stat)(fs_volume fs, fs_vnode vnode, const struct stat *stat, int statMask);

	/* file operations */
	status_t (*fs_create)(fs_volume fs, fs_vnode dir, const char *name, int omode, int perms, fs_cookie *_cookie, vnode_id *_newVnodeID);
	status_t (*fs_open)(fs_volume fs, fs_vnode v, int oflags, fs_cookie *_cookie);
	status_t (*fs_close)(fs_volume fs, fs_vnode v, fs_cookie cookie);
	status_t (*fs_free_cookie)(fs_volume fs, fs_vnode v, fs_cookie cookie);
	ssize_t (*fs_read)(fs_volume fs, fs_vnode v, fs_cookie cookie, off_t pos, void *buffer, size_t *length);
	ssize_t (*fs_write)(fs_volume fs, fs_vnode v, fs_cookie cookie, off_t pos, const void *buffer, size_t *length);
	off_t (*fs_seek)(fs_volume fs, fs_vnode v, fs_cookie cookie, off_t pos, int seekType);

	/* directory operations */
	status_t (*fs_create_dir)(fs_volume fs, fs_vnode parent, const char *name, int perms, vnode_id *_newVnodeID);
	status_t (*fs_remove_dir)(fs_volume fs, fs_vnode parent, const char *name);
	status_t (*fs_open_dir)(fs_volume fs, fs_vnode vnode, fs_cookie *_cookie);
	status_t (*fs_close_dir)(fs_volume fs, fs_vnode vnode, fs_cookie cookie);
	status_t (*fs_free_dir_cookie)(fs_volume fs, fs_vnode vnode, fs_cookie cookie);
	status_t (*fs_read_dir)(fs_volume fs, fs_vnode vnode, fs_cookie cookie, struct dirent *buffer, size_t bufferSize, uint32 *_num);
	status_t (*fs_rewind_dir)(fs_volume fs, fs_vnode vnode, fs_cookie cookie);

	/* attribute directory operations */
	status_t (*fs_open_attr_dir)(fs_volume fs, fs_vnode vnode, fs_cookie *_cookie);
	status_t (*fs_close_attr_dir)(fs_volume fs, fs_vnode vnode, fs_cookie cookie);
	status_t (*fs_free_attr_dir_cookie)(fs_volume fs, fs_vnode vnode, fs_cookie cookie);
	status_t (*fs_read_attr_dir)(fs_volume fs, fs_vnode vnode, fs_cookie cookie, struct dirent *buffer, size_t bufferSize, uint32 *_num);
	status_t (*fs_rewind_attr_dir)(fs_volume fs, fs_vnode vnode, fs_cookie cookie);

	/* attribute operations */
	status_t (*fs_create_attr)(fs_volume fs, fs_vnode file, const char *name, vnode_id *_newAttrID);
	status_t (*fs_open_attr)(fs_volume fs, fs_vnode file, const char *name, int oflags, fs_cookie *_cookie, vnode_id *_attrID);
	status_t (*fs_close_attr)(fs_volume fs, fs_vnode attr, fs_cookie cookie);
	status_t (*fs_free_attr_cookie)(fs_volume fs, fs_vnode attr, fs_cookie cookie);
	ssize_t (*fs_read_attr)(fs_volume fs, fs_vnode attr, fs_cookie cookie, void *buffer, off_t pos, size_t *len);
	ssize_t (*fs_write_attr)(fs_volume fs, fs_vnode attr, fs_cookie cookie, const void *buffer, off_t pos, size_t *len);
	status_t (*fs_seek_attr)(fs_volume fs, fs_vnode attr, fs_cookie cookie, off_t pos, int seekType);

	status_t (*fs_rename_attr)(fs_volume fs, fs_vnode file, const char *oldName, const char *newName);
	status_t (*fs_remove_attr)(fs_volume fs, fs_vnode file, const char *name);
	status_t (*fs_read_attr_stat)(fs_volume fs, fs_vnode attr, fs_cookie cookie, struct stat *stat);
	status_t (*fs_write_attr_stat)(fs_volume fs, fs_vnode attr, fs_cookie cookie, struct stat *stat, int statMask);

	/* index directory & index operations */
	status_t (*fs_open_index_dir)(fs_volume fs, fs_vnode v, fs_cookie *cookie, int oflags);
	status_t (*fs_close_index_dir)(fs_volume fs, fs_vnode v, fs_cookie cookie);
	status_t (*fs_free_index_dir_cookie)(fs_volume fs, fs_vnode v, fs_cookie cookie);
	status_t (*fs_read_index_dir)(fs_volume fs, fs_vnode v, fs_cookie cookie, struct dirent *buffer, size_t bufferSize, uint32 *_num);
	status_t (*fs_rewind_index_dir)(fs_volume fs, fs_vnode v, fs_cookie cookie);

	status_t (*fs_create_index)(fs_volume fs, const char *name, uint32 type, int flags);
	status_t (*fs_remove_index)(fs_volume fs, const char *name);
	status_t (*fs_read_index_stat)(fs_volume fs, const char *name, struct stat *stat);

	/* query operations */
	status_t (*fs_open_query)(fs_volume fs, fs_vnode v, fs_cookie *cookie, int oflags);
	status_t (*fs_close_query)(fs_volume fs, fs_vnode v, fs_cookie cookie);
	status_t (*fs_free_query_cookie)(fs_volume fs, fs_vnode v, fs_cookie cookie);
	status_t (*fs_read_query)(fs_volume fs, fs_vnode v, fs_cookie cookie, struct dirent *buffer, size_t bufferSize, uint32 *_num);
	status_t (*fs_rewind_query)(fs_volume fs, fs_vnode v, fs_cookie cookie);
};

#ifdef __cplusplus
}
#endif

#endif	/* _FS_INTERFACE_H */
