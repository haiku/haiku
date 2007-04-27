/*
 * Copyright 2002-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_SYS_UIO_H
#define _FSSH_SYS_UIO_H


#include "fssh_types.h"


typedef struct fssh_iovec {
	void		*iov_base;
	fssh_size_t	iov_len;
} fssh_iovec;


#ifdef __cplusplus
extern "C" {
#endif

fssh_ssize_t fssh_readv(int fd, const struct fssh_iovec *vector,
					fssh_size_t count);
fssh_ssize_t fssh_readv_pos(int fd, fssh_off_t pos, const struct
					fssh_iovec *vec, fssh_size_t count);
fssh_ssize_t fssh_writev(int fd, const struct fssh_iovec *vector,
					fssh_size_t count);
fssh_ssize_t fssh_writev_pos(int fd, fssh_off_t pos,
					const struct fssh_iovec *vec, fssh_size_t count);

#ifdef __cplusplus
}
#endif

#endif /* _FSSH_SYS_UIO_H */
