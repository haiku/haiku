/*
 * Copyright 2004-2016, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_FS_INTERFACE_H
#define _FSSH_FS_INTERFACE_H

/*! File System Interface Layer Definition */


#include "fssh_disk_device_defs.h"
#include "fssh_module.h"
#include "fssh_os.h"


struct fssh_dirent;
struct fssh_fs_info;
struct fssh_iovec;
struct fssh_partition_data;
struct fssh_selectsync;
struct fssh_stat;

typedef fssh_dev_t fssh_mount_id;
typedef fssh_ino_t fssh_vnode_id;

/* the file system's private data structures */
typedef void *fssh_fs_cookie;

typedef struct FSSHIORequest fssh_io_request;

/* additional flags passed to write_stat() */
#define FSSH_B_STAT_SIZE_INSECURE	0x2000

/* passed to write_fs_info() */
#define	FSSH_FS_WRITE_FSINFO_NAME	0x0001

struct fssh_file_io_vec {
	fssh_off_t	offset;
	fssh_off_t	length;
};

#define	FSSH_B_CURRENT_FS_API_VERSION "/v1"

// flags for publish_vnode() and fs_volume_ops::get_vnode()
#define FSSH_B_VNODE_PUBLISH_REMOVED				0x01
#define FSSH_B_VNODE_DONT_CREATE_SPECIAL_SUB_NODE	0x02


#ifdef __cplusplus
extern "C" {
#endif

typedef struct fssh_fs_volume fssh_fs_volume;
typedef struct fssh_fs_volume_ops fssh_fs_volume_ops;
typedef struct fssh_fs_vnode fssh_fs_vnode;
typedef struct fssh_fs_vnode_ops fssh_fs_vnode_ops;


struct fssh_fs_volume {
	fssh_dev_t			id;
	int32_t				layer;
	void*				private_volume;
	fssh_fs_volume_ops*	ops;
	fssh_fs_volume*		sub_volume;
	fssh_fs_volume*		super_volume;
};

struct fssh_fs_vnode {
	void*				private_node;
	fssh_fs_vnode_ops*	ops;
};

struct fssh_fs_volume_ops {
	fssh_status_t (*unmount)(fssh_fs_volume *volume);

	fssh_status_t (*read_fs_info)(fssh_fs_volume *volume,
				struct fssh_fs_info *info);
	fssh_status_t (*write_fs_info)(fssh_fs_volume *volume,
				const struct fssh_fs_info *info, uint32_t mask);
	fssh_status_t (*sync)(fssh_fs_volume *volume);

	fssh_status_t (*get_vnode)(fssh_fs_volume *volume, fssh_vnode_id id,
				fssh_fs_vnode *_vnode, int *_type, uint32_t *_flags,
				bool reenter);

	/* index directory & index operations */
	fssh_status_t (*open_index_dir)(fssh_fs_volume *volume,
				fssh_fs_cookie *cookie);
	fssh_status_t (*close_index_dir)(fssh_fs_volume *volume,
				fssh_fs_cookie cookie);
	fssh_status_t (*free_index_dir_cookie)(fssh_fs_volume *volume,
				fssh_fs_cookie cookie);
	fssh_status_t (*read_index_dir)(fssh_fs_volume *volume,
				fssh_fs_cookie cookie, struct fssh_dirent *buffer,
				fssh_size_t bufferSize, uint32_t *_num);
	fssh_status_t (*rewind_index_dir)(fssh_fs_volume *volume,
				fssh_fs_cookie cookie);

	fssh_status_t (*create_index)(fssh_fs_volume *volume, const char *name,
				uint32_t type, uint32_t flags);
	fssh_status_t (*remove_index)(fssh_fs_volume *volume, const char *name);
	fssh_status_t (*read_index_stat)(fssh_fs_volume *volume, const char *name,
				struct fssh_stat *stat);

	/* query operations */
	fssh_status_t (*open_query)(fssh_fs_volume *volume, const char *query,
				uint32_t flags, fssh_port_id port, uint32_t token,
				fssh_fs_cookie *_cookie);
	fssh_status_t (*close_query)(fssh_fs_volume *volume, fssh_fs_cookie cookie);
	fssh_status_t (*free_query_cookie)(fssh_fs_volume *volume,
				fssh_fs_cookie cookie);
	fssh_status_t (*read_query)(fssh_fs_volume *volume, fssh_fs_cookie cookie,
				struct fssh_dirent *buffer, fssh_size_t bufferSize,
				uint32_t *_num);
	fssh_status_t (*rewind_query)(fssh_fs_volume *volume,
				fssh_fs_cookie cookie);

	/* support for FS layers */
	fssh_status_t (*create_sub_vnode)(fssh_fs_volume *volume, fssh_ino_t id,
		fssh_fs_vnode *vnode);
	fssh_status_t (*delete_sub_vnode)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode);
};

struct fssh_fs_vnode_ops {
	/* vnode operations */
	fssh_status_t (*lookup)(fssh_fs_volume *volume, fssh_fs_vnode *dir,
				const char *name, fssh_vnode_id *_id);
	fssh_status_t (*get_vnode_name)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode, char *buffer, fssh_size_t bufferSize);

	fssh_status_t (*put_vnode)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				bool reenter);
	fssh_status_t (*remove_vnode)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				bool reenter);

	/* VM file access */
	bool (*can_page)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie);
	fssh_status_t (*read_pages)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, fssh_off_t pos, const fssh_iovec *vecs,
				fssh_size_t count, fssh_size_t *_numBytes);
	fssh_status_t (*write_pages)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, fssh_off_t pos, const fssh_iovec *vecs,
				fssh_size_t count, fssh_size_t *_numBytes);

	/* asynchronous I/O */
	fssh_status_t (*io)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				void *cookie, fssh_io_request *request);
	fssh_status_t (*cancel_io)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				void *cookie, fssh_io_request *request);

	/* cache file access */
	fssh_status_t (*get_file_map)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_off_t offset, fssh_size_t size,
				struct fssh_file_io_vec *vecs, fssh_size_t *_count);

	/* common operations */
	fssh_status_t (*ioctl)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, uint32_t op, void *buffer,
				fssh_size_t length);
	fssh_status_t (*set_flags)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, int flags);
	fssh_status_t (*select)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, uint8_t event, fssh_selectsync *sync);
	fssh_status_t (*deselect)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, uint8_t event, fssh_selectsync *sync);
	fssh_status_t (*fsync)(fssh_fs_volume *volume, fssh_fs_vnode *vnode);

	fssh_status_t (*read_symlink)(fssh_fs_volume *volume, fssh_fs_vnode *link,
				char *buffer, fssh_size_t *_bufferSize);
	fssh_status_t (*create_symlink)(fssh_fs_volume *volume, fssh_fs_vnode *dir,
				const char *name, const char *path, int mode);

	fssh_status_t (*link)(fssh_fs_volume *volume, fssh_fs_vnode *dir,
				const char *name, fssh_fs_vnode *vnode);
	fssh_status_t (*unlink)(fssh_fs_volume *volume, fssh_fs_vnode *dir,
				const char *name);
	fssh_status_t (*rename)(fssh_fs_volume *volume, fssh_fs_vnode *fromDir,
				const char *fromName, fssh_fs_vnode *toDir, const char *toName);

	fssh_status_t (*access)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				int mode);
	fssh_status_t (*read_stat)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				struct fssh_stat *stat);
	fssh_status_t (*write_stat)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				const struct fssh_stat *stat, uint32_t statMask);
	fssh_status_t (*preallocate)(fssh_fs_volume* volume, fssh_fs_vnode* vnode,
				fssh_off_t pos, fssh_off_t length);

	/* file operations */
	fssh_status_t (*create)(fssh_fs_volume *volume, fssh_fs_vnode *dir,
				const char *name, int openMode, int perms,
				fssh_fs_cookie *_cookie, fssh_vnode_id *_newVnodeID);
	fssh_status_t (*open)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				int openMode, fssh_fs_cookie *_cookie);
	fssh_status_t (*close)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie);
	fssh_status_t (*free_cookie)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie);
	fssh_status_t (*read)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, fssh_off_t pos, void *buffer,
				fssh_size_t *length);
	fssh_status_t (*write)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, fssh_off_t pos, const void *buffer,
				fssh_size_t *length);

	/* directory operations */
	fssh_status_t (*create_dir)(fssh_fs_volume *volume, fssh_fs_vnode *parent,
				const char *name, int perms);
	fssh_status_t (*remove_dir)(fssh_fs_volume *volume, fssh_fs_vnode *parent,
				const char *name);
	fssh_status_t (*open_dir)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie *_cookie);
	fssh_status_t (*close_dir)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie);
	fssh_status_t (*free_dir_cookie)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode, fssh_fs_cookie cookie);
	fssh_status_t (*read_dir)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, struct fssh_dirent *buffer,
				fssh_size_t bufferSize, uint32_t *_num);
	fssh_status_t (*rewind_dir)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie);

	/* attribute directory operations */
	fssh_status_t (*open_attr_dir)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie *_cookie);
	fssh_status_t (*close_attr_dir)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode, fssh_fs_cookie cookie);
	fssh_status_t (*free_attr_dir_cookie)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode, fssh_fs_cookie cookie);
	fssh_status_t (*read_attr_dir)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, struct fssh_dirent *buffer,
				fssh_size_t bufferSize, uint32_t *_num);
	fssh_status_t (*rewind_attr_dir)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode, fssh_fs_cookie cookie);

	/* attribute operations */
	fssh_status_t (*create_attr)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				const char *name, uint32_t type, int openMode,
				fssh_fs_cookie *_cookie);
	fssh_status_t (*open_attr)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				const char *name, int openMode, fssh_fs_cookie *_cookie);
	fssh_status_t (*close_attr)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie);
	fssh_status_t (*free_attr_cookie)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode, fssh_fs_cookie cookie);
	fssh_status_t (*read_attr)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, fssh_off_t pos, void *buffer,
				fssh_size_t *length);
	fssh_status_t (*write_attr)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				fssh_fs_cookie cookie, fssh_off_t pos, const void *buffer,
				fssh_size_t *length);

	fssh_status_t (*read_attr_stat)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode, fssh_fs_cookie cookie,
				struct fssh_stat *stat);
	fssh_status_t (*write_attr_stat)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode, fssh_fs_cookie cookie,
				const struct fssh_stat *stat, int statMask);
	fssh_status_t (*rename_attr)(fssh_fs_volume *volume,
				fssh_fs_vnode *fromVnode, const char *fromName,
				fssh_fs_vnode *toVnode, const char *toName);
	fssh_status_t (*remove_attr)(fssh_fs_volume *volume, fssh_fs_vnode *vnode,
				const char *name);

	/* support for vnode and FS layers */
	fssh_status_t (*create_special_node)(fssh_fs_volume *volume,
				fssh_fs_vnode *dir, const char *name, fssh_fs_vnode *subVnode,
				fssh_mode_t mode, uint32_t flags, fssh_fs_vnode *_superVnode,
				fssh_ino_t *_nodeID);
	fssh_status_t (*get_super_vnode)(fssh_fs_volume *volume,
				fssh_fs_vnode *vnode, fssh_fs_volume *superVolume,
				fssh_fs_vnode *superVnode);
};

typedef struct fssh_file_system_module_info {
	struct fssh_module_info	info;
	const char*				short_name;
	const char*				pretty_name;
	uint32_t				flags;	// DDM flags

	/* scanning (the device is write locked) */
	float (*identify_partition)(int fd, fssh_partition_data *partition,
				void **cookie);
	fssh_status_t (*scan_partition)(int fd, fssh_partition_data *partition,
				void *cookie);
	void (*free_identify_partition_cookie)(fssh_partition_data *partition,
				void *cookie);
	void (*free_partition_content_cookie)(fssh_partition_data *partition);

	/* general operations */
	fssh_status_t (*mount)(fssh_fs_volume *volume, const char *device,
				uint32_t flags, const char *args, fssh_vnode_id *_rootVnodeID);

	/* capability querying (the device is read locked) */
	uint32_t (*get_supported_operations)(fssh_partition_data* partition,
				uint32_t mask);

	bool (*validate_resize)(fssh_partition_data *partition, fssh_off_t *size);
	bool (*validate_move)(fssh_partition_data *partition, fssh_off_t *start);
	bool (*validate_set_content_name)(fssh_partition_data *partition,
				char *name);
	bool (*validate_set_content_parameters)(fssh_partition_data *partition,
				const char *parameters);
	bool (*validate_initialize)(fssh_partition_data *partition, char *name,
				const char *parameters);

	/* shadow partition modification (device is write locked) */
	fssh_status_t (*shadow_changed)(fssh_partition_data *partition,
				fssh_partition_data *child, uint32_t operation);

	/* writing (the device is NOT locked) */
	fssh_status_t (*defragment)(int fd, fssh_partition_id partition,
				fssh_disk_job_id job);
	fssh_status_t (*repair)(int fd, fssh_partition_id partition, bool checkOnly,
				fssh_disk_job_id job);
	fssh_status_t (*resize)(int fd, fssh_partition_id partition,
				fssh_off_t size, fssh_disk_job_id job);
	fssh_status_t (*move)(int fd, fssh_partition_id partition,
				fssh_off_t offset, fssh_disk_job_id job);
	fssh_status_t (*set_content_name)(int fd, fssh_partition_id partition,
				const char *name, fssh_disk_job_id job);
	fssh_status_t (*set_content_parameters)(int fd, fssh_partition_id partition,
				const char *parameters, fssh_disk_job_id job);
	fssh_status_t (*initialize)(int fd, fssh_partition_id partition,
				const char *name, const char *parameters,
				fssh_off_t partitionSize, fssh_disk_job_id job);
	fssh_status_t (*uninitialize)(int fd, fssh_partition_id partition,
				fssh_off_t partitionSize, uint32_t blockSize,
				fssh_disk_job_id job);
} fssh_file_system_module_info;


/* file system add-ons only prototypes */

// callbacks for do_iterative_fd_io()
typedef fssh_status_t (*fssh_iterative_io_get_vecs)(void *cookie,
				fssh_io_request* request, fssh_off_t offset, fssh_size_t size,
				struct fssh_file_io_vec *vecs, fssh_size_t *_count);
typedef fssh_status_t (*fssh_iterative_io_finished)(void* cookie,
				fssh_io_request* request, fssh_status_t status,
				bool partialTransfer, fssh_size_t bytesTransferred);

extern fssh_status_t fssh_new_vnode(fssh_fs_volume *volume,
				fssh_vnode_id vnodeID, void *privateNode,
				fssh_fs_vnode_ops *ops);
extern fssh_status_t fssh_publish_vnode(fssh_fs_volume *volume,
				fssh_vnode_id vnodeID, void *privateNode,
				fssh_fs_vnode_ops *ops, int type, uint32_t flags);
extern fssh_status_t fssh_get_vnode(fssh_fs_volume *volume,
				fssh_vnode_id vnodeID, void **_privateNode);
extern fssh_status_t fssh_put_vnode(fssh_fs_volume *volume,
				fssh_vnode_id vnodeID);
extern fssh_status_t fssh_acquire_vnode(fssh_fs_volume *volume,
				fssh_vnode_id vnodeID);
extern fssh_status_t fssh_remove_vnode(fssh_fs_volume *volume,
				fssh_vnode_id vnodeID);
extern fssh_status_t fssh_unremove_vnode(fssh_fs_volume *volume,
				fssh_vnode_id vnodeID);
extern fssh_status_t fssh_get_vnode_removed(fssh_fs_volume *volume,
				fssh_vnode_id vnodeID, bool* removed);
extern fssh_status_t fssh_mark_vnode_busy(fssh_fs_volume* volume,
				fssh_vnode_id vnodeID, bool busy);
extern fssh_status_t fssh_change_vnode_id(fssh_fs_volume* volume,
				fssh_vnode_id vnodeID, fssh_vnode_id newID);
extern fssh_fs_volume* fssh_volume_for_vnode(fssh_fs_vnode *vnode);
extern fssh_status_t fssh_check_access_permissions(int accessMode,
				fssh_mode_t mode, fssh_gid_t nodeGroupID,
				fssh_uid_t nodeUserID);

extern fssh_status_t fssh_read_pages(int fd, fssh_off_t pos,
				const struct fssh_iovec *vecs, fssh_size_t count,
				fssh_size_t *_numBytes);
extern fssh_status_t fssh_write_pages(int fd, fssh_off_t pos,
				const struct fssh_iovec *vecs, fssh_size_t count,
				fssh_size_t *_numBytes);
extern fssh_status_t fssh_read_file_io_vec_pages(int fd,
				const struct fssh_file_io_vec *fileVecs,
				fssh_size_t fileVecCount, const struct fssh_iovec *vecs,
				fssh_size_t vecCount, uint32_t *_vecIndex,
				fssh_size_t *_vecOffset, fssh_size_t *_bytes);
extern fssh_status_t fssh_write_file_io_vec_pages(int fd,
				const struct fssh_file_io_vec *fileVecs,
				fssh_size_t fileVecCount, const struct fssh_iovec *vecs,
				fssh_size_t vecCount, uint32_t *_vecIndex,
				fssh_size_t *_vecOffset, fssh_size_t *_bytes);
extern fssh_status_t fssh_do_fd_io(int fd, fssh_io_request *request);
extern fssh_status_t fssh_do_iterative_fd_io(int fd, fssh_io_request *request,
				fssh_iterative_io_get_vecs getVecs,
				fssh_iterative_io_finished finished, void *cookie);

extern fssh_status_t fssh_notify_entry_created(fssh_mount_id device,
				fssh_vnode_id directory, const char *name, fssh_vnode_id node);
extern fssh_status_t fssh_notify_entry_removed(fssh_mount_id device,
				fssh_vnode_id directory, const char *name, fssh_vnode_id node);
extern fssh_status_t fssh_notify_entry_moved(fssh_mount_id device,
				fssh_vnode_id fromDirectory, const char *fromName,
				fssh_vnode_id toDirectory, const char *toName,
				fssh_vnode_id node);
extern fssh_status_t fssh_notify_stat_changed(fssh_mount_id device,
				fssh_vnode_id dir, fssh_vnode_id node, uint32_t statFields);
extern fssh_status_t fssh_notify_attribute_changed(fssh_mount_id device,
				fssh_vnode_id dir, fssh_vnode_id node, const char *attribute,
				int32_t cause);

extern fssh_status_t fssh_notify_query_entry_created(fssh_port_id port,
				int32_t token, fssh_mount_id device,
				fssh_vnode_id directory, const char *name,
				fssh_vnode_id node);
extern fssh_status_t fssh_notify_query_entry_removed(fssh_port_id port,
				int32_t token, fssh_mount_id device,
				fssh_vnode_id directory, const char *name,
				fssh_vnode_id node);
extern fssh_status_t fssh_notify_query_attr_changed(fssh_port_id port,
				int32_t token, fssh_mount_id device,
				fssh_vnode_id directory, const char *name,
				fssh_vnode_id node);

#ifdef __cplusplus
}
#endif

#endif	/* _FSSH_FS_INTERFACE_H */
