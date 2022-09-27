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


static struct dirent*
next_dirent(struct dirent* dirent, size_t nameLength, size_t& bufferRemaining)
{
	const size_t reclen = offsetof(struct dirent, d_name) + nameLength + 1;
	ASSERT(reclen <= bufferRemaining);
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
