#ifndef VFS_TYPES_H
#define VFS_TYPES_H

#include <sys/uio.h>

typedef struct iovecs {
	size_t num;
	size_t total_len;
	iovec vec[0];
} iovecs;

#endif	/* VFS_TYPES_H */

