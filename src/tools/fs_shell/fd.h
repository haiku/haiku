/* 
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_FD_H
#define _FSSH_FD_H


#include "vfs.h"


namespace FSShell {


struct file_descriptor;
struct select_sync;
struct vnode;

extern io_context* gKernelIOContext;


struct fd_ops {
	fssh_status_t	(*fd_read)(struct file_descriptor *, fssh_off_t pos,
							void *buffer, fssh_size_t *length);
	fssh_status_t	(*fd_write)(struct file_descriptor *, fssh_off_t pos,
							const void *buffer, fssh_size_t *length);
	fssh_off_t		(*fd_seek)(struct file_descriptor *, fssh_off_t pos,
							int seekType);
	fssh_status_t	(*fd_ioctl)(struct file_descriptor *, uint32_t op,
							void *buffer, fssh_size_t length);
	fssh_status_t	(*fd_select)(struct file_descriptor *, uint8_t event,
							uint32_t ref, struct select_sync *sync);
	fssh_status_t	(*fd_deselect)(struct file_descriptor *, uint8_t event,
							struct select_sync *sync);
	fssh_status_t	(*fd_read_dir)(struct file_descriptor *,
							struct fssh_dirent *buffer, fssh_size_t bufferSize,
							uint32_t *_count);
	fssh_status_t	(*fd_rewind_dir)(struct file_descriptor *);
	fssh_status_t	(*fd_read_stat)(struct file_descriptor *,
							struct fssh_stat *);
	fssh_status_t	(*fd_write_stat)(struct file_descriptor *,
							const struct fssh_stat *, int statMask);
	fssh_status_t	(*fd_close)(struct file_descriptor *);
	void			(*fd_free)(struct file_descriptor *);
};

struct file_descriptor {
	int32_t			type;               /* descriptor type */
	int32_t			ref_count;
	int32_t			open_count;
	struct fd_ops*	ops;
	union {
		struct vnode*		vnode;
		struct fs_mount*	mount;
	} u;
	void*			cookie;
	int32_t			open_mode;
	fssh_off_t		pos;
};


/* Types of file descriptors we can create */

enum fd_types {
	FDTYPE_FILE	= 1,
	FDTYPE_ATTR,
	FDTYPE_DIR,
	FDTYPE_ATTR_DIR,
	FDTYPE_INDEX,
	FDTYPE_INDEX_DIR,
	FDTYPE_QUERY,
	FDTYPE_SOCKET
};

// additional open mode - kernel special
#define FSSH_O_DISCONNECTED 0x80000000

/* Prototypes */

extern file_descriptor*	alloc_fd(void);
extern int				new_fd_etc(struct io_context *,
							struct file_descriptor *, int firstIndex);
extern int				new_fd(struct io_context *, struct file_descriptor *);
extern file_descriptor*	get_fd(struct io_context *, int);
extern void				close_fd(struct file_descriptor *descriptor);
extern void				put_fd(struct file_descriptor *descriptor);
extern void				disconnect_fd(struct file_descriptor *descriptor);
extern void				inc_fd_ref_count(struct file_descriptor *descriptor);
extern fssh_status_t	select_fd(int fd, uint8_t event, uint32_t ref,
							struct select_sync *sync, bool kernel);
extern fssh_status_t	deselect_fd(int fd, uint8_t event,
							struct select_sync *sync, bool kernel);
extern bool				fd_is_valid(int fd, bool kernel);
extern vnode*			fd_vnode(struct file_descriptor *descriptor);

extern bool				fd_close_on_exec(struct io_context *context, int fd);
extern void				fd_set_close_on_exec(struct io_context *context, int fd,
							bool closeFD);

static io_context*		get_current_io_context(bool kernel);

/* The prototypes of the (sys|user)_ functions are currently defined in vfs.h */


/* Inlines */

static inline struct io_context *
get_current_io_context(bool /*kernel*/)
{
	return gKernelIOContext;
}


}	// namespace FSShell


#endif /* _FSSH_FD_H */
