// beos_fs_interface.h

#ifndef USERLAND_FS_BEOS_FS_INTERFACE_H
#define USERLAND_FS_BEOS_FS_INTERFACE_H

#include <fs_interface.h>


// BeOS FS API version
#define		BEOS_FS_API_VERSION	2


/* helper struct allowing us to avoid problems with the st_*time macros */
typedef struct {
    time_t  tv_sec;
} stat_beos_time;


// BeOS structures

typedef struct beos_iovec {
	void	*iov_base;
	size_t	iov_len;
} beos_iovec;

typedef struct beos_dirent {
	dev_t			d_dev;
	dev_t			d_pdev;
	ino_t			d_ino;
	ino_t			d_pino;
	unsigned short	d_reclen;
	char			d_name[1];
} beos_dirent_t;

struct beos_stat {
    dev_t			st_dev;
    ino_t			st_ino;
    mode_t			st_mode;
    nlink_t			st_nlink;
    uid_t			st_uid;
    gid_t			st_gid;
    off_t			st_size;
    dev_t			st_rdev;
    size_t			st_blksize;
    stat_beos_time	st_atim;
    stat_beos_time	st_mtim;
    stat_beos_time	st_ctim;
    stat_beos_time	st_crtim;
};

struct beos_fs_info {
	dev_t	dev;
	ino_t	root;
	uint32	flags;
	off_t	block_size;
	off_t	io_size;
	off_t	total_blocks;
	off_t	free_blocks;
	off_t	total_nodes;
	off_t	free_nodes;
	char	device_name[128];
	char	volume_name[B_FILE_NAME_LENGTH];
	char	fsh_name[B_OS_NAME_LENGTH];
};

typedef struct beos_attr_info {
	uint32     type;
	off_t      size;
} beos_attr_info;

typedef struct beos_index_info {
	uint32     type;
	off_t      size;
	time_t     modification_time;
	time_t     creation_time;
	uid_t      uid;
	gid_t      gid;
} beos_index_info;


// FS interface hook types

typedef int	beos_op_read_vnode(void *ns, ino_t vnid, char r, void **node);
typedef int	beos_op_write_vnode(void *ns, void *node, char r);
typedef int	beos_op_remove_vnode(void *ns, void *node, char r);
typedef int	beos_op_secure_vnode(void *ns, void *node);

typedef int	beos_op_walk(void *ns, void *base, const char *file, char **newpath,
					ino_t *vnid);

typedef int	beos_op_access(void *ns, void *node, int mode);

typedef int	beos_op_create(void *ns, void *dir, const char *name,
					int omode, int perms, ino_t *vnid, void **cookie);
typedef int	beos_op_mkdir(void *ns, void *dir, const char *name, int perms);
typedef int	beos_op_symlink(void *ns, void *dir, const char *name,
					const char *path);
typedef int beos_op_link(void *ns, void *dir, const char *name, void *node);

typedef int	beos_op_rename(void *ns, void *olddir, const char *oldname,
					void *newdir, const char *newname);
typedef int	beos_op_unlink(void *ns, void *dir, const char *name);
typedef int	beos_op_rmdir(void *ns, void *dir, const char *name);

typedef int	beos_op_readlink(void *ns, void *node, char *buf, size_t *bufsize);

typedef int beos_op_opendir(void *ns, void *node, void **cookie);
typedef int	beos_op_closedir(void *ns, void *node, void *cookie);
typedef int	beos_op_rewinddir(void *ns, void *node, void *cookie);
typedef int	beos_op_readdir(void *ns, void *node, void *cookie, long *num,
					struct beos_dirent *buf, size_t bufsize);

typedef int	beos_op_open(void *ns, void *node, int omode, void **cookie);
typedef int	beos_op_close(void *ns, void *node, void *cookie);
typedef int beos_op_free_cookie(void *ns, void *node, void *cookie);
typedef int beos_op_read(void *ns, void *node, void *cookie, off_t pos,
					void *buf, size_t *len);
typedef int beos_op_write(void *ns, void *node, void *cookie, off_t pos,
					const void *buf, size_t *len);
typedef int beos_op_readv(void *ns, void *node, void *cookie, off_t pos,
					const beos_iovec *vec, size_t count, size_t *len);
typedef int beos_op_writev(void *ns, void *node, void *cookie, off_t pos,
					const beos_iovec *vec, size_t count, size_t *len);
typedef int	beos_op_ioctl(void *ns, void *node, void *cookie, int cmd,
					void *buf, size_t len);
typedef	int	beos_op_setflags(void *ns, void *node, void *cookie, int flags);

typedef int	beos_op_rstat(void *ns, void *node, struct beos_stat *);
typedef int beos_op_wstat(void *ns, void *node, const struct beos_stat *,
					long mask);
typedef int	beos_op_fsync(void *ns, void *node);

typedef int	beos_op_select(void *ns, void *node, void *cookie, uint8 event,
					uint32 ref, selectsync *sync);
typedef int	beos_op_deselect(void *ns, void *node, void *cookie, uint8 event,
					selectsync *sync);

typedef int	beos_op_initialize(const char *devname, void *parms, size_t len);
typedef int	beos_op_mount(dev_t nsid, const char *devname, ulong flags,
					void *parms, size_t len, void **data, ino_t *vnid);
typedef int	beos_op_unmount(void *ns);
typedef int	beos_op_sync(void *ns);
typedef int beos_op_rfsstat(void *ns, struct beos_fs_info *);
typedef int beos_op_wfsstat(void *ns, struct beos_fs_info *, long mask);

typedef int	beos_op_open_attrdir(void *ns, void *node, void **cookie);
typedef int	beos_op_close_attrdir(void *ns, void *node, void *cookie);
typedef int	beos_op_rewind_attrdir(void *ns, void *node, void *cookie);
typedef int	beos_op_read_attrdir(void *ns, void *node, void *cookie, long *num,
					struct beos_dirent *buf, size_t bufsize);
typedef int	beos_op_remove_attr(void *ns, void *node, const char *name);
typedef	int	beos_op_rename_attr(void *ns, void *node, const char *oldname,
					const char *newname);
typedef int	beos_op_stat_attr(void *ns, void *node, const char *name,
					struct beos_attr_info *buf);

typedef int	beos_op_write_attr(void *ns, void *node, const char *name, int type,
					const void *buf, size_t *len, off_t pos);
typedef int	beos_op_read_attr(void *ns, void *node, const char *name, int type,
					void *buf, size_t *len, off_t pos);

typedef int	beos_op_open_indexdir(void *ns, void **cookie);
typedef int	beos_op_close_indexdir(void *ns, void *cookie);
typedef int	beos_op_rewind_indexdir(void *ns, void *cookie);
typedef int	beos_op_read_indexdir(void *ns, void *cookie, long *num,
					struct beos_dirent *buf, size_t bufsize);
typedef int	beos_op_create_index(void *ns, const char *name, int type,
					int flags);
typedef int	beos_op_remove_index(void *ns, const char *name);
typedef	int	beos_op_rename_index(void *ns, const char *oldname,
					const char *newname);
typedef int	beos_op_stat_index(void *ns, const char *name,
					struct beos_index_info *buf);

typedef int	beos_op_open_query(void *ns, const char *query, ulong flags,
					port_id port, long token, void **cookie);
typedef int	beos_op_close_query(void *ns, void *cookie);
typedef int	beos_op_read_query(void *ns, void *cookie, long *num,
					struct beos_dirent *buf, size_t bufsize);


// the FS interface structure

typedef struct beos_vnode_ops {
	beos_op_read_vnode			*read_vnode;
	beos_op_write_vnode			*write_vnode;
	beos_op_remove_vnode		*remove_vnode;
	beos_op_secure_vnode		*secure_vnode;
	beos_op_walk				*walk;
	beos_op_access				*access;
	beos_op_create				*create;
	beos_op_mkdir				*mkdir;
	beos_op_symlink				*symlink;
	beos_op_link				*link;
	beos_op_rename				*rename;
	beos_op_unlink				*unlink;
	beos_op_rmdir				*rmdir;
	beos_op_readlink			*readlink;
	beos_op_opendir				*opendir;
	beos_op_closedir			*closedir;
	beos_op_free_cookie			*free_dircookie;
	beos_op_rewinddir			*rewinddir;
	beos_op_readdir				*readdir;
	beos_op_open				*open;
	beos_op_close				*close;
	beos_op_free_cookie			*free_cookie;
	beos_op_read				*read;
	beos_op_write				*write;
	beos_op_readv				*readv;
	beos_op_writev				*writev;
	beos_op_ioctl				*ioctl;
	beos_op_setflags			*setflags;
	beos_op_rstat				*rstat;
	beos_op_wstat				*wstat;
	beos_op_fsync				*fsync;
	beos_op_initialize			*initialize;
	beos_op_mount				*mount;
	beos_op_unmount				*unmount;
	beos_op_sync				*sync;
	beos_op_rfsstat				*rfsstat;
	beos_op_wfsstat				*wfsstat;
	beos_op_select				*select;
	beos_op_deselect			*deselect;
	beos_op_open_indexdir		*open_indexdir;
	beos_op_close_indexdir		*close_indexdir;
	beos_op_free_cookie			*free_indexdircookie;
	beos_op_rewind_indexdir		*rewind_indexdir;
	beos_op_read_indexdir		*read_indexdir;
	beos_op_create_index		*create_index;
	beos_op_remove_index		*remove_index;
	beos_op_rename_index		*rename_index;
	beos_op_stat_index			*stat_index;
	beos_op_open_attrdir		*open_attrdir;
	beos_op_close_attrdir		*close_attrdir;
	beos_op_free_cookie			*free_attrdircookie;
	beos_op_rewind_attrdir		*rewind_attrdir;
	beos_op_read_attrdir		*read_attrdir;
	beos_op_write_attr			*write_attr;
	beos_op_read_attr			*read_attr;
	beos_op_remove_attr			*remove_attr;
	beos_op_rename_attr			*rename_attr;
	beos_op_stat_attr			*stat_attr;
	beos_op_open_query			*open_query;
	beos_op_close_query			*close_query;
	beos_op_free_cookie			*free_querycookie;
	beos_op_read_query			*read_query;
} beos_vnode_ops;

#endif	// USERLAND_FS_BEOS_FS_INTERFACE_H
