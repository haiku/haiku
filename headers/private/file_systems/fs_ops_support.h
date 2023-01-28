/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_OPS_SUPPORT_H
#define FS_OPS_SUPPORT_H

#ifndef FS_SHELL
#	include <kernel.h>
#	include <kernel/debug.h>
#	include <dirent.h>
#else
#	include "fssh_kernel_priv.h"
#endif


/*!	Converts a given open mode (e.g. O_RDONLY) into access modes (e.g. R_OK).
 */
static inline int
open_mode_to_access(int openMode)
{
	openMode &= O_RWMASK;
	if (openMode == O_RDONLY)
		return R_OK;
	if (openMode == O_WRONLY)
		return W_OK;
	if (openMode == O_RDWR)
		return R_OK | W_OK;
	return 0;
}


/*! Computes and assigns `dirent->d_reclen`, adjusts `bufferRemaining` accordingly,
 * and either advances to the next buffer, or returns NULL if no space remains.
 */
static inline struct dirent*
next_dirent(struct dirent* dirent, size_t nameLength, size_t& bufferRemaining)
{
	const size_t reclen = offsetof(struct dirent, d_name) + nameLength + 1;
#ifdef ASSERT
	ASSERT(reclen <= bufferRemaining);
#endif
	dirent->d_reclen = reclen;

	const size_t roundedReclen = ROUNDUP(reclen, alignof(struct dirent));
	if (roundedReclen >= bufferRemaining) {
		bufferRemaining -= reclen;
		return NULL;
	}
	dirent->d_reclen = roundedReclen;
	bufferRemaining -= roundedReclen;

	return (struct dirent*)((uint8*)dirent + roundedReclen);
}


#endif	// FS_OPS_SUPPORT_H
