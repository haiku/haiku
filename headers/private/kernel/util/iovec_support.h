/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _UTIL_IOVEC_SUPPORT_H
#define _UTIL_IOVEC_SUPPORT_H


#include <KernelExport.h>


typedef struct generic_io_vec {
	generic_addr_t	base;
	generic_size_t	length;
} generic_io_vec;


#ifdef IS_USER_ADDRESS
static inline status_t
get_iovecs_from_user(const iovec* userVecs, size_t vecCount, iovec*& vecs,
	bool permitNull = false)
{
	// prevent integer overflow
	if (vecCount > IOV_MAX || vecCount == 0)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userVecs))
		return B_BAD_ADDRESS;

	vecs = (iovec*)malloc(sizeof(iovec) * vecCount);
	if (vecs == NULL)
		return B_NO_MEMORY;

	if (user_memcpy(vecs, userVecs, sizeof(iovec) * vecCount) != B_OK) {
		free(vecs);
		return B_BAD_ADDRESS;
	}

	size_t total = 0;
	for (size_t i = 0; i < vecCount; i++) {
		if (permitNull && vecs[i].iov_base == NULL)
			continue;
		if (!is_user_address_range(vecs[i].iov_base, vecs[i].iov_len)) {
			free(vecs);
			return B_BAD_ADDRESS;
		}
		if (vecs[i].iov_len > SSIZE_MAX || total > (SSIZE_MAX - vecs[i].iov_len)) {
			free(vecs);
			return B_BAD_VALUE;
		}
		total += vecs[i].iov_len;
	}

	return B_OK;
}
#endif


#endif	// _UTIL_IOVEC_SUPPORT_H
