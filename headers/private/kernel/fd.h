/* file.h
 *
 * File Descriptors
 */
 
#ifndef _FILE_H
#define _FILE_H

#include <sem.h>
#include <lock.h>
#include <atomic.h>
#include <memheap.h>
#include <sys/stat.h>


struct file_descriptor;
struct fs_mount;
struct vnode;

/** The I/O context of a process/team, holds the fd array */
struct io_context {
	struct vnode *cwd;
	mutex	io_mutex;
	int		table_size;
	int		num_used_fds;
	struct file_descriptor **fds;
};

struct fd_ops {
	char *fs_name;	// can be removed, it's not used anywhere
	ssize_t		(*fd_read) (struct file_descriptor *, off_t pos, void *buffer, size_t *length);
	ssize_t		(*fd_write)(struct file_descriptor *, off_t pos, const void *buffer, size_t *length);
	off_t		(*fd_seek)(struct file_descriptor *, off_t pos, int seekType);
	int			(*fd_ioctl)(struct file_descriptor *, ulong op, void *buffer, size_t length);
//	int			(*fd_poll)(struct file_descriptor *, int);
	status_t	(*fd_read_dir)(struct file_descriptor *,struct dirent *buffer,size_t bufferSize,uint32 *_count);
	status_t	(*fd_rewind_dir)(struct file_descriptor *);
	int			(*fd_stat)(struct file_descriptor *, struct stat *);
	int			(*fd_close)(struct file_descriptor *);
	void		(*fd_free)(struct file_descriptor *);
};

struct file_descriptor {
	int32	type;               /* descriptor type */
	int32	ref_count;
	struct fd_ops *ops;
	union {
		struct vnode *vnode;
		struct fs_mount *mount;
	} u;
	void	*cookie;
	int32	open_mode;
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


/* Prototypes */

struct file_descriptor *alloc_fd(void);
int new_fd(struct io_context *, struct file_descriptor *);
struct file_descriptor *get_fd(struct io_context *, int);
void put_fd(struct file_descriptor *);
void free_fd(struct file_descriptor *);
static struct io_context *get_current_io_context(bool kernel);

/* The prototypes of the (sys|user)_ functions are currently defined in vfs.h */


/* Inlines */

static __inline struct io_context *get_current_io_context(bool kernel)
{
	if (kernel)
		return team_get_kernel_team()->ioctx;

	return thread_get_current_thread()->team->ioctx;
}

#endif /* _FILE_H */
