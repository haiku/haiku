/*
 * Copyright 2002-2018, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FD_H
#define _FD_H


#include <vfs.h>
#include <team.h>
#include <thread.h>


#ifdef __cplusplus
extern "C" {
#endif

struct file_descriptor;
struct io_context;
struct selectsync;
struct select_info;

struct fd_ops {
	status_t	(*fd_close)(struct file_descriptor *);
	void		(*fd_free)(struct file_descriptor *);

	status_t	(*fd_read)(struct file_descriptor *, off_t pos, void *buffer,
						size_t *length);
	status_t	(*fd_write)(struct file_descriptor *, off_t pos,
						const void *buffer, size_t *length);
	ssize_t		(*fd_readv)(struct file_descriptor *, off_t pos,
						const struct iovec *vecs, int count);
	ssize_t		(*fd_writev)(struct file_descriptor *, off_t pos,
						const struct iovec *vecs, int count);
	off_t		(*fd_seek)(struct file_descriptor *, off_t pos, int seekType);

	status_t	(*fd_ioctl)(struct file_descriptor *, ulong op, void *buffer,
						size_t length);
	status_t	(*fd_set_flags)(struct file_descriptor *, int flags);

	status_t	(*fd_select)(struct file_descriptor *, uint8 event,
						struct selectsync *sync);
	status_t	(*fd_deselect)(struct file_descriptor *, uint8 event,
						struct selectsync *sync);

	status_t	(*fd_read_dir)(struct io_context* ioContext,
						struct file_descriptor *, struct dirent *buffer,
						size_t bufferSize, uint32 *_count);
	status_t	(*fd_rewind_dir)(struct file_descriptor *);

	status_t	(*fd_read_stat)(struct file_descriptor *, struct stat *);
	status_t	(*fd_write_stat)(struct file_descriptor *, const struct stat *,
		int statMask);
};

struct file_descriptor {
	int32	ref_count;
	int32	open_count;
	struct fd_ops *ops;
	union {
		struct vnode *vnode;
		struct fs_mount *mount;
	} u;
	void	*cookie;
	int32	open_mode;
	off_t	pos;
};


// additional open mode - kernel special
#define O_DISCONNECTED 0x80000000

/* Prototypes */

extern struct file_descriptor *alloc_fd(void);
extern int new_fd_etc(struct io_context *, struct file_descriptor *,
	int firstIndex);
extern int new_fd(struct io_context *, struct file_descriptor *);
extern struct file_descriptor *get_fd(struct io_context *, int);
extern struct file_descriptor *get_open_fd(struct io_context *, int);
extern void close_fd(struct io_context *context,
	struct file_descriptor *descriptor);
extern status_t close_fd_index(struct io_context *context, int fd);
extern void put_fd(struct file_descriptor *descriptor);
extern void disconnect_fd(struct file_descriptor *descriptor);
extern void inc_fd_ref_count(struct file_descriptor *descriptor);
extern int dup_foreign_fd(team_id fromTeam, int fd, bool kernel);
extern status_t select_fd(int32 fd, struct select_info *info, bool kernel);
extern status_t deselect_fd(int32 fd, struct select_info *info, bool kernel);
extern bool fd_is_valid(int fd, bool kernel);
extern struct vnode *fd_vnode(struct file_descriptor *descriptor);
extern bool fd_is_file(struct file_descriptor* descriptor);

extern bool fd_close_on_exec(struct io_context *context, int fd);
extern void fd_set_close_on_exec(struct io_context *context, int fd,
	bool closeFD);

static struct io_context *get_current_io_context(bool kernel);

extern status_t user_fd_kernel_ioctl(int fd, ulong op, void *buffer,
	size_t length);

/* The prototypes of the (sys|user)_ functions are currently defined in vfs.h */


/* Inlines */

static inline struct io_context *
get_current_io_context(bool kernel)
{
	if (kernel)
		return (struct io_context *)team_get_kernel_team()->io_context;

	return (struct io_context *)thread_get_current_thread()->team->io_context;
}

#ifdef __cplusplus
}
#endif

#endif /* _FD_H */
