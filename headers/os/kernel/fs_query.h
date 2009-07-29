/*
 * Copyright 2002-2003, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FS_QUERY_H
#define _FS_QUERY_H


#include <OS.h>
#include <dirent.h>


/* Flags for fs_open_[live_]query() */

#define B_LIVE_QUERY		0x00000001
	// Note, if you specify B_LIVE_QUERY, you have to use fs_open_live_query();
	// it will be ignored in fs_open_query().
#define B_QUERY_NON_INDEXED	0x00000002
	// Only enable this feature for non time-critical things, it might
	// take a long time to proceed.
	// Also, not every file system might support this feature.


#ifdef  __cplusplus
extern "C" {
#endif

extern DIR		*fs_open_query(dev_t device, const char *query, uint32 flags);
extern DIR		*fs_open_live_query(dev_t device, const char *query,
					uint32 flags, port_id port, int32 token);
extern int		fs_close_query(DIR *d);
extern struct dirent *fs_read_query(DIR *d);

extern status_t	get_path_for_dirent(struct dirent *dent, char *buf,
					size_t len);

#ifdef  __cplusplus
}
#endif

#endif	/* _FS_QUERY_H */
