#ifndef VFS_TYPES_H
#define VFS_TYPES_H

#include <sys/uio.h>

typedef enum {
	STREAM_TYPE_ANY = 0,
	STREAM_TYPE_FILE,
	STREAM_TYPE_DIR,
	STREAM_TYPE_DEVICE
} stream_type;

typedef void * fs_cookie;
typedef void * file_cookie;
typedef void * fs_vnode;

//typedef struct iovec {
//	void *start;
//	size_t len;
//} iovec;

typedef struct iovecs {
	size_t num;
	size_t total_len;
	iovec vec[0];
} iovecs;

//struct file_stat {
//	vnode_id 	vnid;
//	stream_type	type;
//	off_t		size;
//};

#endif

