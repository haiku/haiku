#ifndef _FSPROTO_H
#define _FSPROTO_H

#include <dirent.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iovec.h>

#include <OS.h>

#ifndef BUILDING_USERLAND_FS_SERVER
#	include <fs_attr.h>
#	include <fs_info.h>
#	include <fs_volume.h>
#endif	// ! BUILDING_USERLAND_FS_SERVER

#ifndef _IMPEXP_KERNEL
#define _IMPEXP_KERNEL
#endif

typedef dev_t		nspace_id;
typedef ino_t		vnode_id;

/*
 * PUBLIC PART OF THE FILE SYSTEM PROTOCOL
 */

#define		WSTAT_MODE		0x0001
#define		WSTAT_UID		0x0002
#define		WSTAT_GID		0x0004
#define		WSTAT_SIZE		0x0008
#define		WSTAT_ATIME		0x0010
#define		WSTAT_MTIME		0x0020
#define		WSTAT_CRTIME	0x0040

#define		WFSSTAT_NAME	0x0001

#define		B_ENTRY_CREATED		1
#define		B_ENTRY_REMOVED		2
#define		B_ENTRY_MOVED		3
#define		B_STAT_CHANGED		4
#define		B_ATTR_CHANGED		5
#define		B_DEVICE_MOUNTED	6
#define		B_DEVICE_UNMOUNTED	7

#define		B_STOP_WATCHING     0x0000
#define		B_WATCH_NAME		0x0001
#define		B_WATCH_STAT		0x0002
#define		B_WATCH_ATTR		0x0004
#define		B_WATCH_DIRECTORY	0x0008

#define		SELECT_READ			1
#define		SELECT_WRITE		2
#define 	SELECT_EXCEPTION	3

// missing ioctl() call added
#define		IOCTL_FILE_UNCACHED_IO	10000
#define		IOCTL_CREATE_TIME		10002
#define		IOCTL_MODIFIED_TIME		10003

#define		B_CUR_FS_API_VERSION	2

struct attr_info;
struct index_info;
typedef struct selectsync selectsync;

typedef int	op_read_vnode(void *ns, vnode_id vnid, char r, void **node);
typedef int	op_write_vnode(void *ns, void *node, char r);
typedef int	op_remove_vnode(void *ns, void *node, char r);
typedef int	op_secure_vnode(void *ns, void *node);

typedef int	op_walk(void *ns, void *base, const char *file, char **newpath,
					vnode_id *vnid);

typedef int	op_access(void *ns, void *node, int mode);

typedef int	op_create(void *ns, void *dir, const char *name,
					int omode, int perms, vnode_id *vnid, void **cookie);
typedef int	op_mkdir(void *ns, void *dir, const char *name,	int perms);
typedef int	op_symlink(void *ns, void *dir, const char *name,
					const char *path);
typedef int op_link(void *ns, void *dir, const char *name, void *node);

typedef int	op_rename(void *ns, void *olddir, const char *oldname,
					void *newdir, const char *newname);
typedef int	op_unlink(void *ns, void *dir, const char *name);
typedef int	op_rmdir(void *ns, void *dir, const char *name);

typedef int	op_readlink(void *ns, void *node, char *buf, size_t *bufsize);

typedef int op_opendir(void *ns, void *node, void **cookie);
typedef int	op_closedir(void *ns, void *node, void *cookie);
typedef int	op_rewinddir(void *ns, void *node, void *cookie);
typedef int	op_readdir(void *ns, void *node, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);

typedef int	op_open(void *ns, void *node, int omode, void **cookie);
typedef int	op_close(void *ns, void *node, void *cookie);
typedef int op_free_cookie(void *ns, void *node, void *cookie);
typedef int op_read(void *ns, void *node, void *cookie, off_t pos, void *buf,
					size_t *len);
typedef int op_write(void *ns, void *node, void *cookie, off_t pos,
					const void *buf, size_t *len);
typedef int op_readv(void *ns, void *node, void *cookie, off_t pos, const iovec *vec,
					size_t count, size_t *len);
typedef int op_writev(void *ns, void *node, void *cookie, off_t pos, const iovec *vec,
					size_t count, size_t *len);
typedef int	op_ioctl(void *ns, void *node, void *cookie, int cmd, void *buf,
					size_t len);
typedef	int	op_setflags(void *ns, void *node, void *cookie, int flags);

typedef int	op_rstat(void *ns, void *node, struct stat *);
typedef int op_wstat(void *ns, void *node, struct stat *, long mask);
typedef int	op_fsync(void *ns, void *node);

typedef int	op_select(void *ns, void *node, void *cookie, uint8 event,
				uint32 ref, selectsync *sync);
typedef int	op_deselect(void *ns, void *node, void *cookie, uint8 event,
				selectsync *sync);

typedef int	op_initialize(const char *devname, void *parms, size_t len);
typedef int	op_mount(nspace_id nsid, const char *devname, ulong flags,
					void *parms, size_t len, void **data, vnode_id *vnid);
typedef int	op_unmount(void *ns);
typedef int	op_sync(void *ns);
typedef int op_rfsstat(void *ns, struct fs_info *);
typedef int op_wfsstat(void *ns, struct fs_info *, long mask);


typedef int	op_open_attrdir(void *ns, void *node, void **cookie);
typedef int	op_close_attrdir(void *ns, void *node, void *cookie);
typedef int	op_rewind_attrdir(void *ns, void *node, void *cookie);
typedef int	op_read_attrdir(void *ns, void *node, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);
typedef int	op_remove_attr(void *ns, void *node, const char *name);
typedef	int	op_rename_attr(void *ns, void *node, const char *oldname,
					const char *newname);
typedef int	op_stat_attr(void *ns, void *node, const char *name,
					struct attr_info *buf);

typedef int	op_write_attr(void *ns, void *node, const char *name, int type,
					const void *buf, size_t *len, off_t pos);
typedef int	op_read_attr(void *ns, void *node, const char *name, int type,
					void *buf, size_t *len, off_t pos);

typedef int	op_open_indexdir(void *ns, void **cookie);
typedef int	op_close_indexdir(void *ns, void *cookie);
typedef int	op_rewind_indexdir(void *ns, void *cookie);
typedef int	op_read_indexdir(void *ns, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);
typedef int	op_create_index(void *ns, const char *name, int type, int flags);
typedef int	op_remove_index(void *ns, const char *name);
typedef	int	op_rename_index(void *ns, const char *oldname, 
					const char *newname);
typedef int	op_stat_index(void *ns, const char *name, struct index_info *buf);

typedef int	op_open_query(void *ns, const char *query, ulong flags,
					port_id port, long token, void **cookie);
typedef int	op_close_query(void *ns, void *cookie);
typedef int	op_read_query(void *ns, void *cookie, long *num,
					struct dirent *buf, size_t bufsize);

typedef struct vnode_ops {
	op_read_vnode			*read_vnode;
	op_write_vnode			*write_vnode;
	op_remove_vnode			*remove_vnode;
	op_secure_vnode			*secure_vnode;
	op_walk					*walk;
	op_access				*access;
	op_create				*create;
	op_mkdir				*mkdir;
	op_symlink				*symlink;
	op_link					*link;
	op_rename				*rename;
	op_unlink				*unlink;
	op_rmdir				*rmdir;
	op_readlink				*readlink;
	op_opendir				*opendir;
	op_closedir				*closedir;
	op_free_cookie			*free_dircookie;
	op_rewinddir			*rewinddir;
	op_readdir				*readdir;
	op_open					*open;
	op_close				*close;
	op_free_cookie			*free_cookie;
	op_read					*read;
	op_write				*write;
	op_readv				*readv;
	op_writev				*writev;
	op_ioctl				*ioctl;
	op_setflags				*setflags;
	op_rstat				*rstat;
	op_wstat				*wstat;
	op_fsync				*fsync;
	op_initialize			*initialize;
	op_mount				*mount;
	op_unmount				*unmount;
	op_sync					*sync;
	op_rfsstat				*rfsstat;
	op_wfsstat				*wfsstat;
	op_select				*select;
	op_deselect				*deselect;
	op_open_indexdir		*open_indexdir;
	op_close_indexdir		*close_indexdir;
	op_free_cookie			*free_indexdircookie;
	op_rewind_indexdir		*rewind_indexdir;
	op_read_indexdir		*read_indexdir;
	op_create_index			*create_index;
	op_remove_index			*remove_index;
	op_rename_index			*rename_index;
	op_stat_index			*stat_index;
	op_open_attrdir			*open_attrdir;
	op_close_attrdir		*close_attrdir;
	op_free_cookie			*free_attrdircookie;
	op_rewind_attrdir		*rewind_attrdir;
	op_read_attrdir			*read_attrdir;
	op_write_attr			*write_attr;
	op_read_attr			*read_attr;
	op_remove_attr			*remove_attr;
	op_rename_attr			*rename_attr;
	op_stat_attr			*stat_attr;
	op_open_query			*open_query;
	op_close_query			*close_query;
	op_free_cookie			*free_querycookie;
	op_read_query			*read_query;
} vnode_ops;

#ifdef __cplusplus
	extern "C" {
#endif

extern _IMPEXP_KERNEL int	new_path(const char *path, char **copy);
extern _IMPEXP_KERNEL void	free_path(char *p);

extern _IMPEXP_KERNEL int	notify_listener(int op, nspace_id nsid,
									vnode_id vnida,	vnode_id vnidb,
									vnode_id vnidc, const char *name);
extern _IMPEXP_KERNEL void	notify_select_event(selectsync *sync, uint32 ref);
extern _IMPEXP_KERNEL int	send_notification(port_id port, long token,
									ulong what, long op, nspace_id nsida,
									nspace_id nsidb, vnode_id vnida,
									vnode_id vnidb, vnode_id vnidc,
									const char *name);
extern _IMPEXP_KERNEL int	get_vnode(nspace_id nsid, vnode_id vnid, void **data);
extern _IMPEXP_KERNEL int	put_vnode(nspace_id nsid, vnode_id vnid);
extern _IMPEXP_KERNEL int	new_vnode(nspace_id nsid, vnode_id vnid, void *data);
extern _IMPEXP_KERNEL int	remove_vnode(nspace_id nsid, vnode_id vnid);
extern _IMPEXP_KERNEL int	unremove_vnode(nspace_id nsid, vnode_id vnid);
extern _IMPEXP_KERNEL int	is_vnode_removed(nspace_id nsid, vnode_id vnid);


extern _EXPORT vnode_ops	fs_entry;
extern _EXPORT int32		api_version;

#ifdef __cplusplus
	}	// extern "C"
#endif

#endif
