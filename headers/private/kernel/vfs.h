/*
 * Copyright 2002-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H


#include <kernel.h>
#include <lock.h>
#include <util/list.h>

#include <fs_interface.h>

#include <dirent.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <vfs_defs.h>


#define DEFAULT_FD_TABLE_SIZE	256
#define MAX_FD_TABLE_SIZE		8192
#define DEFAULT_NODE_MONITORS	4096
#define MAX_NODE_MONITORS		65536

#define B_UNMOUNT_BUSY_PARTITION	0x80000000

struct attr_info;
struct file_descriptor;
struct generic_io_vec;
struct kernel_args;
struct net_stat;
struct pollfd;
struct rlimit;
struct selectsync;
struct select_info;
struct VMCache;
struct vnode;


/** The I/O context of a process/team, holds the fd array among others */
typedef struct io_context {
	struct vnode *root;
	struct vnode *cwd;
	mutex		io_mutex;
	int32		ref_count;
	uint32		table_size;
	uint32		num_used_fds;
	struct file_descriptor **fds;
	struct select_info **select_infos;
	uint8		*fds_close_on_exec;
	struct list node_monitors;
	uint32		num_monitors;
	uint32		max_monitors;
} io_context;


#ifdef __cplusplus
extern "C" {
#endif

status_t	vfs_init(struct kernel_args *args);
status_t	vfs_bootstrap_file_systems(void);
void		vfs_mount_boot_file_system(struct kernel_args *args);
void		vfs_exec_io_context(io_context *context);
io_context*	vfs_new_io_context(io_context* parentContext,
				bool purgeCloseOnExec);
void		vfs_get_io_context(io_context *context);
void		vfs_put_io_context(io_context *context);

int			vfs_getrlimit(int resource, struct rlimit *rlp);
int			vfs_setrlimit(int resource, const struct rlimit *rlp);

/* calls needed by the VM for paging and by the file cache */
status_t	vfs_get_vnode_from_fd(int fd, bool kernel, struct vnode **_vnode);
status_t	vfs_get_vnode_from_path(const char *path, bool kernel,
				struct vnode **_vnode);
status_t	vfs_get_vnode(dev_t mountID, ino_t vnodeID, bool canWait,
				struct vnode **_vnode);
status_t	vfs_entry_ref_to_vnode(dev_t mountID, ino_t directoryID,
				const char *name, struct vnode **_vnode);
void		vfs_vnode_to_node_ref(struct vnode *vnode, dev_t *_mountID,
				ino_t *_vnodeID);
struct fs_vnode* vfs_fsnode_for_vnode(struct vnode* vnode);

int			vfs_open_vnode(struct vnode* vnode, int openMode, bool kernel);
status_t	vfs_lookup_vnode(dev_t mountID, ino_t vnodeID,
				struct vnode **_vnode);
void		vfs_put_vnode(struct vnode *vnode);
void		vfs_acquire_vnode(struct vnode *vnode);
status_t	vfs_get_cookie_from_fd(int fd, void **_cookie);
bool		vfs_can_page(struct vnode *vnode, void *cookie);
status_t	vfs_read_pages(struct vnode *vnode, void *cookie, off_t pos,
				const struct generic_io_vec *vecs, size_t count, uint32 flags,
				generic_size_t *_numBytes);
status_t	vfs_write_pages(struct vnode *vnode, void *cookie, off_t pos,
				const struct generic_io_vec *vecs, size_t count, uint32 flags,
				generic_size_t *_numBytes);
status_t	vfs_vnode_io(struct vnode* vnode, void* cookie,
				io_request* request);
status_t	vfs_synchronous_io(io_request* request,
				status_t (*doIO)(void* cookie, off_t offset, void* buffer,
					size_t* length),
				void* cookie);
status_t	vfs_get_vnode_cache(struct vnode *vnode, struct VMCache **_cache,
				bool allocate);
status_t	vfs_get_file_map(struct vnode *vnode, off_t offset, size_t size,
				struct file_io_vec *vecs, size_t *_count);
status_t	vfs_get_fs_node_from_path(fs_volume *volume, const char *path,
				bool traverseLeafLink, bool kernel, void **_node);
status_t	vfs_stat_vnode(struct vnode *vnode, struct stat *stat);
status_t	vfs_stat_node_ref(dev_t device, ino_t inode, struct stat *stat);
status_t	vfs_get_vnode_name(struct vnode *vnode, char *name,
				size_t nameSize);
status_t	vfs_entry_ref_to_path(dev_t device, ino_t inode, const char *leaf,
				char *path, size_t pathLength);
status_t	vfs_get_cwd(dev_t *_mountID, ino_t *_vnodeID);
void		vfs_unlock_vnode_if_locked(struct file_descriptor *descriptor);
status_t	vfs_unmount(dev_t mountID, uint32 flags);
status_t	vfs_disconnect_vnode(dev_t mountID, ino_t vnodeID);
void		vfs_free_unused_vnodes(int32 level);

status_t	vfs_read_stat(int fd, const char *path, bool traverseLeafLink,
				struct stat *stat, bool kernel);

/* special module convenience call */
status_t	vfs_get_module_path(const char *basePath, const char *moduleName,
				char *pathBuffer, size_t bufferSize);

/* service call for whoever needs a normalized path */
status_t	vfs_normalize_path(const char *path, char *buffer,
				size_t bufferSize, bool traverseLink, bool kernel);

/* service call for whoever wants to create a special node */
status_t	vfs_create_special_node(const char *path, fs_vnode *subVnode,
				mode_t mode, uint32 flags, bool kernel, fs_vnode *_superVnode,
				struct vnode **_createdVnode);

/* service call for the node monitor */
status_t	resolve_mount_point_to_volume_root(dev_t mountID, ino_t nodeID,
				dev_t *resolvedMountID, ino_t *resolvedNodeID);

/* calls the syscall dispatcher should use for user file I/O */
dev_t		_user_mount(const char *path, const char *device,
				const char *fs_name, uint32 flags, const char *args,
				size_t argsLength);
status_t	_user_unmount(const char *path, uint32 flags);
status_t	_user_read_fs_info(dev_t device, struct fs_info *info);
status_t	_user_write_fs_info(dev_t device, const struct fs_info *info,
				int mask);
dev_t		_user_next_device(int32 *_cookie);
status_t	_user_sync(void);
status_t	_user_get_next_fd_info(team_id team, uint32 *cookie,
				struct fd_info *info, size_t infoSize);
status_t	_user_entry_ref_to_path(dev_t device, ino_t inode, const char *leaf,
				char *userPath, size_t pathLength);
status_t	_user_normalize_path(const char* userPath, bool traverseLink,
				char* buffer);
int			_user_open_entry_ref(dev_t device, ino_t inode, const char *name,
				int openMode, int perms);
int			_user_open(int fd, const char *path, int openMode, int perms);
int			_user_open_dir_node_ref(dev_t device, ino_t inode);
int			_user_open_dir_entry_ref(dev_t device, ino_t inode,
				const char *name);
int			_user_open_dir(int fd, const char *path);
int			_user_open_parent_dir(int fd, char *name, size_t nameLength);
status_t	_user_fcntl(int fd, int op, uint32 argument);
status_t	_user_fsync(int fd);
status_t	_user_flock(int fd, int op);
status_t	_user_read_stat(int fd, const char *path, bool traverseLink,
				struct stat *stat, size_t statSize);
status_t	_user_write_stat(int fd, const char *path, bool traverseLink,
				const struct stat *stat, size_t statSize, int statMask);
off_t		_user_seek(int fd, off_t pos, int seekType);
status_t	_user_create_dir_entry_ref(dev_t device, ino_t inode,
				const char *name, int perms);
status_t	_user_create_dir(int fd, const char *path, int perms);
status_t	_user_remove_dir(int fd, const char *path);
status_t	_user_read_link(int fd, const char *path, char *buffer,
				size_t *_bufferSize);
status_t	_user_write_link(const char *path, const char *toPath);
status_t	_user_create_symlink(int fd, const char *path, const char *toPath,
				int mode);
status_t	_user_create_link(int pathFD, const char *path, int toFD,
				const char *toPath, bool traverseLeafLink);
status_t	_user_unlink(int fd, const char *path);
status_t	_user_rename(int oldFD, const char *oldpath, int newFD,
				const char *newpath);
status_t	_user_create_fifo(int fd, const char *path, mode_t perms);
status_t	_user_create_pipe(int *fds);
status_t	_user_access(int fd, const char *path, int mode,
				bool effectiveUserGroup);
ssize_t		_user_select(int numfds, fd_set *readSet, fd_set *writeSet,
				fd_set *errorSet, bigtime_t timeout, const sigset_t *sigMask);
ssize_t		_user_poll(struct pollfd *fds, int numfds, bigtime_t timeout);
int			_user_open_attr_dir(int fd, const char *path,
				bool traverseLeafLink);
ssize_t		_user_read_attr(int fd, const char *attribute, off_t pos,
				void *buffer, size_t readBytes);
ssize_t		_user_write_attr(int fd, const char *attribute, uint32 type,
				off_t pos, const void *buffer, size_t readBytes);
status_t	_user_stat_attr(int fd, const char *attribute,
				struct attr_info *attrInfo);
int			_user_open_attr(int fd, const char* path, const char *name,
				uint32 type, int openMode);
status_t	_user_remove_attr(int fd, const char *name);
status_t	_user_rename_attr(int fromFile, const char *fromName, int toFile,
				const char *toName);
int			_user_open_index_dir(dev_t device);
status_t	_user_create_index(dev_t device, const char *name, uint32 type,
				uint32 flags);
status_t	_user_read_index_stat(dev_t device, const char *name,
				struct stat *stat);
status_t	_user_remove_index(dev_t device, const char *name);
status_t	_user_getcwd(char *buffer, size_t size);
status_t	_user_setcwd(int fd, const char *path);
status_t	_user_change_root(const char *path);
int			_user_open_query(dev_t device, const char *query,
				size_t queryLength, uint32 flags, port_id port, int32 token);

/* fd user prototypes (implementation located in fd.cpp)  */
ssize_t		_user_read(int fd, off_t pos, void *buffer, size_t bufferSize);
ssize_t		_user_readv(int fd, off_t pos, const iovec *vecs, size_t count);
ssize_t		_user_write(int fd, off_t pos, const void *buffer,
				size_t bufferSize);
ssize_t		_user_writev(int fd, off_t pos, const iovec *vecs, size_t count);
status_t	_user_ioctl(int fd, uint32 cmd, void *data, size_t length);
ssize_t		_user_read_dir(int fd, struct dirent *buffer, size_t bufferSize,
				uint32 maxCount);
status_t	_user_rewind_dir(int fd);
status_t	_user_close(int fd);
int			_user_dup(int fd);
int			_user_dup2(int ofd, int nfd);
status_t	_user_lock_node(int fd);
status_t	_user_unlock_node(int fd);

/* socket user prototypes (implementation in socket.cpp) */
int			_user_socket(int family, int type, int protocol);
status_t	_user_bind(int socket, const struct sockaddr *address,
				socklen_t addressLength);
status_t	_user_shutdown_socket(int socket, int how);
status_t	_user_connect(int socket, const struct sockaddr *address,
				socklen_t addressLength);
status_t	_user_listen(int socket, int backlog);
int			_user_accept(int socket, struct sockaddr *address,
				socklen_t *_addressLength);
ssize_t		_user_recv(int socket, void *data, size_t length, int flags);
ssize_t		_user_recvfrom(int socket, void *data, size_t length, int flags,
				struct sockaddr *address, socklen_t *_addressLength);
ssize_t		_user_recvmsg(int socket, struct msghdr *message, int flags);
ssize_t		_user_send(int socket, const void *data, size_t length, int flags);
ssize_t		_user_sendto(int socket, const void *data, size_t length, int flags,
				const struct sockaddr *address, socklen_t addressLength);
ssize_t		_user_sendmsg(int socket, const struct msghdr *message, int flags);
status_t	_user_getsockopt(int socket, int level, int option, void *value,
				socklen_t *_length);
status_t	_user_setsockopt(int socket, int level, int option,
				const void *value, socklen_t length);
status_t	_user_getpeername(int socket, struct sockaddr *address,
				socklen_t *_addressLength);
status_t	_user_getsockname(int socket, struct sockaddr *address,
				socklen_t *_addressLength);
int			_user_sockatmark(int socket);
status_t	_user_socketpair(int family, int type, int protocol,
				int *socketVector);
status_t	_user_get_next_socket_stat(int family, uint32 *cookie,
				struct net_stat *stat);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

class AsyncIOCallback {
public:
	virtual						~AsyncIOCallback();

	virtual	void				IOFinished(status_t status,
									bool partialTransfer,
									generic_size_t bytesTransferred) = 0;

	static	status_t 			IORequestCallback(void* data,
									io_request* request, status_t status,
									bool partialTransfer,
									generic_size_t transferEndOffset);
};


class StackableAsyncIOCallback : public AsyncIOCallback {
public:
								StackableAsyncIOCallback(AsyncIOCallback* next);

protected:
			AsyncIOCallback*	fNextCallback;
};


status_t	vfs_asynchronous_read_pages(struct vnode* vnode, void* cookie,
				off_t pos, const struct generic_io_vec* vecs, size_t count,
				generic_size_t numBytes, uint32 flags,
				AsyncIOCallback* callback);

status_t	vfs_asynchronous_write_pages(struct vnode* vnode, void* cookie,
				off_t pos, const struct generic_io_vec* vecs, size_t count,
				generic_size_t numBytes, uint32 flags,
				AsyncIOCallback* callback);

#endif	// __cplusplus

#endif	/* _KERNEL_VFS_H */
